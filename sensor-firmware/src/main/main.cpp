#include "bt.h"
#include "sensor.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "esp_log.h"
static const char *tag = "MAIN";

#define ADVERTISE_TIME_SECONDS 5
#define SENSOR_READ_PERIOD_SECONDS 60

void deinit() {
    BT::deinit();
    ESP_LOGI(tag, "Going to sleep...");
    esp_deep_sleep((SENSOR_READ_PERIOD_SECONDS - ADVERTISE_TIME_SECONDS) * 1000000);
}

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
        deinit();
        return;
    }

    if(!Sensor::init()) {
        ESP_LOGE(tag, "Sensor could not be initialized.");
        deinit();
        return;
    }

    if(!Sensor::readValues()) {
        ESP_LOGE(tag, "Sensor could not perform measurement.");
        deinit();
        return;
    }

    ESP_LOGI(tag, "Advertising readings...");
    BT::advertise(Sensor::getTemperature(), Sensor::getHumidity());

    vTaskDelay((1000 * ADVERTISE_TIME_SECONDS) / portTICK_PERIOD_MS);

    deinit();
}