#include "SystemManager.h"
#include <time.h>

namespace {
    constexpr uint16_t DB_SEND_INTERVAL = 5000;
    constexpr uint8_t WEB_SEND_INTERVAL = 100;
}

SystemManager::SystemManager() :   
    m_server(80),
    m_socket("/ws"),
    m_wifi(),
    m_web(m_server, m_socket),
    m_imu(),
    m_flight(m_imu),
    m_gps(),
    m_telemetry(m_wifi, m_gps, m_flight),
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
    delay(4000);
    m_imu.begin();
    m_wifi.begin();
    m_web.begin();
    m_time.begin();
    m_database.begin();

    m_telemetry.update();
    m_web.cacheTelemetry(m_telemetry.getTelemetry());
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

    unsigned long currentTime = millis();

    if (hasStateChangeRequest || currentTime - m_webPreviousTime >= WEB_SEND_INTERVAL) {
        m_webPreviousTime = currentTime;
        const TelemetryData telemetryData = m_telemetry.getTelemetry();
        m_web.cacheTelemetry(telemetryData);
        m_web.sendTelemetry(telemetryData);
    }

    if (m_wifi.getWiFiStatus() == WL_CONNECTED && m_time.getTimestamp() > 1000) {
        if (currentTime - m_dbPreviousTime >= DB_SEND_INTERVAL) {
            m_dbPreviousTime = currentTime;
            const TelemetryData telemetryData = m_telemetry.getTelemetry();
            const time_t timestamp = m_time.getTimestamp();
            m_database.sendDBData(telemetryData, timestamp);
        }
    }
}