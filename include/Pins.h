#ifndef PINS_H_
#define PINS_H_

#include <cstdint>

namespace Pins {
    namespace MPU {
        // MPU6050 I2C & INT
        // constexpr uint8_t SCL_PIN = 5; // D1
        // constexpr uint8_t SDA_PIN = 4; // D2
        constexpr uint8_t INTERRUPT_PIN = 2; // D4
    }

    namespace GPS {
        // GPS RX/TX
        constexpr uint8_t RX_PIN = 0; // D3
        constexpr int8_t TX_PIN = -1; // Not used
    }

    namespace ESC {
        // ESCs PWM
        constexpr uint8_t MOTOR1_PIN = 14; // D5
        constexpr uint8_t MOTOR2_PIN = 12; // D6
        constexpr uint8_t MOTOR3_PIN = 13; // D7
        constexpr uint8_t MOTOR4_PIN = 15; // D8
    }
};

#endif