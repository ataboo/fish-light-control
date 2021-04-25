#pragma once

#include <time.h>
#include "esp_err.h"

#define TEMP_LABEL_MAX_STR_LEN 8

typedef enum {
    TEMP_NOMINAL,
    TEMP_WARM,
    TEMP_HOT,
    TEMP_COOL,
    TEMP_COLD
} temp_warn_t;

typedef struct {
    struct tm* time;
    float light_level;
    float* temps;
    char* temp_labels;
    temp_warn_t* temp_warns;
    uint8_t temp_count;
} display_values_t;

esp_err_t display_control_init();

esp_err_t display_control_update(display_values_t* values);
