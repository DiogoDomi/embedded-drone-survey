#ifndef TELEMETRY_DATA_H_
#define TELEMETRY_DATA_H_

#include "GPSData.h"
#include <cstdint>
#include <time.h>

struct TelemetryData {
    time_t timestamp{};
    uint8_t state{};
    int8_t rssi{};
    GPSData gps{};
    bool isValid{};
};

#endif