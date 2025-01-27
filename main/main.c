#include <stdio.h>
#include <esp_log.h>
#include <driver/ledc.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <led_strip.h>
#include <esp_zigbee_core.h>
#include <esp_zigbee_trace.h>
#include <zboss_api.h>
#include <zcl/zb_zcl_basic.h>
#include <zcl/esp_zigbee_zcl_basic.h>
#include <ha/esp_zigbee_ha_standard.h>
#include <nvs_flash.h>

#if !defined CONFIG_ZB_ENABLED
#error Define ZB_ENABLED in idf.py menuconfig to compile Zigbee source code.
#endif

#if !defined CONFIG_ZB_ZCZR
#error Define ZB_ZCZR in idf.py menuconfig to compile light (Router) source code.
#endif

static const char *TAG = "CITELAO_ESP32_SWITCH";

// It's written on the ESP32 board :)
static const uint8_t LED_GPIO_PIN = 8;

// BOOT button, via switch_driver.h
// Everyone says is GPIO0, but it's GPIO9.
static const uint8_t SWITCH_GPIO_PIN = GPIO_NUM_9;

static led_strip_handle_t led_strip = NULL;
static bool isPressed = false;
static bool isIdentifying = false;

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

static void IRAM_ATTR switch_pressed(void *arg)
{
    isPressed = !isPressed;

    // Stack overflows in an ISR handler because of the stack maxes.
    // https://github.com/espressif/esp-zigbee-sdk/blob/8114916a4c6d1b4587a9fc24d2c85a1396328a28/examples/esp_zigbee_HA_sample/HA_color_dimmable_switch/main/esp_zb_switch.c#L67
    // ESP_LOGI(TAG, "Switch %s", isPressed ? "pressed" : "released");
    ESP_EARLY_LOGI(TAG, "Switch %s", isPressed ? "pressed" : "released");

    // TODO: do through a queue?
    esp_zb_zcl_move_to_level_cmd_t cmd = {
        .address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
        .zcl_basic_cmd = {},
        .level = isPressed ? 0xFF : 0x00,
        .transition_time = 1,
    };
    uint8_t endpoint_id = 1; // TODO: share
    cmd.zcl_basic_cmd.src_endpoint = endpoint_id;
    uint8_t seq_num = esp_zb_zcl_level_move_to_level_with_onoff_cmd_req(&cmd);
    ESP_EARLY_LOGI(TAG, "Move to level command sent (seq_num: %d)", seq_num);
}

static void start_top_level_commissioning(uint8_t mode_mask)
{
    ESP_ERROR_CHECK(esp_zb_bdb_start_top_level_commissioning(mode_mask));
}

