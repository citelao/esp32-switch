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

---

# SDK questions

# `esp_zb_scheduler_user_alarm` vs `esp_zb_scheduler_alarm`

> What's the difference between `esp_zb_scheduler_user_alarm` and `esp_zb_scheduler_alarm`?

`*_user_*` supports a custom pointer parameter, the other version does not:

```cpp
void esp_zb_scheduler_alarm(esp_zb_callback_t cb, uint8_t param, uint32_t time);
esp_zb_user_cb_handle_t esp_zb_scheduler_user_alarm(esp_zb_user_callback_t cb, void *param, uint32_t time);
```

https://github.com/espressif/esp-zigbee-sdk/issues/507#issuecomment-2552660985