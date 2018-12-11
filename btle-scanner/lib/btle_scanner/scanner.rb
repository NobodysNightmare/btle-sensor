require 'scan_beacon'

module BtleScanner
  class Scanner
    class << self
      def each_advertisement
        device_id = ScanBeacon::BlueZ.devices[0][:device_id]
        ScanBeacon::BlueZ.device_up device_id

        ScanBeacon::BlueZ.scan(device_id) do |mac, data, rssi|
          next unless mac && data

          elements = []
          while data && data.size > 0
            length, type = data.unpack('CC')
            elements << {
              type: type,
              data: data[2..length]
            }

            data = data[(length + 1)..-1]
          end

          yield mac, elements, rssi
        end
      end
    end
  end
end
