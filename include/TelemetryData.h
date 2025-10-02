#ifndef TELEMETRY_DATA_H_
#define TELEMETRY_DATA_H_

#include "GPSData.h"

struct TelemetryData {
    int8_t rssi{};
    GPSData gps{};
};

#endif