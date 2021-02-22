#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include "ws2812.h"
#include "light_control.h"
#include "driver/rmt.h"
#include "esp_system.h"
#include "esp_log.h"

static const char* TAG = "WS2812";

#define WS2812_T0H_NS 350
#define WS2812_T1H_NS 900
#define WS2812_T0L_NS 900
#define WS2812_T1L_NS 350

static uint32_t t0h_ticks = 0;
static uint32_t t1h_ticks = 0;
static uint32_t t0l_ticks = 0;
static uint32_t t1l_ticks = 0;

typedef struct {
    uint8_t g;
    uint8_t r;
    uint8_t b;
} color_grb_t;

typedef struct {
    led_strip_t parent;
    rmt_channel_t rmt_channel;
    uint32_t light_count;
    color_grb_t* colors;
} ws2812_strip_t;

static void IRAM_ATTR rmt_uint8_adapter(const void *src, rmt_item32_t *dest, size_t src_size, size_t wanted_num, size_t *translated_size, size_t *item_num) {
    if (src == NULL || dest == NULL) {
        *translated_size = 0;
        *item_num = 0;
        return;
    }

    const rmt_item32_t bit0 = {{{t0h_ticks, 1, t0l_ticks, 0}}};
    const rmt_item32_t bit1 = {{{t1h_ticks, 1, t1l_ticks, 0}}};
    size_t size = 0;
    size_t num = 0;
    uint8_t *psrc = (uint8_t *)src;
    rmt_item32_t *pdest = dest;
    while (size < src_size && num < wanted_num) {
        for (int i = 0; i < 8; i++) {
            // MSB first
            if (*psrc & (1 << (7 - i))) {
                pdest->val =  bit1.val;
            } else {
                pdest->val =  bit0.val;
            }
            num++;
            pdest++;
        }
        size++;
        psrc++;
    }
    *translated_size = size;
    *item_num = num;
}

static esp_err_t ws2812_set_color(led_strip_t* strip, uint32_t index, color_rgb_t color) {
    ws2812_strip_t* ws2812 = __containerof(strip, ws2812_strip_t, parent);
    
    if (index >= ws2812->light_count) {
        ESP_LOGE(TAG, "ws2812_set_color, index out of range: %d", index);
        return ESP_FAIL;
    }

    ws2812->colors[index].g = color.g;
    ws2812->colors[index].r = color.r;
    ws2812->colors[index].b = color.b;

    return ESP_OK;
}

static esp_err_t ws2812_commit(led_strip_t* strip, uint32_t timeout_ms) {
    ws2812_strip_t* ws2812 = __containerof(strip, ws2812_strip_t, parent);

    esp_err_t ret = rmt_write_sample(ws2812->rmt_channel, (uint8_t*)ws2812->colors, ws2812->light_count*3, true);
    if(ret != ESP_OK) {
        return ret;
    }

    return rmt_wait_tx_done(ws2812->rmt_channel, pdMS_TO_TICKS(timeout_ms));
}

static esp_err_t ws2812_clear(led_strip_t* strip, uint32_t timeout_ms) {
    ws2812_strip_t* ws2812 = __containerof(strip, ws2812_strip_t, parent);
    
    color_rgb_t clear = {0, 0, 0};
    for(int i=0; i<ws2812->light_count; i++) {
        ws2812_set_color(strip, i, clear);
    }

    return ws2812_commit(strip, timeout_ms);
}

static esp_err_t ws2812_free(led_strip_t* strip) {
    ws2812_strip_t *ws2812 = __containerof(strip, ws2812_strip_t, parent);

    free(ws2812->colors);
    free(ws2812);
    return ESP_OK;
}

led_strip_t* ws2812_init(const led_strip_config_t* config) {
    ws2812_strip_t* ws2812 = malloc(sizeof(ws2812_strip_t));
    ws2812->colors = calloc(config->led_count, sizeof(color_grb_t));

    uint32_t counter_clk_hz = 0;
    ESP_ERROR_CHECK(rmt_get_counter_clock(config->rmt_channel, &counter_clk_hz));

    float ticksPerNanosecond = (float)counter_clk_hz / 1e9;
    t0h_ticks = (uint32_t)(ticksPerNanosecond * WS2812_T0H_NS);
    t1h_ticks = (uint32_t)(ticksPerNanosecond * WS2812_T1H_NS);
    t0l_ticks = (uint32_t)(ticksPerNanosecond * WS2812_T0L_NS);
    t1l_ticks = (uint32_t)(ticksPerNanosecond * WS2812_T1L_NS);

    ESP_LOGI(TAG, "Ticks ns T0H: %d, T1H: %d, T0L: %d, T1L: %d", t0h_ticks, t1h_ticks, t0l_ticks, t1l_ticks);

    ESP_ERROR_CHECK(rmt_translator_init(config->rmt_channel, rmt_uint8_adapter));

    ws2812->light_count = config->led_count;
    ws2812->rmt_channel = config->rmt_channel;

    ws2812->parent.clear = ws2812_clear;
    ws2812->parent.commit = ws2812_commit;
    ws2812->parent.free = ws2812_free;
    ws2812->parent.set_color = ws2812_set_color;

    return &ws2812->parent;
}
