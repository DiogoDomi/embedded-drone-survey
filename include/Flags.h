#ifndef FLAGS_H_
#define FLAGS_H_

#include <cstdint>

namespace Flags {
    inline constexpr int8_t WIFI_RSSI_INVALID = -128;
    inline constexpr float GPS_INVALID_LOCATION = 999.0F;
    inline constexpr float GPS_INVALID_ALTITUDE = -9999.0F;
}

#endif