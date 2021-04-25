#include <math.h>
#include "display_control.h"
#include "ssd-1306-i2c.h"
// #define LOCAL_LOG_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "ata_mono12.h"
#include "ata_mono16.h"

#define SUNRISE_HOUR (CONFIG_SUNRISE_START / 100)
#define SUNRISE_MIN (CONFIG_SUNRISE_START - (SUNRISE_HOUR * 100))

#define SUNSET_HOUR (CONFIG_SUNSET_START / 100)
#define SUNSET_MIN (CONFIG_SUNSET_START - (SUNSET_HOUR * 100))

static const char* TAG = "DISPLAY_CTRL";
static canvas_grid_handle canvas;

static canvas_font_handle font16;
static canvas_font_handle font12;


esp_err_t display_control_init() {
    canvas = init_canvas_grid();
    // RETURN_IF_NOT_OK(init_lcd_i2c(), "init_lcd_i2c");

    font12 = CREATE_ATA_MONO12();
    font16 = CREATE_ATA_MONO16();

    return ESP_OK;
}

static int x_pos_for_time(int hour, int minute) {
    float t = (float)(hour * 60 + minute) / (60 * 24);

    ESP_LOGE(TAG, "%d:%d, %f", hour, minute, t);

    return (int)round(t * 80) + 48;
}

esp_err_t display_control_update(display_values_t* values) {
    char str_buffer[8];
    sprintf(str_buffer, "%d:%d", values->time->tm_hour, values->time->tm_min);


    RETURN_IF_NOT_OK(clear_canvas_grid(canvas), "clear_canvas_grid");
    RETURN_IF_NOT_OK(canvas_draw_rect(canvas, (canvas_point_t){x_pos_for_time(0, 0), 13}, (canvas_point_t){128, 14}, true), "canvas_draw_rect");
    RETURN_IF_NOT_OK(canvas_draw_rect(canvas, (canvas_point_t){x_pos_for_time(SUNRISE_HOUR, SUNRISE_MIN), 2}, (canvas_point_t){x_pos_for_time(SUNSET_HOUR, SUNSET_MIN), 3}, true), "canvas_draw_rect");
    RETURN_IF_NOT_OK(canvas_draw_line(canvas, (canvas_point_t){x_pos_for_time(values->time->tm_hour, values->time->tm_min), 2}, (canvas_point_t){x_pos_for_time(values->time->tm_hour, values->time->tm_min), 14}), "canvas_draw_rect");
    
    RETURN_IF_NOT_OK(canvas_draw_text(canvas, str_buffer, (canvas_point_t){2, 0}, font16), "canvas_draw_text");


    RETURN_IF_NOT_OK(dump_canvas(canvas), "dump_canvas");

    return ESP_OK;
}
