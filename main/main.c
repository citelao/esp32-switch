#include <stdio.h>
#include <esp_log.h>
#include <driver/ledc.h>
#include <esp_timer.h>
#include <led_strip.h>

static const char *TAG = "CITELAO_ESP32_SWITCH";

// It's written on the ESP32 board :)
static const uint8_t LED_GPIO_PIN = 8;

static led_strip_handle_t led_strip = NULL;

void blink(void *arg)
{
    static bool led_state = false;
    ESP_LOGI(TAG, "Blinking %s", led_state ? "on" : "off");

    led_state = !led_state;
    ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, led_state ? 255 : 0, 0, 0));
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}

void app_main(void)
{
    ESP_LOGI(TAG, "Hello world!");

    // https://components.espressif.com/components/espressif/led_strip
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO_PIN,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false,
        }
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz, per docs.
        .mem_block_symbols = 64, // 4 bytes, per docs.
        .flags = {
            .with_dma = false, // Only 1 LED. No need.
        }
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

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