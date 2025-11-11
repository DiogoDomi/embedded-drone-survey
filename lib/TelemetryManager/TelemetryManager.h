#ifndef TELEMETRY_MANAGER_H_
#define TELEMETRY_MANAGER_H_

#include "WiFiManager.h"
#include "GPSManager.h"
#include "TelemetryData.h"
#include "FlightManager.h"
#include "TimeManager.h"

class TelemetryManager {
    private:

        WiFiManager& m_wifi;
        GPSManager& m_gps;
        FlightManager& m_flight;
        TimeManager& m_time;

        TelemetryData m_telemetry{};

    private:

        bool isTelemetryValid(const TelemetryData& telemetry) const;

    public:

        TelemetryManager(WiFiManager& wifi, GPSManager& gps, FlightManager& flight, TimeManager& time);
        void update();
        const TelemetryData& getTelemetry() const;
};

#endif