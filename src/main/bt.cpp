#include "bt.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_bt.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"

static const char *tag = "BT";

bool BT::init() {
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    esp_err_t ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    if (ret) {
        ESP_LOGI(tag, "Bluetooth controller release classic bt memory failed: %s", esp_err_to_name(ret));
        return false;
    }

    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGI(tag, "Bluetooth controller initialize failed: %s", esp_err_to_name(ret));
        return false;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_BLE)) != ESP_OK) {
        ESP_LOGI(tag, "Bluetooth controller enable failed: %s", esp_err_to_name(ret));
        return false;
    }

    return true;
}