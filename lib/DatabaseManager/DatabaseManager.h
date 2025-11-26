#ifndef DATABASE_MANAGER_H_
#define DATABASE_MANAGER_H_

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include "TelemetryData.h"

class DatabaseManager {
    static const uint8_t JSON_TELEMETRY_SIZE = 180;
    static const uint8_t MAX_LOGS = 100;

    private:

        TelemetryData m_logs[MAX_LOGS]{};
        uint8_t m_logsCount{};

        WiFiClientSecure m_client{};
        HTTPClient m_http{};

    public:

        DatabaseManager();
        void begin();
        bool flush();

        inline bool addTelemetry(const TelemetryData& telemetry) {
            if (m_logsCount >= MAX_LOGS) {
                return false;
            }
            m_logs[m_logsCount] = telemetry;
            m_logsCount++;
            return true;
        }

        inline uint8_t getRemainingLogs() const {
            if (m_logsCount >= MAX_LOGS) return 0;
            return MAX_LOGS - m_logsCount;
        }

        inline bool isEmpty() const {
            return m_logsCount == 0;
        }

        inline void clearLogs() {
            m_logsCount = 0;
        }

};

#endif