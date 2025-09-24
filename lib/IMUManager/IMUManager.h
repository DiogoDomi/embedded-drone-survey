#ifndef IMU_MANAGER_H_
#define IMU_MANAGER_H_

#include "IMUData.h"
#include <cstdint>

class IMUManager {
    private:

        struct RawIMUData {
            float accX{}, accY{}, accZ{};
            float gyrX{}, gyrY{}, gyrZ{};
        };

    private:

        unsigned long m_previousTime{};

        float m_offsetAccX{};
        float m_offsetAccY{};
        float m_offsetAccZ{};
        float m_offsetGyrX{};
        float m_offsetGyrY{};
        float m_offsetGyrZ{};

        uint8_t m_i2cBytes[14]{};

        IMUData m_data{};

    private:

        RawIMUData getRawData();
        void setupConfig();
        bool readData();
        bool calibrate();

    public:

        IMUManager();
        void begin();
        void update();
        IMUData getData();

};

#endif