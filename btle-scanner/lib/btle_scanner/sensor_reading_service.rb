require 'set'

require 'btle_scanner/scanner'

module BtleScanner
  # Waits for sensor updates from configured devices
  # and makes their data available
  class SensorReadingService
    def initialize(sensors)
      @sensors = sensors.dup
    end

    def each_reading
      Scanner.each_advertisement do |mac, elements, rssi|
        sensor = @sensors[mac]

        next unless sensor

        duplicate_time = sensor.fetch('duplicate_time')
        next if (sensor['last_reading'] || 0) > (Time.now - duplicate_time).to_i
        sensor['last_reading'] = Time.now.to_i

        own_data = elements.find { |e| e[:type] == 0xff }&.fetch(:data)
        unless own_data
          raise 'Expected manufacturer data to be present ' \
                "(Advertisement from #{mac})"
        end

        own_data = own_data[2..-1] # strip company id

        # TODO: use flags to determine available fields
        flags, temperature, humidity = own_data.unpack('CS<S<')
        yield(mac, {
          temperature: temperature / 10.0,
          humidity:    humidity / 10.0
        })
      end
    end
  end
end
