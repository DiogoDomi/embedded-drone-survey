#ifndef STATE_H_
#define STATE_H_

#include <cstdint>

enum class State : uint8_t {
    IDLE = 0,
    ARMED = 1,
    DISARMED = 2
};

#endif