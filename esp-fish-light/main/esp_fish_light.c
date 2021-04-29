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

static const char *TAG = "ESP_FISH_LIGHT";

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);

    time_t now;
    
    struct tm timeinfo;

    wifi_time_init();
    ESP_ERROR_CHECK_WITHOUT_ABORT(display_control_init());
    ESP_ERROR_CHECK_WITHOUT_ABORT(temp_control_init());

    // char temp_labels[3][TEMP_LABEL_MAX_STR_LEN] = { "Dathan", "Johnny", "Thanatos" };
    // float temps[3] = { 25.5f, 28.2f, 30.1f };
    // temp_warn_t temp_warns[3] = {TEMP_NOMINAL, TEMP_WARM, TEMP_HOT};

    scheduler_start();

    // while(true) {
    //     time(&now);
    //     localtime_r(&now, &timeinfo);

    //     display_values_t values = {
    //         .light_level = 0.5,
    //         .time = &timeinfo,
    //         .temp_labels = (char*)temp_labels,
    //         .temp_count = 3,
    //         .temp_warns = (temp_warn_t*)temp_warns,
    //         .temps = temps,
    //     };

    //     display_control_update(&values);

    //     vTaskDelay(1000/portTICK_RATE_MS);

    //     // ESP_ERROR_CHECK(light_control_init());
    //     // vTaskDelay(50/portTICK_PERIOD_MS);


    // }

    temp_data_t* data = CREATE_TEMP_CONTROL_DATA();
}
