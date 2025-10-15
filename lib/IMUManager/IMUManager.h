#ifndef _IMU_MANAGER_H_
#define _IMU_MANAGER_H_

#include "MPU6050_6Axis_MotionApps20.h"
#include "IMUData.h"

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
        IMUData getMPUData() const;

    public:
        void dmpDataReady();

};

#endif