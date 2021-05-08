#include <math.h>
#include <string.h>
#include "temp_control.h"
// #define LOCAL_LOG_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "owb.h"
#include "owb_rmt.h"
#include "ds18b20.h"

#define TEMP_ID_LEN 17
#define TEMP_LABEL_LEN 8

#define MAX_TEMP_PROBES 10
#define TEMP_PIN_NUM GPIO_NUM_27

typedef struct {
    temp_warn_t warn;
    char label[TEMP_LABEL_LEN];
    char id[TEMP_ID_LEN];
    DS18B20_Info* owb_device;
} temp_device_t;

static const char* TAG = "TEMP_CTRL";
static DS18B20_Info* devices[MAX_TEMP_PROBES] = {0};
static int connected_devices;
static owb_rmt_driver_info owb_driver_info;
static OneWireBus* owb = NULL;

static temp_device_t temp_devices[CONFIG_TEMP_COUNT] = {0};

static bool match_device_to_temp_value(DS18B20_Info* info) {
    temp_device_t* first_empty_value = NULL;

    char rom_code_str[17];
    owb_string_from_rom_code(info->rom_code, rom_code_str, sizeof(rom_code_str));

    for(int i=0; i<CONFIG_TEMP_COUNT; i++) {
        if (temp_devices[i].owb_device != NULL) {
            continue;
        }

        if (strcmp(temp_devices[i].id, rom_code_str) == 0) {
            temp_devices[i].owb_device = info;
            ESP_LOGI(TAG, "Matched configured device '%s' with id: '%s'", temp_devices[i].label, rom_code_str);
            return true;
        }

        if (first_empty_value == NULL && temp_devices[i].id[0] == '\0') {
            first_empty_value = &temp_devices[i];
        }
    }

    if (first_empty_value != NULL) {
        first_empty_value->owb_device = info;
        ESP_LOGI(TAG, "Assigned temp device with id: '%s' to '%s' since no id is configured", rom_code_str, first_empty_value->label);
        return true;
    }

    ESP_LOGW(TAG, "Failed to find matching configured temp value for device: '%s'", rom_code_str);
    return false;
}

static esp_err_t find_and_init_temp_devices() {
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    owb = owb_rmt_initialize(&owb_driver_info, TEMP_PIN_NUM, RMT_CHANNEL_1, RMT_CHANNEL_0);
    if (!owb) {
        return ESP_FAIL;
    }

    owb_use_crc(owb, true);

    OneWireBus_ROMCode device_rom_codes[MAX_TEMP_PROBES] = {0};
    connected_devices = 0;
    OneWireBus_SearchState search_state = {0};
    bool found = false;
    owb_search_first(owb, &search_state, &found);
    while (found)
    {
        char rom_code_s[17];
        owb_string_from_rom_code(search_state.rom_code, rom_code_s, sizeof(rom_code_s));
        ESP_LOGI(TAG, "Temp Probe %d: %s\n", connected_devices, rom_code_s);
        device_rom_codes[connected_devices] = search_state.rom_code;
        ++connected_devices;
        owb_search_next(owb, &search_state, &found);
    }
    ESP_LOGI(TAG, "Found %d device%s\n", connected_devices, connected_devices == 1 ? "" : "s");

    // Create DS18B20 devices on the 1-Wire bus
    for (int i = 0; i < connected_devices; ++i)
    {
        DS18B20_Info * ds18b20_info = ds18b20_malloc();  // heap allocation
        devices[i] = ds18b20_info;

        ds18b20_init(ds18b20_info, owb, device_rom_codes[i]); // associate with bus and device
        ds18b20_use_crc(ds18b20_info, true);           // enable CRC check on all reads
        ds18b20_set_resolution(ds18b20_info, DS18B20_RESOLUTION_12_BIT);

        match_device_to_temp_value(ds18b20_info);
    }

    return ESP_OK;
}

esp_err_t temp_config_value_for_idx(int idx, char* label, char* id) {
    switch (idx)
    {
    case 0:
        strcpy(label, CONFIG_TEMP_0_NAME);
        strcpy(id, CONFIG_TEMP_0_ID);
        break;
    case 1:
        strcpy(label, CONFIG_TEMP_1_NAME);
        strcpy(id, CONFIG_TEMP_1_ID);
        break;
    case 2:
        strcpy(label, CONFIG_TEMP_2_NAME);
        strcpy(id, CONFIG_TEMP_2_ID);
        break;
    case 3:
        strcpy(label, CONFIG_TEMP_3_NAME);
        strcpy(id, CONFIG_TEMP_3_ID);
        break;
    default:
        return ESP_ERR_INVALID_SIZE;
        break;
    }

    return ESP_OK;
}

esp_err_t temp_label_for_idx(int idx, char* label) {
    char id[17];
    return temp_config_value_for_idx(idx, label, id);
}

esp_err_t temp_control_init() {
    for(int i=0; i<CONFIG_TEMP_COUNT; i++) {
        temp_config_value_for_idx(i, temp_devices[i].label, temp_devices[i].id);
    }

    esp_err_t ret = find_and_init_temp_devices();
    if (ret != ESP_OK) {
        return ret;
    }

    for(int i=0; i<CONFIG_TEMP_COUNT; i++) {
        if (temp_devices[i].owb_device == NULL) {
            ESP_LOGE(TAG, "Failed to find device for temp '%s' matching id '%s'", temp_devices[i].label, temp_devices[i].id);
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}

static temp_warn_t get_temp_warn(float temp) {
    float tenths = temp * 10;
    
    if (tenths <= CONFIG_TEMP_COLD_LIMIT) {
        return TEMP_COLD;
    }

    if (tenths <= CONFIG_TEMP_COOL_LIMIT) {
        return TEMP_COOL;
    }

    if (tenths < CONFIG_TEMP_WARM_LIMIT) {
        return TEMP_NOMINAL;
    }

    if(tenths < CONFIG_TEMP_HOT_LIMIT) {
        return TEMP_WARM;
    }

    return TEMP_HOT;
}

esp_err_t temp_control_update(temp_data_t* data) {
    if (CONFIG_TEMP_COUNT == 0 || !temp_devices[0].owb_device) {
        ESP_LOGE(TAG, "Failed to read temps since first temp device not configured.");
        return ESP_FAIL;
    }

    ds18b20_convert_all(owb);
    ds18b20_wait_for_conversion(temp_devices[0].owb_device);

    for (int i = 0; i < CONFIG_TEMP_COUNT; ++i)
    {
        if (!temp_devices[i].owb_device) {
            data[i].err = TEMP_READ_ERR_NOT_FOUND;
            data[i].temp = 0;
            data[i].warn = TEMP_NOMINAL;
            continue;
        }

        data[i].err = ds18b20_read_temp(temp_devices[i].owb_device, &data[i].temp);
        data[i].warn = get_temp_warn(data[i].temp);
    }

    return ESP_OK;
}
