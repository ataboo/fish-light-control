#pragma once

#include "light_control.h"
#include "esp_err.h"
#include "driver/rmt.h"

#define COLORS_EQUAL(c1, c2)                     \
({                                               \
   c1.r == c2.r && c1.g == c2.g && c1.b == c2.b; \
})

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_rgb_t;

typedef struct {
    uint32_t led_count;
    rmt_channel_t rmt_channel;
} led_strip_config_t;

typedef struct led_strip_s led_strip_t;

struct led_strip_s {
    esp_err_t (*set_color)(led_strip_t* strip, uint32_t index, color_rgb_t color);
    esp_err_t (*commit)(led_strip_t *strip, uint32_t timeout_ms);
    esp_err_t (*clear)(led_strip_t *strip, uint32_t timeout_ms);
    esp_err_t (*free)(led_strip_t *strip);
    esp_err_t (*set_color_all)(led_strip_t* strip, uint32_t index, color_rgb_t color);
};

#define LED_STRIP_DEFAULT_CONFIG(m_led_count, m_rmt_channel) \
    {                                                        \
        .led_count = m_led_count,                            \
        .rmt_channel = m_rmt_channel,                        \
    }

led_strip_t* ws2812_init(const led_strip_config_t* config);
