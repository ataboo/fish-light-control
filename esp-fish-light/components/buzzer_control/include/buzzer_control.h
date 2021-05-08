#pragma once

#include "esp_err.h"

typedef struct {
    uint16_t frequency;
    uint16_t duration;
} buzzer_keyframe_t;

typedef struct {
    int frame_count;
    buzzer_keyframe_t* key_frames;
} buzzer_pattern_t;

esp_err_t buzzer_control_init();

esp_err_t buzzer_control_play_pattern(buzzer_pattern_t* pattern);