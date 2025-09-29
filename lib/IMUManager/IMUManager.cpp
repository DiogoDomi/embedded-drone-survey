#include "IMUManager.h"
#include "I2Cdev.h"
#include "Wire.h"

namespace { IMUManager* g_pInstance = nullptr; }

void IRAM_ATTR imu_isr_wrapper() {
    if (g_pInstance) {
        g_pInstance->dmpDataReady();
    }
}

namespace {
    enum Axis : uint8_t { X = 0, Y = 1, Z = 2 };
    enum Angles : uint8_t { YAW = 0, PITCH = 1, ROLL = 2 };

    constexpr uint8_t INTERRUPT_PIN = 14;
    constexpr uint8_t HARDWARE_LOOPS = 10;
    constexpr uint16_t SOFTWARE_LOOPS = 200;
    constexpr float MICROS_TO_SEC = 1000000.0F;
}

IMUManager::IMUManager() { g_pInstance = this; }

void IMUManager::dmpDataReady() { m_interrupt = true; }

void IMUManager::begin() {
    setupConfig();
    calibrateHardwareOffsets();
    while(!isDMPEnabled()) { yield(); }
    calibrateSoftwareOffsets();
    m_previousTime = micros();
}

void IMUManager::setupConfig() {
    Wire.begin();
    Wire.setClock(400000);
    m_mpu.initialize();
    pinMode(INTERRUPT_PIN, INPUT);
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
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), imu_isr_wrapper, RISING);
    m_intStatus = m_mpu.getIntStatus();
    m_dmpReady = true;
    m_packetSize = m_mpu.dmpGetFIFOPacketSize();
    return true;
}

void IMUManager::calibrateSoftwareOffsets() {
    uint16_t successfulReads = 0;
    IMUData rawReading{};

    m_swOffsets = IMUData();

    for (uint16_t i = 0; i < SOFTWARE_LOOPS; i++) {
        while (!m_interrupt) { yield(); }

        if (readRawYPR(rawReading)) {
            m_swOffsets.yaw += rawReading.yaw;
            m_swOffsets.pitch += rawReading.pitch;
            m_swOffsets.roll += rawReading.roll;
            successfulReads++;
        }
    }
    if (successfulReads > 0) {
        m_swOffsets.yaw /= successfulReads;
        m_swOffsets.pitch /= successfulReads;
        m_swOffsets.roll /= successfulReads;
    }
}

bool IMUManager::readRawYPR(IMUData& data) {
    m_interrupt = false;
    m_intStatus = m_mpu.getIntStatus();
    m_fifoCount = m_mpu.getFIFOCount();

    if ((m_intStatus & 0x10) || m_fifoCount == 1024) {
        m_mpu.resetFIFO();
        return false;
    } else if (m_intStatus & 0x02) {
        if (m_fifoCount < m_packetSize) { return false; }
        m_mpu.getFIFOBytes(m_fifoBuffer, m_packetSize);
        m_fifoCount -= m_packetSize;

        m_mpu.dmpGetQuaternion(&m_quaternion, m_fifoBuffer);
        m_mpu.dmpGetGravity(&m_gravity, &m_quaternion);

        float ypr[3]{};
        m_mpu.dmpGetYawPitchRoll(ypr, &m_quaternion, &m_gravity);

        data.yaw = ypr[YAW] * 180/M_PI;
        data.pitch = ypr[PITCH] * 180/M_PI;
        data.roll = ypr[ROLL] * 180/M_PI;

        return true;
    }
    return false;
}

void IMUManager::update() {
    if (!m_interrupt) { return; }

    IMUData rawReading{};
    if (readRawYPR(rawReading)) {
        m_mpuData.deltaTime = static_cast<float>(micros() - m_previousTime) / MICROS_TO_SEC;
        m_previousTime = micros();

        m_mpuData.yaw = rawReading.yaw - m_swOffsets.yaw;
        m_mpuData.pitch = rawReading.pitch - m_swOffsets.pitch;
        m_mpuData.roll = rawReading.roll - m_swOffsets.roll;
    }
}

IMUData IMUManager::getData() const {
    return m_mpuData;
}