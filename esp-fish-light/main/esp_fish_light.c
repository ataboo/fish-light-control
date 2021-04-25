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

static const char *TAG = "ESP_FISH_LIGHT";

void app_main(void)
{
    // esp_log_level_set("*", ESP_LOG_INFO);

    float temp_vals[3] = {1.1, 2.2, -3.3};
    struct tm now = {
        .tm_hour = 12,
        .tm_min = 34
    };

    display_values_t values = {
        .light_level = 0.5,
        .temp_count = 3,
        .temps = temp_vals,
        .time = &now 
    };

    ESP_ERROR_CHECK_WITHOUT_ABORT(display_control_init());

    display_control_update(&values);

    // wifi_time_init();
    // ESP_ERROR_CHECK(light_control_init());
    // vTaskDelay(50/portTICK_PERIOD_MS);
    
    // scheduler_start();
}
