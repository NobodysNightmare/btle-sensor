#include "sensor.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "math.h"

#include "driver/i2c.h"

#include "esp_log.h"
static const char *tag = "Sensor";

static i2c_config_t i2c_config;
#define SENSOR_SDA_PIN GPIO_NUM_22
#define SENSOR_SCL_PIN GPIO_NUM_23
#define I2C_PORT I2C_NUM_0
#define I2C_CLOCK_FREQUENCY_HZ 100000
#define I2C_TIMEOUT_MILLISECONDS 1000

#define I2C_ERROR_CHECK(call) if((result = call)) { \
                                  ESP_LOGE(tag, "I2C error in call: %s", esp_err_to_name(result)); \
                                  return -1; \
                              }

// time was calculated according to BME280 data sheet for all measurements in 1x oversampling
#define MEASURE_TIME_MILLISECONDS 10
static struct bme280_dev device;
static struct bme280_data sensor_data;

void user_delay_ms(uint32_t period);
int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);

bool init_i2c() {
    i2c_config.mode = I2C_MODE_MASTER;
    i2c_config.sda_io_num = SENSOR_SDA_PIN;
    i2c_config.scl_io_num = SENSOR_SCL_PIN;
    i2c_config.sda_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_config.scl_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_config.master.clk_speed = I2C_CLOCK_FREQUENCY_HZ;

    esp_err_t result;
    if((result = i2c_param_config(I2C_PORT, &i2c_config))) {
        // Note: apparently for some misconfiguration (bad pins? no slave connected?) we do not reach
        // this code, but get a panic... not sure if we can detect this...
        ESP_LOGE(tag, "Configuring I2C failed: %s", esp_err_to_name(result));
        return false;
    }

    if((result = i2c_driver_install(I2C_PORT, i2c_config.mode, 0, 0, 0))) {
        ESP_LOGE(tag, "Installing I2C driver failed: %s", esp_err_to_name(result));
        return false;
    }

    return true;
}

bool Sensor::init() {
    ESP_LOGI(tag, "Preparing I2C");
    if(!init_i2c()) return false;

    ESP_LOGD(tag, "I2C has been prepared.");

    device.dev_id = BME280_I2C_ADDR_PRIM;
    device.intf = BME280_I2C_INTF;
    device.read = user_i2c_read;
    device.write = user_i2c_write;
    device.delay_ms = user_delay_ms;

    ESP_LOGI(tag, "Initializing BME280...");
    int8_t result = bme280_init(&device);
    if(result != BME280_OK) {
        ESP_LOGE(tag, "Sensor initialize failed: %d", result);
        return false;
    }

    ESP_LOGD(tag, "BME280 has been initialized.");

    device.settings.osr_h = BME280_OVERSAMPLING_1X;
    device.settings.osr_p = BME280_OVERSAMPLING_1X;
    device.settings.osr_t = BME280_OVERSAMPLING_1X;
    device.settings.filter = BME280_FILTER_COEFF_OFF;

    ESP_LOGI(tag, "Configuring BME280...");
    result = bme280_set_sensor_settings(BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL | BME280_OSR_HUM_SEL | BME280_FILTER_SEL, &device);
    if(result != BME280_OK) {
        ESP_LOGE(tag, "Configuring sensor failed: %d", result);
        return false;
    }

    ESP_LOGD(tag, "BME280 has been configured.");

    return true;
}

bool Sensor::readValues() {
    ESP_LOGI(tag, "Starting measurement and waiting...");
    int8_t result = bme280_set_sensor_mode(BME280_FORCED_MODE, &device);
    if(result != BME280_OK) {
        ESP_LOGE(tag, "Starting measurement failed: %d", result);
        return false;
    }

    device.delay_ms(MEASURE_TIME_MILLISECONDS);

    ESP_LOGI(tag, "Retrieving measurement data...");
    result = bme280_get_sensor_data(BME280_ALL, &sensor_data, &device);
    if(result != BME280_OK) {
        ESP_LOGE(tag, "Fetching sensor data failed: %d", result);
        return false;
    }

    ESP_LOGD(tag, "Measurement received.");

    return true;
}

int16_t Sensor::getTemperature() {
    return sensor_data.temperature;
}

