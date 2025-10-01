#ifndef TELEMETRY_DATA_H_
#define TELEMETRY_DATA_H_

#include "GPSData.h"
#include "State.h"

struct TelemetryData {
    State state{};
    int8_t rssi{};
    GPSData gps{};
};

#endif