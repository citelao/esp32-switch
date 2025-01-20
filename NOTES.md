# `esp_zb_scheduler_user_alarm` vs `esp_zb_scheduler_alarm`

> What's the difference between `esp_zb_scheduler_user_alarm` and `esp_zb_scheduler_alarm`?

`*_user_*` supports a custom pointer parameter, the other version does not:

```cpp
void esp_zb_scheduler_alarm(esp_zb_callback_t cb, uint8_t param, uint32_t time);
esp_zb_user_cb_handle_t esp_zb_scheduler_user_alarm(esp_zb_user_callback_t cb, void *param, uint32_t time);
```

https://github.com/espressif/esp-zigbee-sdk/issues/507#issuecomment-2552660985