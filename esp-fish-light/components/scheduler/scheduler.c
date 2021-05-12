// #define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "scheduler.h"
#include "light_control.h"
#include "display_control.h"
#include "wifi_time.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "time.h"
#include "esp_sleep.h"
#include "temp_control.h"
#include "buzzer_control.h"
#include "buzzer_music.h"

#define CLOCK_UPDATE_COOLDOWN_MINS (60) 

static const char* TAG = "FISH_LIGHT_SCHEDULER";

static int sunrise_start_mins = 0;
static int sunrise_end_mins = 0;
static int sunset_start_mins = 0;
static int sunset_end_mins = 0;

static int64_t clock_update_cooldown_micros;
static int64_t last_update_micros;
static time_t now = 0;
static uint16_t sleep_time_mins;
static struct tm timeinfo;
static temp_data_t* temp_data;
static display_values_t disp_values; 
static bool temp_alert_active = false;
static buzzer_pattern_t* warn_pattern;
static buzzer_pattern_t* alarm_pattern;
static temp_warn_t current_alarm;

static int hm_to_mins(int hour_mins) {
    return (hour_mins / 100 * 60) + (hour_mins % 100);
}

static void init_buzzer_patterns() {
    ESP_ERROR_CHECK_WITHOUT_ABORT(parse_music_str("l1o6f#o3fo6f#o3fo6f#o3fr8r8", &alarm_pattern));
    alarm_pattern->loop = false;
    alarm_pattern->waveform = BUZZER_WAV_SAW;

    ESP_ERROR_CHECK_WITHOUT_ABORT(parse_music_str("l1o5gb$r2gb$r8r8r8r8", &warn_pattern));
    warn_pattern->loop = true;
    warn_pattern->waveform = BUZZER_WAV_SIN;
}

static float level_for_time(int mins) {
    if(mins > sunrise_start_mins && mins < sunset_end_mins) {
        if (mins < sunrise_end_mins) {
            return (float)(mins - sunrise_start_mins) / (sunrise_end_mins - sunrise_start_mins);
        }

        if (mins > sunset_start_mins) {
            return 1 - (float)(mins - sunset_start_mins) / (sunset_end_mins - sunset_start_mins);
        }

        return 1;
    }

    return 0;
}

static void update_light_for_time() {
    int mins = timeinfo.tm_hour * 60 + timeinfo.tm_min;
    float level = level_for_time(mins);

    ESP_ERROR_CHECK_WITHOUT_ABORT(light_dim_level(level));
}

static void update_display(struct tm* time_info) {    
    disp_values.time = time_info;
    display_control_update(&disp_values);
}

static esp_err_t update_temp_data() {
    esp_err_t ret = temp_control_update(temp_data);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to update temp values");
    }

    temp_warn_t lowest_alarm = TEMP_NOMINAL;
    for(int i=0; i<CONFIG_TEMP_COUNT; i++) {
        disp_values.temp_data[i].temp = temp_data[i].temp;
        disp_values.temp_data[i].warning = temp_data[i].warn;
        if (temp_data[i].warn != TEMP_NOMINAL) {
            lowest_alarm = temp_data[i].warn < lowest_alarm ? temp_data[i].warn : lowest_alarm;
        }
    }

    if (lowest_alarm != current_alarm) {
        switch (lowest_alarm)
        {
        case TEMP_NOMINAL:
            buzzer_control_play_pattern(NULL);
            break;
        case TEMP_COOL:
        case TEMP_WARM:
            buzzer_control_play_pattern(warn_pattern);
            break;
        case TEMP_COLD:
        case TEMP_HOT:
            buzzer_control_play_pattern(alarm_pattern);
            break;
        default:
            return ESP_ERR_NOT_SUPPORTED;
        }

        current_alarm = lowest_alarm;
    }

    return ESP_OK;
}

static uint16_t mins_until_next_wakeup() {

    return 1;

    //If the data line gets less noise, will use something like this again.
    // int mins = timeinfo.tm_hour * 60 + timeinfo.tm_min;

    // if(mins < sunrise_start_mins) {
    //     return (sunrise_start_mins - mins);
    // }

    // if (mins < sunrise_end_mins) {
    //     return 1;
    // }

    // if (mins < sunset_start_mins) {
    //     return sunset_start_mins - mins;
    // }

    // if (mins < sunset_end_mins) {
    //     return 1;
    // }

    // return 24 * 60 - mins + sunrise_start_mins;
}

static void update_internal_clock() {
    esp_err_t ret = blocking_update_time();
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to update time");
    } else {
        ESP_LOGD(TAG, "successfully updated clock");
        last_update_micros = esp_timer_get_time();
    }
}

static void scheduler_loop_task(void* arg) {
    ESP_LOGD(TAG, "Started update task");
    while(true) {
        time(&now);
        localtime_r(&now, &timeinfo);
        ESP_LOGI(TAG, "Updating for time: %.2d:%.2d", timeinfo.tm_hour, timeinfo.tm_min);
        update_light_for_time(now);
        update_temp_data();

        update_display(&timeinfo);
        
        if (esp_timer_get_time() - last_update_micros > clock_update_cooldown_micros) {
            update_internal_clock();
        }

        if(temp_alert_active) {
            vTaskDelay(1000/portTICK_PERIOD_MS);
        } else {
            sleep_time_mins = mins_until_next_wakeup();
            ESP_LOGI(TAG, "Sleeping for %d seconds", sleep_time_mins * 60);
            esp_sleep_enable_timer_wakeup((uint64_t)sleep_time_mins * 60e6);
            vTaskDelay(50 / portTICK_PERIOD_MS);
            esp_light_sleep_start();
        }
    }

    vTaskDelete(NULL);
}

static void scheduler_init() {
    clock_update_cooldown_micros = CLOCK_UPDATE_COOLDOWN_MINS * 60 * 1e6;
    sunrise_start_mins = hm_to_mins(CONFIG_SUNRISE_START);
    sunrise_end_mins = hm_to_mins(CONFIG_SUNRISE_END);
    sunset_start_mins = hm_to_mins(CONFIG_SUNSET_START);
    sunset_end_mins = hm_to_mins(CONFIG_SUNSET_END);
    temp_data = CREATE_TEMP_CONTROL_DATA();

    disp_values.light_level = 0;
    disp_values.temp_count = CONFIG_TEMP_COUNT;
    disp_values.temp_data = malloc(sizeof(temp_disp_t) * CONFIG_TEMP_COUNT);

    for(int i=0; i<CONFIG_TEMP_COUNT; i++) {
        temp_label_for_idx(i, disp_values.temp_data[i].temp_label);
    }

    ESP_LOGD(TAG, "Sunrise: %d mins - %d mins, Sunset: %d mins - %d mins", sunrise_start_mins, sunrise_end_mins, sunset_start_mins, sunset_end_mins);

    ESP_ERROR_CHECK_WITHOUT_ABORT(light_loading());

    esp_err_t ret = ESP_FAIL;
    while(ret != ESP_OK) {
        ret = blocking_update_time();
        if(ret != ESP_OK) {
            ESP_LOGE(TAG, "failed initial time update");
            ESP_ERROR_CHECK_WITHOUT_ABORT(light_error());
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
    }

    last_update_micros = esp_timer_get_time();
}

esp_err_t scheduler_start() {
    scheduler_init();

    xTaskCreate(scheduler_loop_task, "scheduler loop", 4096, NULL, 5, NULL);

    return ESP_OK;
}
