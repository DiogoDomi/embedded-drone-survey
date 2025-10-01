#include "GPSManager.h"
#include "Pins.h"
#include "Flags.h"
#include <Arduino.h>

namespace { constexpr uint16_t SERIAL_BAUDRATE = 9600; }

GPSManager::GPSManager() :
    m_swSerial(Pins::GPS::RX_PIN, Pins::GPS::TX_PIN)
    {}

void GPSManager::begin() {
    m_swSerial.begin(SERIAL_BAUDRATE);
}

void GPSManager::update() { 
    while (m_swSerial.available() > 0) {
        char c = m_swSerial.read();
        Serial.print(c);
        if (m_gps.encode(c)) {
            if (m_gps.location.isUpdated()) {
                if (m_gps.location.isValid()) {
                    m_data.lat = m_gps.location.lat();
                    m_data.lon = m_gps.location.lng();
                } else {
                    m_data.lat = Flags::GPS_LAT_INVALID;
                    m_data.lon = Flags::GPS_LON_INVALID;
                }
            }
            if (m_gps.altitude.isUpdated()) {
                if (m_gps.altitude.isValid()) {
                    m_data.alt = m_gps.altitude.meters();
                } else {
                    m_data.alt = Flags::GPS_ALT_INVALID;
                }
            }
        }
    }
}

GPSData GPSManager::getData() const { return m_data; }