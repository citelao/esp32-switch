#include <driver/gpio.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    DBNC_SWITCH_STATE_DOWN,
    DBNC_SWITCH_STATE_UP,
} dbnc_switch_state_t;

// Handler for GPIO debouncer events
// This is called when a switch state is changed
//
// @param gpio GPIO number that changed
//
// TODO: @param arg User data passed to the handler
typedef void (*dbnc_handler_t)(gpio_num_t gpio, dbnc_switch_state_t new_state);

// Required to start the debouncer
esp_err_t dbnc_init(dbnc_handler_t handler);

// Register a GPIO pin as a switch. It'll start reporting events!
esp_err_t dbnc_register_switch(gpio_num_t gpio);

#ifdef __cplusplus
}
#endif