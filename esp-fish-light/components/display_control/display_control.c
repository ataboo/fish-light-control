#include <math.h>
#include "display_control.h"
#include "ssd-1306-i2c.h"
// #define LOCAL_LOG_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "ata_mono12.h"
#include "ata_mono16.h"
#include "ata_monobold12.h"
#include "ata_monobold16.h"
#include "fish_light_common.h"

static const char* TAG = "DISPLAY_CTRL";
static canvas_grid_handle canvas;

static canvas_font_handle font16;
static canvas_font_handle fontbold16;
static canvas_font_handle font12;
static canvas_font_handle fontbold12;
static bool blink_on;
static int sunrise_start_mins;
static int sunrise_end_mins;
static int sunset_start_mins;
static int sunset_end_mins;


int static ten_base_time_to_mins(int ten_base) {
    return ten_base - (ten_base / 100) * 40;
}

esp_err_t display_control_init() {
    canvas = init_canvas_grid();
    RETURN_IF_NOT_OK(init_lcd_i2c(), "init_lcd_i2c");

    font12 = CREATE_ATA_MONO12();
    font16 = CREATE_ATA_MONO16();
    fontbold12 = CREATE_ATA_MONOBOLD12();
    fontbold16 = CREATE_ATA_MONOBOLD16();

    sunrise_start_mins = ten_base_time_to_mins(CONFIG_SUNRISE_START);
    sunrise_end_mins = ten_base_time_to_mins(CONFIG_SUNRISE_END);
    sunset_start_mins = ten_base_time_to_mins(CONFIG_SUNSET_START);
    sunset_end_mins = ten_base_time_to_mins(CONFIG_SUNSET_END);

    RETURN_IF_NOT_OK(clear_canvas_grid(canvas), "clear_canvas_grid");
    canvas_draw_text(canvas, "Loading...", (canvas_point_t){0, 0}, font16);
    RETURN_IF_NOT_OK(draw_canvas_grid(canvas), "draw_canvas_grid");

    return ESP_OK;
}

static int x_pos_for_time(int minute) {
    float t = (float)minute / (60 * 24);

    return (int)round(t * 80) + 48;
}

esp_err_t display_control_fatal_error(char* messages, int count) {
    RETURN_IF_NOT_OK(clear_canvas_grid(canvas), "clear_canvas_grid");

    canvas_draw_text(canvas, "Fatal Error", (canvas_point_t){0, 0}, fontbold16);
    for(int i=0; i<count; i++) {
        canvas_draw_text(canvas, messages + i * ERR_MAX_STR_LEN, (canvas_point_t){0, 16}, font12);
    }
    RETURN_IF_NOT_OK(draw_canvas_grid(canvas), "draw_canvas_grid");

    return ESP_OK;
}

esp_err_t display_control_update(display_values_t* values) {
    blink_on = !blink_on;
    canvas_font_handle temp_font;
    temp_warn_t temp_warn;
    char* temp_label;
    char str_buffer[16];
    sprintf(str_buffer, "%.2d:%.2d", values->time->tm_hour, values->time->tm_min);

    RETURN_IF_NOT_OK(clear_canvas_grid(canvas), "clear_canvas_grid");
    RETURN_IF_NOT_OK(canvas_draw_rect(canvas, (canvas_point_t){x_pos_for_time(0), 13}, (canvas_point_t){128, 14}, true), "canvas_draw_rect");
  
    RETURN_IF_NOT_OK(canvas_draw_line(canvas, (canvas_point_t){x_pos_for_time(sunrise_start_mins), 14}, (canvas_point_t){x_pos_for_time(sunrise_end_mins), 2}), "canvas_draw_line");
    RETURN_IF_NOT_OK(canvas_draw_line(canvas, (canvas_point_t){x_pos_for_time(sunrise_end_mins), 2}, (canvas_point_t){x_pos_for_time(sunset_start_mins), 2}), "canvas_draw_line");
    RETURN_IF_NOT_OK(canvas_draw_line(canvas, (canvas_point_t){x_pos_for_time(sunset_start_mins), 2}, (canvas_point_t){x_pos_for_time(sunset_end_mins), 14}), "canvas_draw_line");

    int now_x = x_pos_for_time(values->time->tm_hour * 60 + values->time->tm_min);
    RETURN_IF_NOT_OK(canvas_draw_line(canvas, (canvas_point_t){now_x, 2}, (canvas_point_t){now_x, 14}), "canvas_draw_rect");
    RETURN_IF_NOT_OK(canvas_draw_text(canvas, str_buffer, (canvas_point_t){2, 0}, fontbold16), "canvas_draw_text");
    
    for(int i=0; i<values->temp_count; i++) {
        temp_warn = values->temp_data[i].warning;
        temp_font = (temp_warn == TEMP_NOMINAL || ((temp_warn == TEMP_HOT || temp_warn == TEMP_COLD) && !blink_on)) ? font12 : fontbold12;
        temp_label = values->temp_data[i].temp_label;

        switch (temp_warn)
        {
        case TEMP_NOMINAL:
            sprintf(str_buffer, "%-9s %.1f", temp_label, values->temp_data[i].temp);
            break;
        case TEMP_WARM:
        case TEMP_COOL:
            sprintf(str_buffer, "%-9s %.1f", temp_label, values->temp_data[i].temp);
            break;
        case TEMP_HOT:
        case TEMP_COLD:
            sprintf(str_buffer, "%-9s =%.1f=", temp_label, values->temp_data[i].temp);
            break;
        default:
            ESP_LOGE(TAG, "Unsupported temp warning: %d", temp_warn);
            break;
        }

        RETURN_IF_NOT_OK(canvas_draw_text(canvas, str_buffer, (canvas_point_t){2, i*12 + 16}, temp_font), "canvas_draw_text");
        RETURN_IF_NOT_OK(canvas_draw_eq_tri(canvas, (canvas_point_t){90, i*12 + 16 + 1}, 8, 8, false), "canvas_draw_eq_tri");
        RETURN_IF_NOT_OK(canvas_draw_eq_tri(canvas, (canvas_point_t){110, i*12 + 16 + 1}, 8, 8, true), "canvas_draw_eq_tri");

    }

    RETURN_IF_NOT_OK(draw_canvas_grid(canvas), "draw_canvas_grid");

    return ESP_OK;
}