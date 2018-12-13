#include "bme280.h"

namespace Sensor {
    bool init();
    bool readValues();

    int16_t getTemperature(); // temperature in 100 * °C
    uint16_t getHumidity();   // humidity in 100 * % relative humidity
    uint16_t getPressure();   // pressure in 10 * hPa (or Pascal / 10)
}