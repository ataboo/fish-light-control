#pragma once

#include "esp_system.h"

esp_err_t light_control_init();

esp_err_t light_loading();

esp_err_t light_error();

esp_err_t light_dim_for_time(time_t dim_time);

esp_err_t light_dim_for_hourmin(int hours, int mins);
