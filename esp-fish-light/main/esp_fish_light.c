// #define LOCAL_LOG_LEVEL ESP_LOG_DEBUG
#include <string.h>
#include <time.h>
#include "esp_system.h"
#include "esp_log.h"

#include "wifi_time.h"
#include "light_control.h"
#include "scheduler.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "display_control.h"
#include "temp_control.h"
#include "buzzer_control.h"
#include "buzzer_music.h"

static const char *TAG = "ESP_FISH_LIGHT";

static void indicate_failure(char* messages, int count) {
    ESP_LOGE(TAG, "Fatal error on startup.  Resetting in 10 seconds.");
    light_error();
    display_control_fatal_error(messages, count);

    vTaskDelay(10000/portTICK_PERIOD_MS);
    esp_restart();
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);

    wifi_time_init();

    esp_err_t light_ret = light_control_init();
    esp_err_t display_ret = display_control_init();
    esp_err_t temp_ret = temp_control_init();
    esp_err_t buzzer_ret = buzzer_control_init();

    int errCount = 0;
    char err_buffer[3][ERR_MAX_STR_LEN];
    if(light_ret != ESP_OK) {
        strcpy(err_buffer[errCount++], "Light Control");
    }

    if (display_ret != ESP_OK) {
        strcpy(err_buffer[errCount++], "Display Control");
    }

    if (temp_ret != ESP_OK) {
        strcpy(err_buffer[errCount++], "Temp Control");
    }

    if (buzzer_ret != ESP_OK) {
        strcpy(err_buffer[errCount++], "Buzzer Control");
    }

    if (errCount > 0) {
        indicate_failure((char*)err_buffer, errCount);
    }

    scheduler_start();
}
