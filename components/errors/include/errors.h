// Macros that are missing from esp_err.h & esp_check.h

#define ABORT_IF_FALSE(cond, err, tag, fmt, ...) \
    do { \
        if (!(cond)) { \
            ESP_LOGE(tag, fmt, ##__VA_ARGS__); \
            abort(); \
        } \
    } while (0)

#define RETURN_IF_FALSE(cond, err, tag, fmt, ...) \
    do { \
        if (!(cond)) { \
            ESP_LOGE(tag, fmt, ##__VA_ARGS__); \
            return err; \
        } \
    } while (0)
