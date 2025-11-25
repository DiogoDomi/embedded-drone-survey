#ifndef _IMU_MANAGER_H_
#define _IMU_MANAGER_H_

#include "IMUData.h"
#include "MPU6050.h"

class IMUManager {
    private:
        IMUData m_swOffsets{};
        IMUData m_mpuData{};

        MPU6050 m_mpu{};

        volatile bool m_interrupt{};

        bool m_dmpReady{};
        uint8_t m_intStatus{};
        uint8_t m_devStatus{};
        uint16_t m_packetSize{};
        uint16_t m_fifoCount{};
        uint8_t m_fifoBuffer[64]{};

        Quaternion m_quaternion{};
        VectorFloat m_gravity{};
        VectorInt16 m_gyro{};
        float m_ypr[3]{};

        float m_gyroScaleDivisor{};

    private:

        static IMUManager* g_localInstance;
        static void IRAM_ATTR isr();

    private:

        bool readDMPData(IMUData& data);
        void setupConfig();
        void calibrateHardwareOffsets();
        void calibrateSoftwareOffsets();
        bool isDMPEnabled();

    public:

        IMUManager();
        void begin();
        void update();

        inline const IMUData& getMPUData() const { 
            return m_mpuData;
        }

};

#endif