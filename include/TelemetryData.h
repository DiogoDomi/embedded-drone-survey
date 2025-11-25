#ifndef TELEMETRY_DATA_H_
#define TELEMETRY_DATA_H_

#include "GPSData.h"
#include "State.h"
#include <cstdint>

struct TelemetryData {
    uint32_t timestamp{};
    State state{};
    int8_t rssi{};
    GPSData gps{};
    bool isValid{};
};

#endif