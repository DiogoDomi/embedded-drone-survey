#include "SystemManager.h"

namespace { constexpr unsigned long TELEMETRY_INTERVAL = 1000; }

SystemManager::SystemManager() :   
    m_wifi(),

    m_server(80),
    m_socket("/ws"),
    m_web(m_server, m_socket),

    m_imu(),
    m_flight(m_imu),

    m_gps(),

    m_telemetry(m_wifi, m_gps, m_flight)
    {}

SystemManager& SystemManager::getInstance() {
    static SystemManager instance;
    return instance;
}

void SystemManager::setup() {
    m_flight.begin();

    m_wifi.begin();

    m_web.begin();

    m_gps.begin();

    delay(500);

    m_telemetry.update();

    m_web.cacheTelemetry(m_telemetry.getTelemetryData());

    m_previousTelemetryTime = millis();

}

void SystemManager::loop() {
    m_web.update();
    m_wifi.update();
    m_gps.update();

    bool stateChangeRequested = m_web.hasStateChangeRequest();

    m_flight.update(stateChangeRequested, m_web.getJoystickData());

    if (stateChangeRequested || (millis() - m_previousTelemetryTime >= TELEMETRY_INTERVAL)) { 
        m_previousTelemetryTime = millis();

        m_web.cacheTelemetry(m_telemetry.getTelemetryData());
        m_web.sendTelemetry(m_telemetry.getTelemetryData());
    }
}