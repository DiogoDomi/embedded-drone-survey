#include "MPU6050_6Axis_MotionApps20.h"
#include "IMUManager.h"
#include "I2Cdev.h"
#include "Wire.h"
#include "Pins.h"

namespace {
    enum class Angles : uint8_t { YAW = 0, PITCH = 1, ROLL = 2 };

    constexpr uint8_t HARDWARE_LOOPS = 20;
    constexpr uint16_t SOFTWARE_LOOPS = 200;
}

void IRAM_ATTR IMUManager::isr() {
    if (g_localInstance) {
        g_localInstance->m_interrupt = true; 
    }
}

IMUManager* IMUManager::g_localInstance = nullptr;

IMUManager::IMUManager() :
    m_gyroScaleDivisor(16.4F)
{ 
    g_localInstance = this; 
}

void IMUManager::begin() {
    setupConfig();
    unsigned long timer = millis();
    while (millis() - timer < 2000) yield();
    calibrateHardwareOffsets();
    while(!isDMPEnabled()) { yield(); }
    calibrateSoftwareOffsets();
}

void IMUManager::setupConfig() {
    Wire.begin(Pins::MPU::SDA_PIN, Pins::MPU::SCL_PIN);
    Wire.setClock(400000L);
    m_mpu.initialize();
    pinMode(Pins::MPU::INTERRUPT_PIN, INPUT_PULLUP);

    // Valores capturados em: 25/11/2025 
    m_mpu.setXAccelOffset(-5008);
    m_mpu.setYAccelOffset(-1863);
    m_mpu.setZAccelOffset(178);
    m_mpu.setXGyroOffset(1031);
    m_mpu.setYGyroOffset(-125);
    m_mpu.setZGyroOffset(18);

    m_devStatus = m_mpu.dmpInitialize();
    m_mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_2000);
    m_mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
    m_mpu.setDLPFMode(MPU6050_DLPF_BW_42);
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
    attachInterrupt(digitalPinToInterrupt(Pins::MPU::INTERRUPT_PIN), isr, RISING);
    m_intStatus = m_mpu.getIntStatus();
    m_dmpReady = true;
    m_packetSize = m_mpu.dmpGetFIFOPacketSize();
    return true;
}

void IMUManager::calibrateSoftwareOffsets() {
    uint16_t successfulReads = 0;
    IMUData rawReading{};

    m_mpu.resetFIFO();
    m_mpu.getIntStatus();
    m_interrupt = false;

    for (uint16_t i = 0; i < SOFTWARE_LOOPS; i++) {
        unsigned long timeoutStart = millis();

        while(m_mpu.getFIFOCount() < m_packetSize) {
            yield();

            if (millis() - timeoutStart > 100) {
                break;
            }
        }

        if (readDMPData(rawReading)) {
            m_swOffsets.yaw += rawReading.yaw;
            m_swOffsets.pitch += rawReading.pitch;
            m_swOffsets.roll += rawReading.roll;
            m_swOffsets.gyroX += rawReading.gyroX;
            m_swOffsets.gyroY += rawReading.gyroY;
            m_swOffsets.gyroZ += rawReading.gyroZ;
            successfulReads++;
        }
    }

    if (successfulReads > 0) {
        m_swOffsets.yaw /= successfulReads;
        m_swOffsets.pitch /= successfulReads;
        m_swOffsets.roll /= successfulReads;
        m_swOffsets.gyroX /= successfulReads;
        m_swOffsets.gyroY /= successfulReads;
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
    }

    if ((m_intStatus & 0x02) && (m_fifoCount >= m_packetSize)) {
        while (m_fifoCount >= m_packetSize) {
            m_mpu.getFIFOBytes(m_fifoBuffer, m_packetSize);
            m_fifoCount -= m_packetSize;
        }

        m_mpu.dmpGetQuaternion(&m_quaternion, m_fifoBuffer);
        m_mpu.dmpGetGravity(&m_gravity, &m_quaternion);

        m_mpu.dmpGetYawPitchRoll(m_ypr, &m_quaternion, &m_gravity);

        m_mpu.dmpGetGyro(&m_gyro, m_fifoBuffer);

        data.yaw = m_ypr[static_cast<uint8_t>(Angles::YAW)] * 180/M_PI;
        data.pitch = m_ypr[static_cast<uint8_t>(Angles::PITCH)] * 180/M_PI;
        data.roll = m_ypr[static_cast<uint8_t>(Angles::ROLL)] * 180/M_PI;
        data.gyroX = static_cast<float>(m_gyro.x) / m_gyroScaleDivisor;
        data.gyroY = static_cast<float>(m_gyro.y) / m_gyroScaleDivisor;
        data.gyroZ = static_cast<float>(m_gyro.z) / m_gyroScaleDivisor;

        return true;
    }
    return false;
}

void IMUManager::update() {
    if (!m_interrupt) { 
        uint16_t currentFIFO = m_mpu.getFIFOCount();
        if (currentFIFO < m_packetSize) {
            return;
        }
    }

    m_interrupt = false;

    IMUData rawReading{};
    if (readDMPData(rawReading)) {
        float tmpYaw = rawReading.yaw - m_swOffsets.yaw;
        float tmpPitch = rawReading.pitch - m_swOffsets.pitch;
        float tmpRoll = rawReading.roll - m_swOffsets.roll;

        float tmpGyroX = rawReading.gyroX - m_swOffsets.gyroX;
        float tmpGyroY = rawReading.gyroY - m_swOffsets.gyroY;
        float tmpGyroZ = rawReading.gyroZ - m_swOffsets.gyroZ;

        m_mpuData.roll = tmpRoll;
        m_mpuData.gyroX = tmpGyroX;

        m_mpuData.pitch = tmpPitch;
        m_mpuData.gyroY = -tmpGyroY;

        m_mpuData.yaw = tmpYaw;
        m_mpuData.gyroZ = -tmpGyroZ;
    }
}