uint16_t Sensor::getHumidity() {
    return round(sensor_data.humidity / 10.24);
}

uint16_t Sensor::getPressure() {
    return sensor_data.pressure / 10;
}

void user_delay_ms(uint32_t period) {
    ESP_LOGV(tag, "Waiting for %d ms", period);
    vTaskDelay(period / portTICK_PERIOD_MS);
}

int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len) {
    /*
     * The parameter dev_id can be used as a variable to store the I2C address of the device
     */

    /*
     * Data on the bus should be like
     * |------------+---------------------|
     * | I2C action | Data                |
     * |------------+---------------------|
     * | Start      | -                   |
     * | Write      | (reg_addr)          |
     * | Stop       | -                   |
     * | Start      | -                   |
     * | Read       | (reg_data[0])       |
     * | Read       | (....)              |
     * | Read       | (reg_data[len - 1]) |
     * | Stop       | -                   |
     * |------------+---------------------|
     */

    ESP_LOGV(tag, "Preparing cmd link to read data from 0x%x...", dev_id);
    esp_err_t result;
    i2c_cmd_handle_t command = i2c_cmd_link_create();
    I2C_ERROR_CHECK(i2c_master_start(command))
    I2C_ERROR_CHECK(i2c_master_write_byte(command, (dev_id << 1) | I2C_MASTER_WRITE, true))
    I2C_ERROR_CHECK(i2c_master_write_byte(command, reg_addr, true))
    I2C_ERROR_CHECK(i2c_master_stop(command)) // documented in the example above, but not in spec sheet
    I2C_ERROR_CHECK(i2c_master_cmd_begin(I2C_PORT, command, I2C_TIMEOUT_MILLISECONDS / portTICK_RATE_MS))

    ESP_LOGV(tag, "Asking to read...");
    I2C_ERROR_CHECK(i2c_master_cmd_begin(I2C_PORT, command, I2C_TIMEOUT_MILLISECONDS / portTICK_RATE_MS))
    ESP_LOGV(tag, "Finished asking to read.");

    command = i2c_cmd_link_create();
    I2C_ERROR_CHECK(i2c_master_start(command)) // documented in the example above, but not in spec sheet
    I2C_ERROR_CHECK(i2c_master_write_byte(command, (dev_id << 1) | I2C_MASTER_READ, true))
    I2C_ERROR_CHECK(i2c_master_read(command, reg_data, len, I2C_MASTER_LAST_NACK))
    I2C_ERROR_CHECK(i2c_master_stop(command))

    ESP_LOGV(tag, "Beginning to read...");
    I2C_ERROR_CHECK(i2c_master_cmd_begin(I2C_PORT, command, I2C_TIMEOUT_MILLISECONDS / portTICK_RATE_MS))
    ESP_LOGV(tag, "Finished reading.");

    return 0;
}

int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len) {
    /*
     * The parameter dev_id can be used as a variable to store the I2C address of the device
     */

    /*
     * Data on the bus should be like
     * |------------+---------------------|
     * | I2C action | Data                |
     * |------------+---------------------|
     * | Start      | -                   |
     * | Write      | (reg_addr)          |
     * | Write      | (reg_data[0])       |
     * | Write      | (....)              |
     * | Write      | (reg_data[len - 1]) |
     * | Stop       | -                   |
     * |------------+---------------------|
     */

    ESP_LOGV(tag, "Preparing cmd link to write data to 0x%x...", dev_id);
    esp_err_t result;
    i2c_cmd_handle_t command = i2c_cmd_link_create();
    I2C_ERROR_CHECK(i2c_master_start(command))
    I2C_ERROR_CHECK(i2c_master_write_byte(command, (dev_id << 1) | I2C_MASTER_WRITE, true))
    I2C_ERROR_CHECK(i2c_master_write_byte(command, reg_addr, true))
    I2C_ERROR_CHECK(i2c_master_write(command, reg_data, len, true))
    I2C_ERROR_CHECK(i2c_master_stop(command))

    ESP_LOGV(tag, "Beginning to write...");
    I2C_ERROR_CHECK(i2c_master_cmd_begin(I2C_PORT, command, I2C_TIMEOUT_MILLISECONDS / portTICK_RATE_MS))
    ESP_LOGV(tag, "Finished writing.");

    return 0;
}
