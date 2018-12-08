#include "bt.h"
#include "nvs_flash.h"

#include "esp_log.h"

static const char *tag = "MAIN";

extern "C" void app_main() {
    ESP_LOGI(tag, "Starting up...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    if(BT::init()) {
        ESP_LOGI(tag, "Bluetooth initialized successfully.");
    } else {
        ESP_LOGE(tag, "Bluetooth could not be initialized.");
        return;
    }

    BT::advertise(215, 650);
}