require 'scan_beacon'

device_id = ScanBeacon::BlueZ.devices[0][:device_id]
ScanBeacon::BlueZ.device_up device_id

def nice_scan(device_id)
  ScanBeacon::BlueZ.scan(device_id) do |mac, data, rssi|
    yield nil unless data

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


puts 'Started scanning...'
nice_scan(device_id) do |mac, elements, rssi|
  next unless mac && mac.start_with?("30:")

  own_data = elements.find { |e| e[:type] == 0xff }&.fetch(:data)
  next unless own_data
  own_data = own_data[2..-1]
  flags, temperature, humidity = own_data.unpack('CS<S<')
  puts
  puts '## Advertisement'
  puts "Mac: #{mac}"
  puts "Flags: #{flags}"
  puts "Temperature: #{temperature / 10.0} °C"
  puts "Humidity: #{humidity / 10.0} %"
end
