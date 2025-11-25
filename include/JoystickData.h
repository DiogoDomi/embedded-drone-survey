#ifndef JOYSTICK_DATA_H_
#define JOYSTICK_DATA_H_

#include <cstdint>

struct JoystickData {
    int8_t lx{};
    int8_t ly{};
    int8_t rx{};
    int8_t ry{};
};

#endif