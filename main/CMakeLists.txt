idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver
                    REQUIRES esp_driver_ledc
                    REQUIRES esp_driver_gpio
                    REQUIRES esp_timer
                    REQUIRES nvs_flash)