idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver
                    REQUIRES esp_driver_ledc
                    REQUIRES esp_timer)