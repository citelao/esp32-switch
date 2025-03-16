#include <driver/gpio.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

// Handler for GPIO debouncer events
// This is called when a switch state is changed
//
// @param gpio GPIO number that changed
//
// TODO: @param arg User data passed to the handler
typedef void (*dbnc_handler_t)(gpio_num_t gpio);

// Required to start the debouncer
esp_err_t dbnc_init(dbnc_handler_t handler);

// Register a GPIO pin as a switch. It'll start reporting events!
esp_err_t dbnc_register_switch(gpio_num_t gpio);

#ifdef __cplusplus
}
#endif