#include <stdio.h>
#include <esp_log.h>
#include <driver/ledc.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <led_strip.h>
#include <esp_zigbee_core.h>
#include <zboss_api.h>
#include <zcl/zb_zcl_basic.h>
#include <zcl/esp_zigbee_zcl_basic.h>
#include <ha/esp_zigbee_ha_standard.h>
#include <nvs_flash.h>

static const char *TAG = "CITELAO_ESP32_SWITCH";

// It's written on the ESP32 board :)
static const uint8_t LED_GPIO_PIN = 8;

// BOOT button, via switch_driver.h
// Everyone says is GPIO0, but it's GPIO9.
static const uint8_t SWITCH_GPIO_PIN = GPIO_NUM_9;

static led_strip_handle_t led_strip = NULL;
static bool isPressed = false;

#define ABORT_IF_FALSE(cond, err, tag, fmt, ...) \
    do { \
        if (!(cond)) { \
            ESP_LOGE(tag, fmt, ##__VA_ARGS__); \
            abort(); \
        } \
    } while (0)

void blink(void *arg)
{
    static bool led_state = false;
    ESP_LOGI(TAG, "Blinking %s", led_state ? "on" : "off");

    led_state = !led_state;
    int g = !led_state && isPressed ? 255 : 0;
    ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, led_state ? 255 : 0, g, 0));
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}

static void IRAM_ATTR switch_pressed(void *arg)
{
    isPressed = !isPressed;

    // Stack overflows in an ISR handler because of the stack maxes.
    // ESP_LOGI(TAG, "Switch %s", isPressed ? "pressed" : "released");
}

// Name is required by the ESP Zigbee stack.
void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    // TODO
}

static void zigbee_task(void* params)
{
    esp_zb_cfg_t zb_nwk_cfg = {
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_COORDINATOR,
        .install_code_policy = false,
        .nwk_cfg = {
            .zczr_cfg = {
                .max_children = 10,
            }
        }
    };
    esp_zb_init(&zb_nwk_cfg);

    esp_zb_color_dimmable_switch_cfg_t switch_cfg = {
        .basic_cfg = {
            .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
            .power_source = ZB_ZCL_BASIC_POWER_SOURCE_BATTERY,
        },
        .identify_cfg = {
            .identify_time = ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE,
        },
    };
    uint8_t endpoint_id = 1;
    esp_zb_ep_list_t* dimm_switch_ep = esp_zb_color_dimmable_switch_ep_create(endpoint_id, &switch_cfg);

    // https://github.com/espressif/esp-idf/blob/master/examples/zigbee/common/zcl_utility/src/zcl_utility.c
    esp_zb_cluster_list_t* cluster_list = esp_zb_ep_list_get_ep(dimm_switch_ep, endpoint_id);
    ABORT_IF_FALSE(cluster_list, ESP_ERR_INVALID_ARG, TAG, "Failed to find endpoint id: %d in list: %p", endpoint_id, dimm_switch_ep);
    esp_zb_attribute_list_t* basic_cluster = esp_zb_cluster_list_get_cluster(cluster_list, ESP_ZB_ZCL_CLUSTER_ID_BASIC, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    ABORT_IF_FALSE(basic_cluster, ESP_ERR_INVALID_ARG, TAG, "Failed to find basic cluster in endpoint: %d", endpoint_id);
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, "Citelao"));
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, "ESP32 Switch"));

    ESP_ERROR_CHECK(esp_zb_device_register(dimm_switch_ep));
    ESP_ERROR_CHECK(esp_zb_set_primary_network_channel_set((1l << 13)));
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_stack_main_loop();
}

void old_loop(void* params)
{
    // Configure the BOOT button as input
    // https://github.com/espressif/esp-zigbee-sdk/blob/main/examples/common/switch_driver/src/switch_driver.c
    uint64_t mask = 0;
    mask |= 1ULL << SWITCH_GPIO_PIN;
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = mask,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    ESP_LOGI(TAG, "Configuring switch on GPIO %d", SWITCH_GPIO_PIN);

    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1));
    ESP_ERROR_CHECK(gpio_isr_handler_add(SWITCH_GPIO_PIN, &switch_pressed, NULL));

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

    ESP_LOGI(TAG, "Configuring LED on GPIO %d", LED_GPIO_PIN);

    while (1)
    {
        blink(NULL);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Hello world!");

    esp_zb_platform_config_t config = {
        .host_config = {
            .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE,
        },
        .radio_config = {
            .radio_mode = ZB_RADIO_MODE_NATIVE,
        }
    };
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));
    xTaskCreate(zigbee_task, "zigbee_task", 4096, NULL, 5, NULL);
    xTaskCreate(old_loop, "old_loop", 4096, NULL, 5, NULL);
}