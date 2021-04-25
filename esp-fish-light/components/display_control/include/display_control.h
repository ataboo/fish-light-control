#pragma once

#include <time.h>
#include "esp_err.h"

typedef struct {
    struct tm* time;
    float light_level;
    float* temps;
    uint8_t temp_count;
} display_values_t;

esp_err_t display_control_init();

esp_err_t display_control_update(display_values_t* values);
