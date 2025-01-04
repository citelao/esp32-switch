#include <stdio.h>
#include <esp_log.h>
#include <driver/ledc.h>
#include <esp_timer.h>

static const char *TAG = "CITELAO_ESP32_SWITCH";

// It's written on the ESP32 board :)
static const uint8_t LED_GPIO_PIN = 8;

void blink(void *arg)
{
    static bool led_state = false;
    ESP_LOGI(TAG, "Blinking %s", led_state ? "on" : "off");

    led_state = !led_state;
    gpio_set_level(LED_GPIO_PIN, led_state);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Hello world!");

    gpio_reset_pin(LED_GPIO_PIN);
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);

    esp_timer_create_args_t periodic_timer_args = {
        .callback = &blink,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "periodic_timer"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000000)); // 1 second
}