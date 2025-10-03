#include "SystemManager.h"

SystemManager::SystemManager() :   
    m_wifi(),

    m_server(80),
    m_socket("/ws"),
    m_web(m_server, m_socket),

    m_imu(),
    m_flight(m_imu),

    m_gps(),

    m_telemetry(m_wifi, m_gps)
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

    m_web.cacheTelemetry(m_telemetry.getTelemetryData(), m_flight.getStateData());
}

void SystemManager::loop() {
    m_web.update();
    m_wifi.update();
    m_gps.update();

    bool hasStateChangeRequest = m_web.hasStateChangeRequest();
    m_flight.update(hasStateChangeRequest, m_web.getJoystickData());
    m_telemetry.update();

    if (hasStateChangeRequest || m_telemetry.shouldSendToWeb()) {

        const TelemetryData telemetryData = m_telemetry.getTelemetryData();
        const State state = m_flight.getStateData();

        m_web.cacheTelemetry(telemetryData, state);
        m_web.sendTelemetry(telemetryData, state);
    }

    // if (m_telemetry.shouldSendToBD()) {}
}