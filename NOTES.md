# Zigbee questions

## "Coordinator" vs "router" vs "end device"

> What's a Zigbee coordinator vs a router vs an end device?

Via [Silicon Labs](https://community.silabs.com/s/article/what-is-the-difference-between-an-end-device-a-router-and-a-coordinator-do-i?language=en_US), emphasis added:

> In ZigBee, there are three different types of devices: end device, router, and coordinator. The key difference between these is that an **end device** can not route traffic, **routers** can route traffic, and the **coordinator**, in addition to routing traffic, is responsible for forming the network in the first place. Every network must have one and only one coordinator.

Also, **end devices** "may also sleep", whereas **routers** "may not sleep" (at least in some implementations?):

> Because **end devices** may sleep, all traffic to an end device is first routed to its parent. The end device is responsible for requesting any pending messages from its parent. If an end device has moved, it is responsible for informing the network that it has rejoined to a new parent.
>
> [...] **Routers** are also responsible for receiving and storing messages intended for their children.

Seriously, read [their brief page](https://community.silabs.com/s/article/what-is-the-difference-between-an-end-device-a-router-and-a-coordinator-do-i?language=en_US). See also [Digi](https://www.digi.com/resources/documentation/Digidocs/90002002/Concepts/c_device_types.htm?TocPath=Zigbee%20networks%7CZigbee%20networking%20concepts%7C_____1).

## "Client" vs "Server"

**Clients** ask the questions; **servers** have the answer.

For example, if a switch exposes its battery life, it is a server for that info. Anyone asking about it is a client.

https://github.com/SiliconLabs/zigbee_applications/blob/master/zigbee_concepts/Zigbee-Introduction/Zigbee%20Introduction%20-%20Clusters%2C%20Endpoints%2C%20Device%20Types.md

https://medium.com/@omaslyuchenko/hello-zigbee-world-part-15-commands-binding-133567d690b9

## `RW` vs `R/W` vs `R*W`?

What's the difference between the 3 terms?

* `RW`: readable & writeable
* `R/W`: readable & writeable
* `R*W`: readable & *optionally* writeable 

See [Zigbee Cluster Library Specification](https://zigbeealliance.org/wp-content/uploads/2019/12/07-5123-06-zigbee-cluster-library-specification.pdf) **1.2 Acronyms and Abbreviations** (page 1-3).


## How do I send "identify" from Z2M?

1. Dev Console for your device
2. Endpoint `1`, Cluster `3`, Command `0`, Payload a la `{"identifytime": 3}` (time in seconds; case-sensitive)

https://medium.com/@omaslyuchenko/hello-zigbee-part-22-identify-cluster-90cf12680306

---

# SDK questions

## `esp_zb_scheduler_user_alarm` vs `esp_zb_scheduler_alarm`

> What's the difference between `esp_zb_scheduler_user_alarm` and `esp_zb_scheduler_alarm`?

`*_user_*` supports a custom pointer parameter, the other version does not:

```cpp
void esp_zb_scheduler_alarm(esp_zb_callback_t cb, uint8_t param, uint32_t time);
esp_zb_user_cb_handle_t esp_zb_scheduler_user_alarm(esp_zb_user_callback_t cb, void *param, uint32_t time);
```

https://github.com/espressif/esp-zigbee-sdk/issues/507#issuecomment-2552660985

## How to add a new component?

Super simple!

1. Create the component dir
2. Create the CMakeLists.txt, `.c`, & `.h` files
3. Add a `REQUIRES` statement to the main component CMakeLists.txt

https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html#minimal-component-cmakelists

## Can I log from an ISR interupt?

Yes! Just use `ESP_EARLY_LOGI` instead of `ESP_LOGI`. (Also `ESP_RETURN_ON_ERROR_ISR` instead of `ESP_RETURN_ON_ERROR`).

Because of stack maxes.

https://github.com/espressif/esp-zigbee-sdk/blob/8114916a4c6d1b4587a9fc24d2c85a1396328a28/examples/esp_zigbee_HA_sample/HA_color_dimmable_switch/main/esp_zb_switch.c#L67

## Where do all the error macros live?

Include `esp_check.h`.

See also my `errors` component for missing macros.

https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/esp_err.html