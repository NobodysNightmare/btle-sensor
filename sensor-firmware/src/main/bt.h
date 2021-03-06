#include "freertos/FreeRTOS.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"

namespace BT {
    bool init();
    void deinit();
    void advertise(uint16_t temperature, uint16_t humidity);
};