#include "SystemManager.h"

namespace {
    constexpr uint16_t TELEMETRY_INTERVAL = 5000;
    constexpr uint8_t GPS_INTERVAL = 20;
}

SystemManager& SystemManager::getInstance() {
    static SystemManager instance;
    return instance;
}

SystemManager::SystemManager() :   
    m_server(80),
    m_socket("/ws"),

    m_imu(),
    m_wifi(),
    m_gps(),
    m_time(),
    m_db(),

    m_flight(m_imu),
    m_web(m_server, m_socket),
    m_telemetry(m_wifi, m_gps, m_flight, m_time, m_db),

    m_telemetryPreviousTime(0),
    m_lastGPSLoop(0)
    {}

void SystemManager::setup() {
    m_flight.begin();
    m_telemetryPreviousTime = millis();
    while (millis() - m_telemetryPreviousTime < 4000) {
        yield();
    }

    m_imu.begin();

    m_telemetry.update();
    m_web.updateCache(m_telemetry.getTelemetry());

    m_wifi.begin();
    m_web.begin();

    // m_gps.begin();
    // m_time.begin();
    // m_db.begin();

    m_telemetryPreviousTime = 0L;
}

void SystemManager::loop() {
    // unsigned long currentTime = millis();

    m_web.update();

    bool hasStateChangeRequest = m_web.hasStateChangeRequest();
    JoystickData joystickData = m_web.getJoystickData();

    m_flight.update(hasStateChangeRequest, joystickData);

    // if (currentTime - m_lastGPSLoop >= GPS_INTERVAL) {
    //     m_gps.update();
    //     m_lastGPSLoop = currentTime;
    // }

    // bool sendTelemetry = (currentTime - m_telemetryPreviousTime >= TELEMETRY_INTERVAL);

    if (hasStateChangeRequest) {
        // m_telemetryPreviousTime = currentTime;

        // m_wifi.update();

        m_telemetry.update();

        TelemetryData telemetry = m_telemetry.getTelemetry();

        m_web.sendTelemetry(telemetry);

        // m_db.addTelemetry(telemetry);

        // if (m_wifi.getWiFiStatus() == WL_CONNECTED && 
        //     m_db.getRemainingLogs() == 0 &&
        //     m_flight.getStateData() == State::DISARMED) {

        //     m_db.flush();
        // }
    }

    yield();
}