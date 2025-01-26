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

### Usage: bind to a light

In Z2M:

1. Click on the switch to open the details page.
2. Click **Bind**
3. In **Destination** & **endpoint**, select the light you wish to control
4. Check **LevelCtrl**, **OnOff**, and **LColorCtlr**
5. Click **Bind** (the switch does not currently sleep, but if it does, you may
   need to wake it up by pressing any button on it.)

The button will now control your light!


## Troubleshooting

### Zigbee headers not present

1. **ESP-IDF: SDK Configuration Editor (Menuconfig)**
2. Enable Zigbee (at the bottom)

https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/kconfig/configuration_structure.html#sdkconfig-file

### Cannot interview with Zigbee2MQTT

Something like:

> Error: Interview failed because can not get node descriptor

Also applies if metadata is outdated on Z2M after making firmware changes.

1. Delete the device from Z2M ("Force delete" is OK).
2. **ESP-IDF: Erase Flash Memory From Device**.
3. Re-flash the device.
4. (If you made changes to clusters/stuff that is "cached" by Z2M): Restart Z2M.

https://github.com/Koenkk/zigbee2mqtt/issues/24202