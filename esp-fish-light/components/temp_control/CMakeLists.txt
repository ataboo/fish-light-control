idf_component_register(SRCS "temp_control.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES
                       REQUIRES esp32-ds18b20 esp32-owb fish-light-common)