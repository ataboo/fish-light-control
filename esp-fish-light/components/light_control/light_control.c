
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "light_control.h"
#include "ws2812.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt.h"
#include "esp_system.h"
#include "time.h"
#include "esp_log.h"


static const char *TAG = "LIGHT_CONTROL";
static led_strip_t* led_strip;
static struct tm timeinfo;
static color_rgb_t last_color = {-1, -1, -1};
static color_rgb_t current_color;

#define RMT_TX_CHANNEL RMT_CHANNEL_0
#define RMT_CLOCK_DIV 1


static color_rgb_t color_for_level(float level) {
    uint8_t r = (uint8_t)((CONFIG_MIDDAY_COLOR>>16 & 0xFF) * level);
    uint8_t g = (uint8_t)((CONFIG_MIDDAY_COLOR>>8 & 0xFF) * level);
    uint8_t b = (uint8_t)((CONFIG_MIDDAY_COLOR & 0xFF) * level);
    
    return (color_rgb_t){r, g, b};
}

esp_err_t light_control_init() {
    rmt_config_t rmt_tx_config = RMT_DEFAULT_CONFIG_TX(CONFIG_LIGHTSTRIP_GPIO, RMT_TX_CHANNEL);
    rmt_tx_config.clk_div = RMT_CLOCK_DIV;
    ESP_ERROR_CHECK(rmt_config(&rmt_tx_config));
    ESP_ERROR_CHECK(rmt_driver_install(RMT_TX_CHANNEL, 0, 0));

    led_strip_config_t ws2812_config = LED_STRIP_DEFAULT_CONFIG(CONFIG_LED_COUNT, RMT_TX_CHANNEL);
    led_strip = ws2812_init(&ws2812_config);

    led_strip->clear(led_strip, 100);

    return ESP_OK;
}

esp_err_t light_loading() {
    return led_strip->set_color_all(led_strip, 200, (color_rgb_t){0, 0, 20});
}

esp_err_t light_error() {
    return led_strip->set_color_all(led_strip, 200, (color_rgb_t){20, 0, 0});
}

esp_err_t light_dim_level(float level) {
    esp_err_t ret = ESP_OK;
    
    if(level > 1 || level < 0) {
        return ESP_ERR_INVALID_ARG;
    }

    current_color = color_for_level(level);
    ESP_LOGD(TAG, "Current color for level: %.3f, %d, %d, %d", level, current_color.r, current_color.g, current_color.b);

    if(!COLORS_EQUAL(current_color, last_color)) {
        ret = led_strip->set_color_all(led_strip, 200, current_color);
        last_color = current_color;
    }

    return ret;
}
