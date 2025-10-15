#include "IMUManager.h"
#include "I2Cdev.h"
#include "Wire.h"
#include "Pins.h"

namespace {
    IMUManager* g_pInstance = nullptr;

    enum class Angles : uint8_t { YAW = 0, PITCH = 1, ROLL = 2 };

    constexpr uint8_t HARDWARE_LOOPS = 10;
    constexpr uint16_t SOFTWARE_LOOPS = 200;

    constexpr float DMP_OUTPUT_RATE_HZ = 100.0F;
    constexpr float DMP_DELTA_TIME_SEC = 1.0F / DMP_OUTPUT_RATE_HZ;
}

void IRAM_ATTR imu_isr_wrapper() {
    if (g_pInstance) { g_pInstance->dmpDataReady(); }
}

void IMUManager::dmpDataReady() { m_interrupt = true; }

IMUManager::IMUManager() { g_pInstance = this; }

void IMUManager::begin() {
    setupConfig();
    calibrateHardwareOffsets();
    while(!isDMPEnabled()) { yield(); }
    calibrateSoftwareOffsets();
}

void IMUManager::setupConfig() {
    Wire.begin(Pins::MPU::SDA_PIN, Pins::MPU::SCL_PIN);
    Wire.setClock(400000);
    m_mpu.initialize();
    pinMode(Pins::MPU::INTERRUPT_PIN, INPUT);
    m_devStatus = m_mpu.dmpInitialize();
}

void IMUManager::calibrateHardwareOffsets() {
    m_mpu.CalibrateAccel(HARDWARE_LOOPS);
    m_mpu.CalibrateGyro(HARDWARE_LOOPS);
}

bool IMUManager::isDMPEnabled() {
    if (m_devStatus != 0) {
        return false;
    }
    m_mpu.setDMPEnabled(true);
    attachInterrupt(digitalPinToInterrupt(Pins::MPU::INTERRUPT_PIN), imu_isr_wrapper, RISING);
    m_intStatus = m_mpu.getIntStatus();
    m_dmpReady = true;
    m_packetSize = m_mpu.dmpGetFIFOPacketSize();
    return true;
}

void IMUManager::calibrateSoftwareOffsets() {
    uint16_t successfulReads = 0;
    IMUData rawReading{};

    for (uint16_t i = 0; i < SOFTWARE_LOOPS; i++) {
        while (!m_interrupt) { yield(); }

        if (readDMPData(rawReading)) {
            m_swOffsets.yaw += rawReading.yaw;
            m_swOffsets.pitch += rawReading.pitch;
            m_swOffsets.roll += rawReading.roll;
            m_swOffsets.gyroZ += rawReading.gyroZ;
            successfulReads++;
        }
    }
    if (successfulReads > 0) {
        m_swOffsets.yaw /= successfulReads;
        m_swOffsets.pitch /= successfulReads;
        m_swOffsets.roll /= successfulReads;
        m_swOffsets.gyroZ /= successfulReads;
    }
}

bool IMUManager::readDMPData(IMUData& data) {
    m_interrupt = false;
    m_intStatus = m_mpu.getIntStatus();
    m_fifoCount = m_mpu.getFIFOCount();

    if ((m_intStatus & 0x10) || m_fifoCount == 1024) {
        m_mpu.resetFIFO();
        return false;
    } else if (m_intStatus & 0x02) {
        if (m_fifoCount < m_packetSize) { return false; }

        m_mpu.getFIFOBytes(m_fifoBuffer, m_packetSize);

        m_mpu.dmpGetQuaternion(&m_quaternion, m_fifoBuffer);
        m_mpu.dmpGetGravity(&m_gravity, &m_quaternion);

        float ypr[3]{};
        m_mpu.dmpGetYawPitchRoll(ypr, &m_quaternion, &m_gravity);

        VectorInt16 gyro{};
        m_mpu.dmpGetGyro(&gyro, m_fifoBuffer);

        data.yaw = ypr[static_cast<uint8_t>(Angles::YAW)] * 180/M_PI;
        data.pitch = ypr[static_cast<uint8_t>(Angles::PITCH)] * 180/M_PI;
        data.roll = ypr[static_cast<uint8_t>(Angles::ROLL)] * 180/M_PI;
        data.gyroZ = static_cast<float>(gyro.z) / 131.0F;

        return true;
    }
    return false;
}

void IMUManager::update() {
    if (!m_interrupt) { return; }

    IMUData rawReading{};
    if (readDMPData(rawReading)) {
        m_mpuData.deltaTime = DMP_DELTA_TIME_SEC;

        m_mpuData.yaw = rawReading.yaw - m_swOffsets.yaw;
        m_mpuData.pitch = rawReading.pitch - m_swOffsets.pitch;
        m_mpuData.roll = rawReading.roll - m_swOffsets.roll;
        m_mpuData.gyroZ = rawReading.gyroZ - m_swOffsets.gyroZ;

        m_mpuData.pitch *= -1.00;
        m_mpuData.gyroZ *= -1.00;
    }
}

IMUData IMUManager::getMPUData() const { 
    noInterrupts();
    IMUData tempData = m_mpuData;
    interrupts();
    return tempData;
}