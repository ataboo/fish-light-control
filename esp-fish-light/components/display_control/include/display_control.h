#pragma once

#include <time.h>
#include "esp_err.h"

typedef struct {
    time_t time,
    float light_level,
    int16_t temps[4];
} display_values_t;

esp_err_t display_control_init();

esp_err_t display_control_update();
