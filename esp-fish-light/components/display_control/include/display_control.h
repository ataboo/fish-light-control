#pragma once

#include <time.h>
#include "esp_err.h"
#include "fish_light_common.h"

#define TEMP_LABEL_MAX_STR_LEN 8

typedef struct {
    char* temp_label;
    temp_warn_t warning;
    float temp;
} temp_disp_t;

typedef struct {
    struct tm* time;
    float light_level;
    uint8_t temp_count;
    temp_disp_t* temp_data;
} display_values_t;

esp_err_t display_control_init();

esp_err_t display_control_update(display_values_t* values);
