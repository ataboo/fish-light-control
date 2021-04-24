// #define LOCAL_LOG_LEVEL ESP_LOG_DEBUG
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"

#include "wifi_time.h"
#include "light_control.h"
#include "scheduler.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ssd-1306-i2c.h"

static const char *TAG = "ESP_FISH_LIGHT";

void app_main(void)
{
    // esp_log_level_set("*", ESP_LOG_INFO);

    init_lcd_i2c();

    wifi_time_init();
    ESP_ERROR_CHECK(light_control_init());
    vTaskDelay(50/portTICK_PERIOD_MS);
    
    scheduler_start();
}
