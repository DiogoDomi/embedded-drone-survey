#include "TelemetryManager.h"
#include "Flags.h"
#include <cmath>

TelemetryManager::TelemetryManager(WiFiManager& wifi, GPSManager& gps) :
    m_wifi(wifi),
    m_gps(gps)
    {}

void TelemetryManager::update() {
    m_currentTelemetry.rssi = m_wifi.getRSSIData();
    m_currentTelemetry.gps = m_gps.getGPSData();
}

bool TelemetryManager::shouldSendToWeb() {
    if (isTelemetryDifferent(m_previousWebTelemetry, m_currentTelemetry)) {
        m_previousWebTelemetry = m_currentTelemetry;
        return true;
    }
    return false;
}

bool TelemetryManager::shouldSendToBD() {
    if (isTelemetryDifferent(m_previousDBTelemetry, m_currentTelemetry) &&
    isTelemetryValid(m_currentTelemetry)) {
        m_previousDBTelemetry = m_currentTelemetry;
        return true;
    }
    return false;
}

const TelemetryData& TelemetryManager::getTelemetryData() const {
    return m_currentTelemetry;
}

bool TelemetryManager::isTelemetryValid(const TelemetryData& telemetry) const {
    if (telemetry.rssi == Flags::WIFI_RSSI_INVALID) { return false; }
    if (telemetry.gps.lat == Flags::GPS_INVALID_LOCATION) { return false; }
    return true;
}

bool TelemetryManager::isTelemetryDifferent(const TelemetryData& current, const TelemetryData& previous) const {
    constexpr float EPSILON = 0.000001F;

    if (current.rssi != previous.rssi) { return true; }
    if (fabsf(current.gps.lat - previous.gps.lat) > EPSILON) { return true; }
    if (fabsf(current.gps.lon - previous.gps.lon) > EPSILON) { return true; }
    if (fabsf(current.gps.alt - previous.gps.alt) > EPSILON) { return true; }

    return false;
};