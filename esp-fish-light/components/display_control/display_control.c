#include <math.h>
#include "display_control.h"
#include "ssd-1306-i2c.h"
// #define LOCAL_LOG_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "ata_mono12.h"
#include "ata_monobold12.h"
#include "ata_monobold16.h"

static const char* TAG = "DISPLAY_CTRL";
static canvas_grid_handle canvas;

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
    fontbold12 = CREATE_ATA_MONOBOLD12();
    fontbold16 = CREATE_ATA_MONOBOLD16();

    sunrise_start_mins = ten_base_time_to_mins(CONFIG_SUNRISE_START);
    sunrise_end_mins = ten_base_time_to_mins(CONFIG_SUNRISE_END);
    sunset_start_mins = ten_base_time_to_mins(CONFIG_SUNSET_START);
    sunset_end_mins = ten_base_time_to_mins(CONFIG_SUNSET_END);

    return ESP_OK;
}

static int x_pos_for_time(int minute) {
    float t = (float)minute / (60 * 24);

    return (int)round(t * 80) + 48;
}

esp_err_t display_control_update(display_values_t* values) {
    blink_on = !blink_on;
    canvas_font_handle temp_font;
    temp_warn_t temp_warn;
    char* temp_label;
    char str_buffer[16];
    sprintf(str_buffer, "%d:%d", values->time->tm_hour, values->time->tm_min);


    RETURN_IF_NOT_OK(clear_canvas_grid(canvas), "clear_canvas_grid");
    RETURN_IF_NOT_OK(canvas_draw_rect(canvas, (canvas_point_t){x_pos_for_time(0), 13}, (canvas_point_t){128, 14}, true), "canvas_draw_rect");
    // RETURN_IF_NOT_OK(canvas_draw_rect(canvas, (canvas_point_t){x_pos_for_time(SUNRISE_HOUR, SUNRISE_MIN), 2}, (canvas_point_t){x_pos_for_time(SUNSET_HOUR, SUNSET_MIN), 3}, true), "canvas_draw_rect");

    RETURN_IF_NOT_OK(canvas_draw_line(canvas, (canvas_point_t){x_pos_for_time(sunrise_start_mins), 14}, (canvas_point_t){x_pos_for_time(sunrise_end_mins), 2}), "canvas_draw_line");
    RETURN_IF_NOT_OK(canvas_draw_line(canvas, (canvas_point_t){x_pos_for_time(sunrise_end_mins), 2}, (canvas_point_t){x_pos_for_time(sunset_start_mins), 2}), "canvas_draw_line");
    RETURN_IF_NOT_OK(canvas_draw_line(canvas, (canvas_point_t){x_pos_for_time(sunset_start_mins), 2}, (canvas_point_t){x_pos_for_time(sunset_end_mins), 14}), "canvas_draw_line");

    int now_x = x_pos_for_time(values->time->tm_hour * 60 + values->time->tm_min);
    RETURN_IF_NOT_OK(canvas_draw_line(canvas, (canvas_point_t){now_x, 2}, (canvas_point_t){now_x, 14}), "canvas_draw_rect");
    
    RETURN_IF_NOT_OK(canvas_draw_text(canvas, str_buffer, (canvas_point_t){2, 0}, fontbold16), "canvas_draw_text");

    for(int i=0; i<values->temp_count; i++) {
        temp_warn = values->temp_warns[i];
        temp_font = (temp_warn == TEMP_NOMINAL || ((temp_warn == TEMP_HOT || temp_warn == TEMP_COLD) && !blink_on)) ? font12 : fontbold12;
        temp_label = values->temp_labels + i * TEMP_LABEL_MAX_STR_LEN;

        switch (temp_warn)
        {
        case TEMP_NOMINAL:
            sprintf(str_buffer, "%-9s %.1f", temp_label, values->temps[i]);
            break;
        case TEMP_WARM:
        case TEMP_COOL:
            sprintf(str_buffer, "%-9s %.1f", temp_label, values->temps[i]);
            break;
        case TEMP_HOT:
        case TEMP_COLD:
            sprintf(str_buffer, "%-9s =%.1f=", temp_label, values->temps[i]);
            break;
        default:
            ESP_LOGE(TAG, "Unsupported temp warning: %d", temp_warn);
            break;
        }

        RETURN_IF_NOT_OK(canvas_draw_text(canvas, str_buffer, (canvas_point_t){2, i*12 + 16}, temp_font), "canvas_draw_text");
    }


    // RETURN_IF_NOT_OK(dump_canvas(canvas), "dump_canvas");

    RETURN_IF_NOT_OK(draw_canvas_grid(canvas), "draw_canvas_grid");

    return ESP_OK;
}
