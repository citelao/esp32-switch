# ESP32-Switch

Trying to implement a nice Zigbee switch on an ESP32-C6.

## Usage

Prerequisites:

* Hardware: **ESP32C6** or ESP32H2
* Software: **ESP-IDF** or equivalent. This was generated from `ESP-IDF: New Project` in VS Code. See [the official docs](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#start-a-new-project).
* Software: **VS Code** with [**ESP-IDF extension**](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension). Makes everything *so* easy.

Configure the project:

1. Select the right **target** (`ESP-IDF: Set Expressif Device Target`; probably an `esp32c6` or H2).
2. Select the right **port** (`ESP-IDF: Select Port To Use`, probably UART & some COM port or dev path)

Inner loop:

1. Make your changes
2. `ESP-IDF: Build, Flash, and Monitor` (or just substeps of that; `Ctrl-E D`)

Yay!

## Troubleshooting

### Zigbee headers not present

1. **ESP-IDF: SDK Configuration Editor (Menuconfig)**
2. Enable Zigbee (at the bottom)

https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/kconfig/configuration_structure.html#sdkconfig-file
