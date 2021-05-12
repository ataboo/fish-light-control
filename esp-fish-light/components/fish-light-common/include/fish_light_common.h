#pragma once

#define RETURN_IF_NOT_OK(action, message) ({ \
    esp_err_t ret = action;                  \
    if(ret != ESP_OK) {                      \
        ESP_LOGE(TAG, message);              \
        return ret;                          \
    }                                        \
})

typedef enum {
    TEMP_NOMINAL = 0,
    TEMP_WARM,
    TEMP_COOL,
    TEMP_HOT,
    TEMP_COLD
} temp_warn_t;