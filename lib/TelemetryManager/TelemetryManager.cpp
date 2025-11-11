#include "TelemetryManager.h"
#include "Flags.h"
#include <cmath>

TelemetryManager::TelemetryManager(WiFiManager& wifi, GPSManager& gps, FlightManager& flight, TimeManager& time) :
    m_wifi(wifi),
    m_gps(gps),
    m_flight(flight),
    m_time(time)
    {}

void TelemetryManager::update() {
    m_telemetry.timestamp = m_time.getTimestamp();
    m_telemetry.state = static_cast<uint8_t>(m_flight.getStateData());
    m_telemetry.rssi = m_wifi.getRSSIData();
    m_telemetry.gps = m_gps.getGPSData();
    m_telemetry.isValid = isTelemetryValid(m_telemetry);
}

const TelemetryData& TelemetryManager::getTelemetry() const {
    return m_telemetry;
}

bool TelemetryManager::isTelemetryValid(const TelemetryData& telemetry) const {
    if (telemetry.timestamp <= 0) { return false; }
    if (telemetry.rssi == Flags::WIFI_RSSI_INVALID) { return false; }
    if (telemetry.gps.lat == Flags::GPS_INVALID_LOCATION) { return false; }
    if (telemetry.gps.alt == Flags::GPS_INVALID_ALTITUDE) { return false; }
    return true;
}