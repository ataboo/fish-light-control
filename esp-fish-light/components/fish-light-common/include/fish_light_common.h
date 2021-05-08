#pragma once

#define TEMP_WARN_IS_ALERT(temp_warn) (temp_warn > TEMP_COOL)


typedef enum {
    TEMP_NOMINAL = 0,
    TEMP_WARM,
    TEMP_COOL,
    TEMP_HOT,
    TEMP_COLD
} temp_warn_t;