#pragma once

#include <time.h>
#include "esp_err.h"
#include "ds18b20.h"
#include "fish_light_common.h"

#define TEMP_LABEL_MAX_STR_LEN 8

#define CREATE_TEMP_CONTROL_DATA() (  \
    (temp_data_t*)malloc(sizeof(temp_data_t) * CONFIG_TEMP_COUNT)  \
)

typedef enum {
    TEMP_READ_FAIL = -1,
    TEMP_READ_OK = 0,
    TEMP_READ_ERR_DEVICE,
    TEMP_READ_ERR_CRC,
    TEMP_READ_ERR_OWB,
    TEMP_READ_ERR_NULL,
    TEMP_READ_ERR_NOT_FOUND
} temp_err_t;

typedef struct {
    float temp;
    temp_warn_t warn;
    temp_err_t err;
} temp_data_t;

esp_err_t temp_control_init();

esp_err_t temp_control_update(temp_data_t* data);

esp_err_t temp_label_for_idx(int idx, char* label);
