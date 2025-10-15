#ifndef TELEMETRY_MANAGER_H_
#define TELEMETRY_MANAGER_H_

#include "WiFiManager.h"
#include "GPSManager.h"
#include "TelemetryData.h"

class TelemetryManager {
    private:

        WiFiManager& m_wifi;
        GPSManager& m_gps;

        TelemetryData m_currentTelemetry{};
        TelemetryData m_previousWebTelemetry{};
        TelemetryData m_previousDBTelemetry{};

    private:

        bool isTelemetryValid(const TelemetryData& telemetry) const;
        bool isTelemetryDifferent(const TelemetryData& current, const TelemetryData& previous) const;

    public:

        TelemetryManager(WiFiManager& wifi, GPSManager& gps);
        void update();
        bool shouldSendToWeb();
        bool shouldSendToBD();
        const TelemetryData& getTelemetryData() const;
};

#endif