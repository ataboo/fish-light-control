
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

static int sunrise_start_mins = 0;
static int sunrise_end_mins = 0;
static int sunset_start_mins = 0;
static int sunset_end_mins = 0;

static const char *TAG = "LIGHT_CONTROL";
static led_strip_t* led_strip;
static struct tm timeinfo;
static color_rgb_t last_color = {-1, -1, -1};
static color_rgb_t current_color;

#define RMT_TX_CHANNEL RMT_CHANNEL_0
#define RMT_CLOCK_DIV 1


static int hm_to_mins(int hour_mins) {
    return (hour_mins / 100 * 60) + (hour_mins % 100);
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

static color_rgb_t color_for_mins(int mins) {

    float level = level_for_time(mins);

    uint8_t r = (uint8_t)((CONFIG_MIDDAY_COLOR>>16 & 0xFF) * level);
    uint8_t g = (uint8_t)((CONFIG_MIDDAY_COLOR>>8 & 0xFF) * level);
    uint8_t b = (uint8_t)((CONFIG_MIDDAY_COLOR & 0xFF) * level);
    
    return (color_rgb_t){r, g, b};
}

esp_err_t light_control_init() {
    sunrise_start_mins = hm_to_mins(CONFIG_SUNRISE_START);
    sunrise_end_mins = hm_to_mins(CONFIG_SUNRISE_END);
    sunset_start_mins = hm_to_mins(CONFIG_SUNSET_START);
    sunset_end_mins = hm_to_mins(CONFIG_SUNSET_END);

    ESP_LOGD(TAG, "Sunrise: %d mins - %d mins, Sunset: %d mins - %d mins", sunrise_start_mins, sunrise_end_mins, sunset_start_mins, sunset_end_mins);

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
    led_strip->set_color(led_strip, 0, (color_rgb_t){100, 0, 0});
    led_strip->set_color(led_strip, 1, (color_rgb_t){0, 100, 0});
    led_strip->set_color(led_strip, 2, (color_rgb_t){0, 0, 100});
    led_strip->set_color(led_strip, 3, (color_rgb_t){0, 0, 0});

    return led_strip->commit(led_strip, 100);
}

esp_err_t light_error() {
    return ESP_FAIL;
}

esp_err_t light_dim_for_hourmin(int hours, int minutes) {
    esp_err_t ret = ESP_OK;
    current_color = color_for_mins(hours * 60 + minutes);
    ESP_LOGD(TAG, "Current color for time %d, %d, %d", current_color.r, current_color.g, current_color.b);

    if(!COLORS_EQUAL(current_color, last_color)) {
        for(int i=0; i<CONFIG_LED_COUNT; i++) {
            ret = led_strip->set_color(led_strip, i, current_color);
            if(ret != ESP_OK) {
                return ret;
            }
        }
        ret = led_strip->commit(led_strip, 200);
        last_color = current_color;

    }

    return ret;
}

esp_err_t light_dim_for_time(time_t dim_time) {
    timeinfo = (struct tm){0};

    localtime_r(&dim_time, &timeinfo);

    return light_dim_for_hourmin(timeinfo.tm_hour, timeinfo.tm_min);
}