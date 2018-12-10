require 'set'

require 'btle_scanner/scanner'

module BtleScanner
  # Scans for compatible BTLE devices
  class DiscoveryService
    # company id of compatible devices (indicated in manufacturer_data)
    COMPANY_ID = "\xFF\xFF"

    def each_device
      known_macs = Set.new
      Scanner.each_advertisement do |mac, elements, rssi|
        next if known_macs.include? mac

        known_macs << mac

        manufacturer_data = elements.find { |e| e.fetch(:type) == 0xff }

        next unless manufacturer_data_compatible?(manufacturer_data)

        name = nil
        name_element = elements.find { |e| e.fetch(:type) == 9 }
        name = name_element.fetch(:data) if name_element
        yield mac, name
      end
    end

    private

    def manufacturer_data_compatible?(manufacturer_data)
      # if there is no manufacturer_data, no readings could be transmitted
      return false unless manufacturer_data
      company_id = manufacturer_data.fetch(:data)[0..1]

      company_id.bytes == COMPANY_ID.bytes
    end
  end
end
