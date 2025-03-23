// Adapted from https://github.com/espressif/esp-idf/blob/master/examples/zigbee/light_sample/HA_on_off_switch/main/switch_driver.c
#include "debouncer.h"

#include "esp_check.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <driver/gpio.h>
#include <errors.h>

// Number of events to store in the queue
static const int DBNC_STORED_EVENTS = 10;

// Time to wait for a keypress (MS)
static const int DBNC_DEBOUNCE_DELAY_MS = 200;

// Tag for logging
static const char* TAG = "CL_DEBOUNCE";

static QueueHandle_t s_queue = NULL;
static dbnc_handler_t s_handler = NULL;

static dbnc_switch_state_t level_to_state(int level)
{
    return (level != 0) ? DBNC_SWITCH_STATE_HIGH : DBNC_SWITCH_STATE_LOW;
}

static void dbnc_task(void *arg)
{
    while (1)
    {
        // TODO: simultaneously handle keypress
        gpio_num_t pin;
        ABORT_IF_FALSE(xQueueReceive(s_queue, &pin, portMAX_DELAY), ESP_ERR_INVALID_STATE, TAG, "Failed to receive from queue");

        // Handle the event
        const int level = gpio_get_level(pin);
        ESP_LOGI(TAG, "Switch state changed on GPIO %d (to %d)", pin, level);
        ESP_ERROR_CHECK(gpio_intr_disable(pin));

        const dbnc_switch_state_t state = level_to_state(level);

        // Fire the handler as soon as we get the event.
        s_handler(pin, state);

        vTaskDelay(pdMS_TO_TICKS(DBNC_DEBOUNCE_DELAY_MS));

        // Check to see if the switch is still pressed after the delay.
        //
        // TODO: it's not handling release of very bounce switches well. I think
        // we have to rerun this delay if we detect a change.
        const bool levelAfterDelay = gpio_get_level(pin);
        if (level != levelAfterDelay)
        {
            // The switch state has changed, so we need to handle it again.
            s_handler(pin, level_to_state(levelAfterDelay));
        }

        // ESP_LOGI(TAG, "Switch GPIO %d is %s, %s after delay", pin, level ? "pressed" : "released", levelAfterDelay ? "pressed" : "released");
        ESP_ERROR_CHECK(gpio_intr_enable(pin));
    }
}

static void IRAM_ATTR handle_switch_pressed(void *arg)
{
    // Fetch the GPIO number from the argument
    gpio_num_t gpio_num = (gpio_num_t)arg;

    // Send the GPIO number to the queue
    xQueueSendFromISR(s_queue, &gpio_num, NULL);

    // BTW, logging in an ISR stackoverflows because of the stack maxes. Use
    // ESP_EARLY_LOGI instead.
    // https://github.com/espressif/esp-zigbee-sdk/blob/8114916a4c6d1b4587a9fc24d2c85a1396328a28/examples/esp_zigbee_HA_sample/HA_color_dimmable_switch/main/esp_zb_switch.c#L67
}

esp_err_t dbnc_init(dbnc_handler_t handler)
{
    ESP_RETURN_ON_FALSE(handler, ESP_ERR_INVALID_ARG, TAG, "Handler is NULL");
    ESP_RETURN_ON_FALSE(s_handler == NULL, ESP_ERR_INVALID_STATE, TAG, "Already initialized");
    ESP_RETURN_ON_FALSE(s_queue == NULL, ESP_ERR_INVALID_STATE, TAG, "Already initialized");

    s_handler = handler;
    s_queue = xQueueCreate(DBNC_STORED_EVENTS, sizeof(gpio_num_t));

    ESP_RETURN_ON_FALSE(s_queue, ESP_ERR_NO_MEM, TAG, "Failed to create queue");
    
    BaseType_t result = xTaskCreate(dbnc_task, "dbnc_task", 4096, NULL, 10, NULL);
    ESP_RETURN_ON_FALSE(result == pdPASS, ESP_ERR_NO_MEM, TAG, "Failed to create task");

    return ESP_OK;
}

esp_err_t dbnc_register_switch(gpio_num_t gpio)
{
    ESP_RETURN_ON_ERROR_ISR(gpio_isr_handler_add(gpio, handle_switch_pressed, (void*)gpio), TAG, "Failed to register GPIO %d", gpio);
    return ESP_OK;
}