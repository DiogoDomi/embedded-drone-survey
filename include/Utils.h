#ifndef UTILS_H_
#define UTILS_H_

#include <Arduino.h>
#include <cmath>

namespace Utils {

    template <typename T>
    inline T mMap(T x, T in_min, T in_max, T out_min, T out_max) {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

    template <typename T>
    inline T mConstrain(T x, T minVal, T maxVal) {
        if (x < minVal) { return minVal; }
        if (x > maxVal) { return maxVal; }
        return x;
    }
}

#endif