// Name is required by the ESP Zigbee stack.
void esp_zb_app_signal_handler(esp_zb_app_signal_t* signal_struct)
{
    uint32_t* p_sg_p = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;
    switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        // Called with no_autostart. We must start commissioning.
        // https://ncsdoc.z6.web.core.windows.net/zboss/3.11.1.177/using_zigbee__z_c_l.html#:~:text=The%20application%20should%20later%20call%20ZBOSS%20commissioning%20initiation%20%E2%80%93%20bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING)%20that%20will%20trigger%20further%20commissioning.
        // https://github.com/espressif/esp-zigbee-sdk/blob/8114916a4c6d1b4587a9fc24d2c85a1396328a28/examples/esp_zigbee_HA_sample/HA_color_dimmable_switch/main/esp_zb_switch.c#L143C10-L143C40
        ESP_LOGI(TAG, "Manually initializing Zigbee stack (starting commissioning)");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;

    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        ESP_LOGI(TAG, "Device started for the first time or rebooted");
        if (err_status == ESP_OK)
        {
            // ESP_LOGI(TAG, "Deferred driver initialization %s", deferred_driver_init() ? "failed" : "successful");
            const bool is_factory_new = esp_zb_bdb_is_factory_new();
            ESP_LOGI(TAG, "Device started up in%s factory-reset mode", is_factory_new ? "" : " non");
            if (is_factory_new)
            {
                ESP_LOGI(TAG, "Start network steering");
                esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
            }
            else
            {
                ESP_LOGI(TAG, "Device rebooted");
                // No need for reconnection, hopefully.
            }
        }
        else
        {
            ESP_LOGW(TAG, "%s failed with status: %s, retrying", esp_zb_zdo_signal_to_string(sig_type),
                     esp_err_to_name(err_status));
            esp_zb_scheduler_alarm((esp_zb_callback_t)start_top_level_commissioning,
                                   ESP_ZB_BDB_MODE_INITIALIZATION, 1000);
        }
        break;

    // case ESP_ZB_BDB_SIGNAL_FORMATION:
    //     ESP_LOGI(TAG, "Network formation started: %s", esp_err_to_name(err_status));
    //     if (err_status == ESP_OK)
    //     {
    //         ESP_LOGI(TAG, "Network formation started");
    //         esp_zb_ieee_addr_t extended_pan_id;
    //         esp_zb_get_extended_pan_id(extended_pan_id);
    //         ESP_LOGI(TAG, "Formed network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d, Short Address: 0x%04hx)",
    //                  extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
    //                  extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
    //                  esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());
    //         esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
    //     }
    //     else
    //     {
    //         ESP_LOGW(TAG, "Network formation failed with status: %s, retrying", esp_err_to_name(err_status));
    //         esp_zb_scheduler_alarm((esp_zb_callback_t)start_top_level_commissioning,
    //                                ESP_ZB_BDB_MODE_NETWORK_FORMATION, 1000);
    //     }
    //     break;

    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK)
        {
            ESP_LOGI(TAG, "Network steering started");
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            ESP_LOGI(TAG, "Formed network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d, Short Address: 0x%04hx)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                     extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                     esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());
        }
        else
        {
            ESP_LOGW(TAG, "Network steering failed with status: %s, retrying", esp_err_to_name(err_status));
            esp_zb_scheduler_alarm((esp_zb_callback_t)start_top_level_commissioning,
                ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        }
        break;

    case ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        esp_zb_zdo_signal_device_annce_params_t* dev_annce_params = (esp_zb_zdo_signal_device_annce_params_t*)esp_zb_app_signal_get_params(p_sg_p);
        ESP_LOGI(TAG, "New device commissioned or rejoined (short: 0x%04hx)", dev_annce_params->device_short_addr);

        // esp_zb_zdo_match_desc_req_param_t cmd_req;
        // cmd_req.dst_nwk_addr = dev_annce_params->device_short_addr;
        // cmd_req.addr_of_interest = dev_annce_params->device_short_addr;
        // esp_zb_zdo_find_color_dimmable_light(&cmd_req, user_find_cb, NULL);
        break;

    case ESP_ZB_NWK_SIGNAL_PERMIT_JOIN_STATUS:
        ESP_LOGI(TAG, "Permit join status: %s", esp_err_to_name(err_status));
        if (err_status == ESP_OK)
        {
            uint8_t* open_seconds = (uint8_t*)esp_zb_app_signal_get_params(p_sg_p);
            uint16_t pan_id = esp_zb_get_pan_id();
            if (*open_seconds)
            {
                ESP_LOGI(TAG, "Network (0x%04hx) is open for %d seconds", pan_id, *open_seconds);
            }
            else
            {
                ESP_LOGW(TAG, "Network (0x%04hx) is closed", pan_id);
            }
        }
        break;

    default:
        ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type,
                 esp_err_to_name(err_status));
        break;
    }
}

static esp_err_t action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message)
{
    switch (callback_id)
    {
    default:
        ESP_LOGW(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
        break;
    }

    return ESP_OK;
}

// In Z2M dev console:
// Endpoint: (whatever endpoint has the identify cluster; prob 0x01)
// Cluster: 0x03
// Command: 0x00
// Payload: {"identifytime": 3}
//      (time in seconds; case-sensitive)
static void identify_cb(uint8_t identify_on)
{
    ESP_LOGI(TAG, "Identify %s", identify_on ? "on" : "off");
    isIdentifying = identify_on;
}

typedef struct zcl_basic_manufacturer_info_s {
    char *manufacturer_name;
    char *model_identifier;
} zcl_basic_manufacturer_info_t;
esp_err_t esp_zcl_utility_add_ep_basic_manufacturer_info(esp_zb_ep_list_t *ep_list, uint8_t endpoint_id, zcl_basic_manufacturer_info_t *info)
{
    esp_err_t ret = ESP_OK;
    esp_zb_cluster_list_t *cluster_list = NULL;
    esp_zb_attribute_list_t *basic_cluster = NULL;

    cluster_list = esp_zb_ep_list_get_ep(ep_list, endpoint_id);
    RETURN_IF_FALSE(cluster_list, ESP_ERR_INVALID_ARG, TAG, "Failed to find endpoint id: %d in list: %p", endpoint_id, ep_list);
    basic_cluster = esp_zb_cluster_list_get_cluster(cluster_list, ESP_ZB_ZCL_CLUSTER_ID_BASIC, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    RETURN_IF_FALSE(basic_cluster, ESP_ERR_INVALID_ARG, TAG, "Failed to find basic cluster in endpoint: %d", endpoint_id);
    RETURN_IF_FALSE((info && info->manufacturer_name), ESP_ERR_INVALID_ARG, TAG, "Invalid manufacturer name");
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, info->manufacturer_name));
    RETURN_IF_FALSE((info && info->model_identifier), ESP_ERR_INVALID_ARG, TAG, "Invalid model identifier");
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, info->model_identifier));
    return ret;
}

