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

## How are XY colors represented?

CIE 1931 xyY.

> This cluster provides an interface for changing the color of a light. Color is specified according to the 
Commission Internationale de l'Éclairage (CIE) specification CIE 1931 Color Space, [I1]. Color control is 
carried out in terms of x,y values, as defined by this specification
> [...]
> The CurrentX attribute contains the current value of the normalized chromaticity value x, as defined in the 
CIE xyY Color Space. It is updated as fast as practical during commands that change the color.
>
> The value of x SHALL be related to the CurrentX attribute by the relationship
>
> x = CurrentX / 65536 (CurrentX in the range 0 to 65279 inclusive)

[ZCL Spec pg. 5-2, 5-5](https://zigbeealliance.org/wp-content/uploads/2019/12/07-5123-06-zigbee-cluster-library-specification.pdf)

The xyY colorspace is a normalized version of the XYZ colorspace, which is based on the standard luminosity functions. Because xyY is normalized, `x + y + z = 1`, therefore `z = 1 - x - y`, therefore you can represent any color with just `x` and `y`. An additional `Y` represents the *brightness* of the color, corresponding to the original `Y` in XYZ (which happens to be basically identical to visible brightness).

https://www.scratchapixel.com/lessons/digital-imaging/colors/color-space.html#:~:text=Once%20normalized%2C%20the%20values%20of%20the%20resulting%20x%20y%20and%20z%20components%20sum%20up%20to%201.%20Therefore%20we%20can%20find%20the%20value%20of%20any%20of%20the%20components%20if%20we%20know%20the%20values%20of%20the%20other%20two.

https://github.com/Koenkk/zigbee2mqtt/issues/272
https://easyrgb.com/en/math.php

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

## Do pull-ups/pull-downs need an external resistor?

No, unless the trace is long enough that you worry antenna effects will induce
larger current than the very, very tiny current that the internal resistors
allow.

https://archive.seattlerobotics.org/encoder/199703/basics.html
https://electronics.stackexchange.com/questions/270833/considerations-when-using-internal-pull-up-down-resistors
https://electronics.stackexchange.com/a/20746/296425

I had an interesting outcome where I'd wired up the buttons using external
resistors---I had set the input pins to be pull-DOWN, but they still behaved as
pull-UP (e.g. high when switch open, low when switch closed). 

I have 2/2 switches, and I got the pinouts wrong. I thought it was:

   -+        +-
    | switch |
   -+        +-

But it was:

   -----+------
      switch
   -----+------

So I accidentally wired this:

   +3.3V (or Vcc)
      |
      Resistor (~200R)
      |
      LED
      |
      +------- IC Input Pin
      |
   [Switch]
      |
    Ground

Which is *explicitly* a pull-UP wiring. And since the external resistor was MUCH
weaker than the internal resistor, enough current flowed to completely replace
whatever the internal resistor set.

