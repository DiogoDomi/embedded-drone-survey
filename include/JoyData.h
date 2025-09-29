#ifndef JOY_DATA_H_
#define JOY_DATA_H_

#include <cstdint>

struct JoyData {
    int8_t lx{}, ly{}, rx{}, ry{};
};

#endif