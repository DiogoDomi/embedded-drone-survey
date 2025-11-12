#ifndef SYSTEM_MANAGER_H_
#define SYSTEM_MANAGER_H_

#include "WiFiManager.h"
#include "WebManager.h"
#include "IMUManager.h"
#include "FlightManager.h"
#include "GPSManager.h"
#include "TelemetryManager.h"
// #include "DatabaseManager.h"
#include "TimeManager.h"

class SystemManager {
    private:
        AsyncWebServer m_server;
        AsyncWebSocket m_socket;

        IMUManager m_imu{};
        WiFiManager m_wifi{};
        GPSManager m_gps{};
        TimeManager m_time{};
        // DatabaseManager m_database{};

        FlightManager m_flight;
        WebManager m_web;
        TelemetryManager m_telemetry;

        unsigned long m_webPreviousTime{};
        // unsigned long m_dbPreviousTime{};

    private:

        SystemManager();
        SystemManager(const SystemManager&) = delete;
        void operator=(const SystemManager&) = delete;

    public:

        static SystemManager& getInstance();
        void setup();
        void loop();

};

#endif