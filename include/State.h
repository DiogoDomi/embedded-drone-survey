#ifndef STATE_H_
#define STATE_H_

#include <cstdint>

enum class State : uint8_t {
    DISARMED = 0, ARMED = 1
};

#endif