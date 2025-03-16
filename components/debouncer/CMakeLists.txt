idf_component_register(SRCS "debouncer.c"
                       INCLUDE_DIRS "include"
                       REQUIRES driver
                       REQUIRES esp_driver_gpio)