static void zigbee_task(void* params)
{
#if defined CONFIG_ZB_DEBUG_MODE && CONFIG_ZB_DEBUG_MODE
    // Enable debugging.
    // Make sure to replace partitions.csv with partitions.debug.csv.
    // https://docs.espressif.com/projects/esp-zigbee-sdk/en/latest/esp32/developing.html#enable-debug-mode-and-trace-logging
    esp_zb_set_trace_level_mask(ESP_ZB_TRACE_LEVEL_INFO, ESP_ZB_TRACE_SUBSYSTEM_COMMON | ESP_ZB_TRACE_SUBSYSTEM_ZCL | ESP_ZB_TRACE_SUBSYSTEM_ZLL);
#endif

    esp_zb_cfg_t zb_nwk_cfg = {
        // TODO: make this an End Device so we can sleep.
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_ROUTER,
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
            // TODO: correctly report battery.
            // .power_source = ZB_ZCL_BASIC_POWER_SOURCE_BATTERY,
            .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE,
        },
        .identify_cfg = {
            .identify_time = ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE,
        },
    };
    uint8_t endpoint_id = 1;
    esp_zb_ep_list_t* dimm_switch_ep = esp_zb_color_dimmable_switch_ep_create(endpoint_id, &switch_cfg);

    // esp_zb_color_dimmable_light_cfg_t light_cfg = {
    //     .basic_cfg = {
    //         .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
    //         .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE,
    //     },
    //     .identify_cfg = {
    //         .identify_time = ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE,
    //     },
    //     .groups_cfg = {
    //         .groups_name_support_id = ESP_ZB_ZCL_GROUPS_NAME_SUPPORT_DEFAULT_VALUE,
    //     },
    //     .scenes_cfg = {
    //         .scenes_count = ESP_ZB_ZCL_SCENES_SCENE_COUNT_DEFAULT_VALUE,
    //         .current_scene = ESP_ZB_ZCL_SCENES_CURRENT_SCENE_DEFAULT_VALUE,
    //         .current_group = ESP_ZB_ZCL_SCENES_CURRENT_GROUP_DEFAULT_VALUE,
    //         .scene_valid = ESP_ZB_ZCL_SCENES_SCENE_VALID_DEFAULT_VALUE,
    //         .name_support = ESP_ZB_ZCL_SCENES_NAME_SUPPORT_DEFAULT_VALUE,
    //     },
    //     .on_off_cfg = {
    //         .on_off = ESP_ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE,
    //     },
    //     .level_cfg = {
    //         .current_level = ESP_ZB_ZCL_LEVEL_CONTROL_CURRENT_LEVEL_DEFAULT_VALUE,
    //     },
    //     .color_cfg = {
    //         .current_x = ESP_ZB_ZCL_COLOR_CONTROL_CURRENT_X_DEF_VALUE,
    //         .current_y = ESP_ZB_ZCL_COLOR_CONTROL_CURRENT_Y_DEF_VALUE,
    //         .color_mode = ESP_ZB_ZCL_COLOR_CONTROL_COLOR_MODE_DEFAULT_VALUE,
    //         .options = ESP_ZB_ZCL_COLOR_CONTROL_OPTIONS_DEFAULT_VALUE,
    //         .enhanced_color_mode = ESP_ZB_ZCL_COLOR_CONTROL_ENHANCED_COLOR_MODE_DEFAULT_VALUE,
    //         .color_capabilities = 0x0008,
    //     },
    // };
    // esp_zb_ep_list_t* dimm_light_ep = esp_zb_color_dimmable_light_ep_create(endpoint_id, &light_cfg);

    // // https://github.com/espressif/esp-idf/blob/master/examples/zigbee/common/zcl_utility/src/zcl_utility.c
    // esp_zb_cluster_list_t* cluster_list = esp_zb_ep_list_get_ep(dimm_switch_ep, endpoint_id);
    // ABORT_IF_FALSE(cluster_list, ESP_ERR_INVALID_ARG, TAG, "Failed to find endpoint id: %d in list: %p", endpoint_id, dimm_switch_ep);
    // esp_zb_attribute_list_t* basic_cluster = esp_zb_cluster_list_get_cluster(cluster_list, ESP_ZB_ZCL_CLUSTER_ID_BASIC, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    // ABORT_IF_FALSE(basic_cluster, ESP_ERR_INVALID_ARG, TAG, "Failed to find basic cluster in endpoint: %d", endpoint_id);

    // // Must be prefixed with string length, excluding null terminator.
    // // https://github.com/espressif/esp-zigbee-sdk/blob/main/examples/esp_zigbee_HA_sample/HA_on_off_light/main/esp_zb_light.h#L27
    // // https://esp32.com/viewtopic.php?t=33143
    // //
    // // See Section 2.6.2.12 "Character String" in Zigbee Cluster Library Specification
    // // https://zigbeealliance.org/wp-content/uploads/2019/12/07-5123-06-zigbee-cluster-library-specification.pdf
    // // via https://github.com/espressif/esp-zigbee-sdk/issues/202#issuecomment-1919594141
    // ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, "\x09" "ESPRESSIF"));
    // ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, "\x07" "esp32c6"));
    zcl_basic_manufacturer_info_t manufacturer_info = {
        .manufacturer_name = "\x07" "citelao",
        .model_identifier = "\x07" "esp32c6",
    };
    ESP_ERROR_CHECK(esp_zcl_utility_add_ep_basic_manufacturer_info(dimm_switch_ep, endpoint_id, &manufacturer_info));
    ESP_ERROR_CHECK(esp_zb_device_register(dimm_switch_ep));
    esp_zb_core_action_handler_register(action_handler);
    esp_zb_identify_notify_handler_register(endpoint_id, identify_cb);
    ESP_ERROR_CHECK(esp_zb_set_primary_network_channel_set(ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK));

    // Because we call with `false`, esp_zb_app_signal_handler will get a
    // `ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP` message, and we must start commissioning
    // manually.
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_stack_main_loop();
}

