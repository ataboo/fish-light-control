idf_component_register(SRCS "scheduler.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES light_control wifi_time display_control temp_control
                       REQUIRES )