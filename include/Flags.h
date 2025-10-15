#ifndef FLAGS_H_
#define FLAGS_H_

#include <cstdint>

namespace Flags {
    constexpr int8_t WIFI_RSSI_INVALID = -128;

    constexpr float GPS_INVALID_LOCATION = 999.0F;
    constexpr float GPS_INVALID_ALTITUDE = -9999.0F;
}

#endif