// color struct
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;

static const color_t idle_color = { .r = 0x80, .g = 0x00, .b = 0x80 };
static const color_t identifying_color = { .r = 0x00, .g = 0x40, .b = 0x00 };
static const color_t pressed_color = { .r = 0x70, .g = 0x40, .b = 0x00 };

// Generate 60 steps of brightness ramp from 0 to 1
static const int brightness_steps = 58;
static const double brightness_ramp[58] = {
    0.000000, 0.017544, 0.035088, 0.052632, 0.070175, 0.087719, 0.105263, 0.122807,
    0.140351, 0.157895, 0.175439, 0.192982, 0.210526, 0.228070, 0.245614, 0.263158,
    0.280702, 0.298246, 0.315789, 0.333333, 0.350877, 0.368421, 0.385965, 0.403509,
    0.421053, 0.438596, 0.456140, 0.473684, 0.491228, 0.508772, 0.526316, 0.543860,
    0.561404, 0.578947, 0.596491, 0.614035, 0.631579, 0.649123, 0.666667, 0.684211,
    0.701754, 0.719298, 0.736842, 0.754386, 0.771930, 0.789474, 0.807018, 0.824561,
    0.842105, 0.859649, 0.877193, 0.894737, 0.912281, 0.929825, 0.947368, 0.964912,
    0.982456, 1.000000
};

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

    int brightness_idx = brightness_steps - 1;
    int delta = -1;
    while (1)
    {
        if (isPressed)
        {
            brightness_idx = brightness_steps - 1;
        }
        else
        {
            // adjust the delta
            if (brightness_idx <= 0)
            {
                delta = 1;
            }
            else if (brightness_idx >= brightness_steps - 1)
            {
                delta = -1;
            }
        }
        brightness_idx += delta;

        const color_t color_to_use = isIdentifying ? identifying_color : isPressed ? pressed_color : idle_color;
        const double brightness = brightness_ramp[brightness_idx];
        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, color_to_use.r * brightness, color_to_use.g * brightness, color_to_use.b * brightness));
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        vTaskDelay(50 / portTICK_PERIOD_MS);
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