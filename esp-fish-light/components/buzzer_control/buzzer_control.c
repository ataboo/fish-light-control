#include "buzzer_control.h"
#include "driver/mcpwm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#define LOCAL_LOG_LEVEL ESP_LOG_INFO
#include "esp_log.h"
#define PWM_UNIT MCPWM_UNIT_0
#define PWM_TIMER MCPWM_TIMER_0
#define PWM_GENERATOR MCPWM0A
#define BUZZER_PIN GPIO_NUM_15
#define MAX_KEYFRAME_COUNT 32
#define FRAMEPERIOD_MS 10

typedef struct {
    uint16_t frequency;
    uint16_t end_pos;
} keyframe_internal_t;

static const char* TAG = "BUZZER_CONTROL";

static uint8_t current_keyframe_idx = 0;
static buzzer_pattern_t* current_pattern = NULL;
static TaskHandle_t* buzzer_task_handle;
static uint32_t playback_position = 0;
static keyframe_internal_t internal_keyframes[MAX_KEYFRAME_COUNT];

static void output_keyframe(keyframe_internal_t* keyframe) {
    if (keyframe != NULL && keyframe->frequency > 0) {
        mcpwm_set_frequency(PWM_UNIT, PWM_TIMER, keyframe->frequency);
        mcpwm_set_duty(PWM_UNIT, PWM_TIMER, PWM_GENERATOR, 50);
    } else {
        mcpwm_set_frequency(PWM_UNIT, PWM_TIMER, 400);
        mcpwm_set_duty(PWM_UNIT, PWM_TIMER, PWM_GENERATOR, 0);
    }
}

static void update_buzzer_frame() {
    if(current_pattern != NULL) {
        keyframe_internal_t current_keyframe = internal_keyframes[current_keyframe_idx];
        if(playback_position > current_keyframe.end_pos) {
            current_keyframe_idx = (current_keyframe_idx + 1) % current_pattern->frame_count;
            if(current_keyframe_idx == 0) {
                playback_position = 0;
            }

            ESP_LOGI(TAG, "Current idx: %d", current_keyframe_idx);

            current_keyframe = internal_keyframes[current_keyframe_idx];

            output_keyframe(&current_keyframe);
        }

        playback_position++;
    }
}

static void buzzer_play_task(void* args) {
    uint32_t notification = 0;

    while(true) {
        update_buzzer_frame();    
        if (xTaskNotifyWait(1, 1, &notification, FRAMEPERIOD_MS/portTICK_PERIOD_MS) && notification) {
            break;
        }
    }

    vTaskDelete(NULL);
}

esp_err_t buzzer_control_init() {
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 800;
    pwm_config.cmpr_a = 50.0;
    pwm_config.cmpr_b = 50.0;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    esp_err_t ret = mcpwm_init(PWM_UNIT, PWM_TIMER, &pwm_config);
    if (ret != ESP_OK) {
        return ret;
    }

    mcpwm_gpio_init(PWM_UNIT, PWM_GENERATOR, BUZZER_PIN);

    mcpwm_start(PWM_UNIT, PWM_TIMER);

    // xTaskCreate(buzzer_play_task, "Buzzer Task", 2048, NULL, 5, buzzer_task_handle);

    return ESP_OK;
}

esp_err_t buzzer_control_play_pattern(buzzer_pattern_t* pattern) {
    if (pattern != NULL) {
        playback_position = 0;
        current_keyframe_idx = 0;
        current_pattern = NULL;
        uint32_t play_pos = 0;

        for(int i=0; i<pattern->frame_count; i++) {
            buzzer_keyframe_t frame = pattern->key_frames[i];

            play_pos += frame.duration;
            internal_keyframes[i] = (keyframe_internal_t){
                .end_pos = play_pos,
                .frequency = frame.frequency,
            };
        }

        output_keyframe(&internal_keyframes[0]);
    } else {
        output_keyframe(NULL);
    }
    current_pattern = pattern;

    return ESP_OK;
}

void buzzer_control_deinit() {
    xTaskNotify(buzzer_task_handle, 1, eSetBits);
    mcpwm_stop(PWM_UNIT, PWM_TIMER);

    current_pattern = NULL;
}
