#ifndef DATABASE_MANAGER_H_
#define DATABASE_MANAGER_H_

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "TelemetryData.h"
#include <time.h>

class DatabaseManager {
    private:

    WiFiClientSecure m_client{};
    HTTPClient m_http{};

    public:

        DatabaseManager();

        void begin();

        bool sendDBData(const TelemetryData& telemetry, time_t timestamp);

};

#endif