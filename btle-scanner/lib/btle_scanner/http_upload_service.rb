require 'httparty'

module BtleScanner
  # Waits for sensor updates from configured devices
  # and makes their data available
  class HttpUploadService
    def initialize(sensor)
      @sensor = sensor
    end

    def upload(readings)
      readings.each do |kind, value|
        post_value(kind, value)
      end
    end

    private

    def post_value(kind, amount)
      friendly_name = "#{@sensor.fetch('name')} #{friendly_kind(kind)}"
      device_name = underscore(friendly_name)
      response = HTTParty.post("#{@sensor.fetch('home_assistant_url')}/api/states/sensor.#{device_name}_#{kind}",
                               body: {
                                 state: amount,
                                 attributes: { unit_of_measurement: unit_for(kind), friendly_name: friendly_name } }.to_json,
                               headers: { 'X-HA-Access': @sensor.fetch('home_assistant_key') })
      raise "Unexpected response: #{response.inspect}" unless [200, 201].include?(response.code)
    end

    def underscore(string)
      string.gsub(/\s/, '_')
    end

    def unit_for(kind)
      case(kind)
      when :temperature
        '°C'
      when :humidity
        '%'
      when :pressure
        'hPa'
      else
        raise 'Unexpected measurement kind'
      end
    end

    def friendly_kind(kind)
      case(kind)
      when :temperature
        'Temperature'
      when :humidity
        'Humidity'
      when :pressure
        'Pressure'
      else
        raise 'Unexpected measurement kind'
      end
    end
  end
end
