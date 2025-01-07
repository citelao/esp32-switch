#include <stdio.h>
#include <esp_log.h>
#include <driver/ledc.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <led_strip.h>

static const char *TAG = "CITELAO_ESP32_SWITCH";

// It's written on the ESP32 board :)
static const uint8_t LED_GPIO_PIN = 8;

// BOOT button, via switch_driver.h
static const uint8_t SWITCH_GPIO_PIN = GPIO_NUM_9;

static led_strip_handle_t led_strip = NULL;
static bool isPressed = false;

void blink(void *arg)
{
    static bool led_state = false;
    ESP_LOGI(TAG, "Blinking %s", led_state ? "on" : "off");

    led_state = !led_state;
    ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, led_state ? 255 : 0, isPressed ? 255 : 0, 0));
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}

static void IRAM_ATTR switch_pressed(void *arg)
{
    isPressed = !isPressed;
    // ESP_LOGI(TAG, "Switch %s", isPressed ? "pressed" : "released");
}

void app_main(void)
{
    ESP_LOGI(TAG, "Hello world!");

    // Configure the BOOT button as input
    // https://github.com/espressif/esp-zigbee-sdk/blob/main/examples/common/switch_driver/src/switch_driver.c
    uint64_t mask = 0;
    mask |= 1ULL << SWITCH_GPIO_PIN;
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_LOW_LEVEL,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << LED_GPIO_PIN,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    ESP_LOGI(TAG, "Configuring switch on GPIO %d", SWITCH_GPIO_PIN);

    const int intr_alloc_flags = 0; // default interrupt flag?
    ESP_ERROR_CHECK(gpio_install_isr_service(intr_alloc_flags));
    ESP_ERROR_CHECK(gpio_isr_handler_add(SWITCH_GPIO_PIN, &switch_pressed, NULL));

    ESP_LOGI(TAG, "Configuring LED on GPIO %d", LED_GPIO_PIN);

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