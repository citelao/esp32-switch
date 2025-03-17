// Adapted from https://github.com/espressif/esp-idf/blob/master/examples/zigbee/light_sample/HA_on_off_switch/main/switch_driver.c
#include "debouncer.h"

#include "esp_check.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <driver/gpio.h>

// Number of events to store in the queue
static const int DBNC_STORED_EVENTS = 10;

// Time to wait for a keypress (MS)
static const int DBNC_DEBOUNCE_DELAY_MS = 50;

// Tag for logging
static const char* TAG = "CL_DEBOUNCE";

static QueueHandle_t s_queue = NULL;
static dbnc_handler_t s_handler = NULL;

static void dbnc_task(void *arg)
{
    while (1)
    {
        gpio_num_t pin;
        if (xQueueReceive(s_queue, &pin, portMAX_DELAY))
        {
            // Handle the event
            ESP_LOGI(TAG, "Switch pressed on GPIO %d", pin);
            ESP_ERROR_CHECK(gpio_intr_disable(pin));
        }

        // TODO: simultaneously handle keypress
        const bool isPressed = !gpio_get_level(pin);
        const dbnc_switch_state_t state = isPressed ? DBNC_SWITCH_STATE_DOWN : DBNC_SWITCH_STATE_UP;

        // Fire the handler as soon as we get the event.
        s_handler(pin, state);

        vTaskDelay(pdMS_TO_TICKS(DBNC_DEBOUNCE_DELAY_MS));

        // Check to see if the switch is still pressed after the delay.
        const bool isPressedAfterDelay = !gpio_get_level(pin);
        if (isPressed != isPressedAfterDelay)
        {
            // The switch state has changed, so we need to handle it again.
            s_handler(pin, isPressedAfterDelay ? DBNC_SWITCH_STATE_DOWN : DBNC_SWITCH_STATE_UP);
        }

        // ESP_LOGI(TAG, "Switch GPIO %d is %s, %s after delay", pin, isPressed ? "pressed" : "released", isPressedAfterDelay ? "pressed" : "released");
        ESP_ERROR_CHECK(gpio_intr_enable(pin));
    }
}

static void IRAM_ATTR handle_switch_pressed(void *arg)
{
    // Fetch the GPIO number from the argument
    gpio_num_t gpio_num = (gpio_num_t)arg;

    // Send the GPIO number to the queue
    xQueueSendFromISR(s_queue, &gpio_num, NULL);
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