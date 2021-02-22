#define LOCAL_LOG_LEVEL ESP_LOG_DEBUG
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_sleep.h"
#include "esp_timer.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi_time.h"
#include "light_control.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define SLEEP_TIME_SECS 10
#define TIME_UPDATE_BITS BIT0

static const char *TAG = "ESP_FISH_LIGHT";

static time_t now = 0;
static xSemaphoreHandle time_update_lock;
static esp_timer_handle_t update_timer;


static void timeupdate_callback(void* arg) {
    xSemaphoreTake(time_update_lock, portMAX_DELAY);
    ESP_LOGI(TAG, "update timer fired");
    esp_err_t ret = blocking_update_time();
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to update time");
    } else {
        ESP_LOGI(TAG, "updated time on callback");
    }
    xSemaphoreGive(time_update_lock);
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_DEBUG);
    time_update_lock = xSemaphoreCreateMutex();

    wifi_time_init();
    ESP_ERROR_CHECK(light_control_init());
    ESP_ERROR_CHECK(light_loading());

    esp_err_t ret = ESP_FAIL;
    while(ret != ESP_OK) {
        ret = blocking_update_time();
        if(ret != ESP_OK) {
            ESP_LOGE(TAG, "failed initial time update");
            ESP_ERROR_CHECK(light_error());
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
    }

    esp_timer_create_args_t periodic_timer_args = {
        .callback = &timeupdate_callback,
        .name = "update_time"
    };

    esp_timer_create(&periodic_timer_args, &update_timer);
    esp_timer_start_periodic(update_timer, 30e6);

    while(true) {
        xSemaphoreTake(time_update_lock, portMAX_DELAY);
        time(&now);
        light_dim_for_time(now);
        ESP_LOGI(TAG, "Sleeping for %d seconds", SLEEP_TIME_SECS);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        esp_sleep_enable_timer_wakeup(SLEEP_TIME_SECS * 1e6);
        esp_light_sleep_start();
        ESP_LOGI(TAG, "Wokeup");
        xSemaphoreGive(time_update_lock);
    }

    ESP_LOGI(TAG, "Done!");
}
