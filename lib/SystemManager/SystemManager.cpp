#include "SystemManager.h"
#include <time.h>

namespace {
    constexpr unsigned long DB_SEND_INTERVAL = 1000;
}

SystemManager::SystemManager() :   
    m_server(80),
    m_socket("/ws"),
    m_wifi(),
    m_web(m_server, m_socket),
    m_imu(),
    m_flight(m_imu),
    m_gps(),
    m_telemetry(m_wifi, m_gps),
    m_database(),
    m_time()
    {}

SystemManager& SystemManager::getInstance() {
    static SystemManager instance;
    return instance;
}

void SystemManager::setup() {
    m_flight.begin();
    m_gps.begin();
    delay(2000);
    m_imu.begin();
    m_wifi.begin();
    m_web.begin();
    m_time.begin();
    m_database.begin();

    m_telemetry.update();
    m_web.cacheTelemetry(m_telemetry.getTelemetryData(), m_flight.getStateData());
}

void SystemManager::loop() {
    m_imu.update();
    m_web.update();
    bool hasStateChangeRequest = m_web.hasStateChangeRequest();
    JoystickData joystickData = m_web.getJoystickData();
    m_flight.update(hasStateChangeRequest, joystickData);
    m_wifi.update();
    m_gps.update();
    m_time.update();
    m_telemetry.update();

    if (hasStateChangeRequest || m_telemetry.shouldSendToWeb()) {
        const TelemetryData telemetryData = m_telemetry.getTelemetryData();
        State state = m_flight.getStateData();
        m_web.cacheTelemetry(telemetryData, state);
        m_web.sendTelemetry(telemetryData, state);
    }

    if ((millis() - m_lastDBSendTime >= DB_SEND_INTERVAL) || m_telemetry.shouldSendToBD()) {
        if (m_wifi.getWiFiStatus() == WL_CONNECTED) {
            TelemetryData currentTelemetry = m_telemetry.getTelemetryData();
            time_t timeStamp = m_time.getTimeStamp();
            if (m_database.sendDBData(currentTelemetry, timeStamp)) {
                m_lastDBSendTime = millis();
            }
        }
    }
}