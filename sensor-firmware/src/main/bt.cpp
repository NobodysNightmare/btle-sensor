#include "bt.h"

#include "esp_log.h"
static const char *tag = "BT";

#define UINT8_TO_STREAM(p, u8)   {*(p)++ = (uint8_t)(u8);}
#define UINT16_TO_STREAM(p, u16) {*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}
#define PAYLOAD_SIZE 7

static esp_ble_adv_params_t adv_params = {};
static esp_ble_adv_data_t adv_data = {};
static uint8_t payload[PAYLOAD_SIZE];
static const char *device_name = "NN Sensor";

bool BT::init() {
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    esp_err_t ret;
    if ((ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT))) {
        ESP_LOGE(tag, "Bluetooth controller release classic bt memory failed: %s", esp_err_to_name(ret));
        return false;
    }

    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(tag, "Bluetooth controller initialize failed: %s", esp_err_to_name(ret));
        return false;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_BLE)) != ESP_OK) {
        ESP_LOGE(tag, "Bluetooth controller enable failed: %s", esp_err_to_name(ret));
        return false;
    }

    if ((ret = esp_bluedroid_init())){
        ESP_LOGE(tag, "Could not initialize bluetooth: %s", esp_err_to_name(ret));
        return false;
    }

    if ((ret = esp_bluedroid_enable())){
        ESP_LOGE(tag, "Could not enable bluetooth: %s", esp_err_to_name(ret));
        return false;
    }

    if((ret = esp_ble_gap_set_device_name(device_name))) {
        ESP_LOGE(tag, "Failed to set device name: %s", esp_err_to_name(ret));
        return false;
    }

    adv_params.adv_int_min       = 800; // 500ms equivalent
    adv_params.adv_int_max       = 800; // (multiply by 0.625ms)
    adv_params.adv_type          = ADV_TYPE_NONCONN_IND;
    adv_params.own_addr_type     = BLE_ADDR_TYPE_PUBLIC;
    adv_params.channel_map       = ADV_CHNL_ALL;
    adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST;

    adv_data.include_name = true;
    adv_data.manufacturer_len = PAYLOAD_SIZE;
    adv_data.p_manufacturer_data = payload;

    return true;
}

void BT::advertise(uint16_t temperature, uint16_t humidity) {
    uint8_t* stream = payload;
    UINT16_TO_STREAM(stream, 0xFFFF); // company ID
    UINT8_TO_STREAM(stream, 3);       // flags of submitted data (0x1 temperature | 0x2 humidity)
    UINT16_TO_STREAM(stream, temperature);
    UINT16_TO_STREAM(stream, humidity);

    esp_err_t ret;
    if((ret = esp_ble_gap_config_adv_data(&adv_data))) {
        ESP_LOGE(tag, "Failed to set advertising data: %s", esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_ble_gap_start_advertising(&adv_params))) {
        ESP_LOGE(tag, "Failed to start advertising: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(tag, "Started advertising...");
}