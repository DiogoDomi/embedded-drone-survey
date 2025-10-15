#ifndef SYSTEM_MANAGER_H_
#define SYSTEM_MANAGER_H_

#include "WiFiManager.h"
#include "WebManager.h"
#include "IMUManager.h"
#include "FlightManager.h"
#include "GPSManager.h"
#include "TelemetryManager.h"
#include "DatabaseManager.h"
#include "TimeManager.h"

class SystemManager {
    private:
        AsyncWebServer m_server;
        AsyncWebSocket m_socket;

        WiFiManager m_wifi{};
        WebManager m_web;
        IMUManager m_imu{};
        FlightManager m_flight;
        GPSManager m_gps{};
        TelemetryManager m_telemetry;
        DatabaseManager m_database{};
        TimeManager m_time{};

    private:

        void sendTelemetryToDatabase();
        unsigned long m_lastDBSendTime{};

        SystemManager();
        SystemManager(const SystemManager&) = delete;
        void operator=(const SystemManager&) = delete;

    public:

        static SystemManager& getInstance();
        void setup();
        void loop();

};

#endif