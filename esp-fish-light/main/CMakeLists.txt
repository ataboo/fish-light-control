idf_component_register(SRCS "esp_fish_light.c"
                    INCLUDE_DIRS "."
                    REQUIRES wifi_time light_control scheduler)