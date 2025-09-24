#include "IMUManager.h"
#include <Wire.h>
#include <cmath>

namespace {
    constexpr uint8_t MPU_ADDR = 0x68;
    constexpr uint8_t RESET_ADDR = 0x00;
    constexpr uint8_t POWER_ADDR = 0x6B;

    constexpr uint8_t FILTER_CONFIG = 0x1A;
    constexpr uint8_t GYRO_CONFIG = 0x1B;
    constexpr uint8_t ACCEL_CONFIG = 0x1C;

    constexpr uint8_t ACCEL_OUTPUT = 0x3B;
    constexpr uint8_t I2C_BYTES = 14;

    constexpr uint8_t GYRO_SCALE_250_DEG = 0x00;
    constexpr uint8_t GYRO_SCALE_500_DEG = 0x08;
    constexpr uint8_t GYRO_SCALE_1000_DEG = 0x10;
    constexpr uint8_t GYRO_SCALE_2000_DEG = 0x18;

    constexpr uint8_t ACCEL_SCALE_2_G = 0x00;
    constexpr uint8_t ACCEL_SCALE_4_G = 0x08;
    constexpr uint8_t ACCEL_SCALE_8_G = 0x10;
    constexpr uint8_t ACCEL_SCALE_16_G = 0x18;

    constexpr float GYRO_SENS_250_DEG = 131.0F;
    constexpr float GYRO_SENS_500_DEG = 65.5F;
    constexpr float GYRO_SENS_1000_DEG = 32.8F;
    constexpr float GYRO_SENS_2000_DEG = 16.4F;

    constexpr float ACCEL_SENS_2_G = 16384.0F;
    constexpr float ACCEL_SENS_4_G = 8192.0F;
    constexpr float ACCEL_SENS_8_G = 4096.0F;
    constexpr float ACCEL_SENS_16_G = 2048.0F;

    enum class FilterScale : uint8_t {
        HZ_260,
        HZ_184,
        HZ_94,
        HZ_44,
        HZ_21,
        HZ_10,
        HZ_5,
    };

    constexpr float MICROS_TO_SEC = 1000000.0F;
    constexpr float RAD_TO_DEG = 57.29578F;
    constexpr float ALPHA = 0.98F;

    constexpr uint16_t WAIT_TIME_US = 10000;
    constexpr uint16_t CALIB_MIN_READS = 500;
}

IMUManager::IMUManager() {}

void IMUManager::setupConfig() {
    Wire.begin(0, 5);

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(POWER_ADDR);
    Wire.write(RESET_ADDR);
    Wire.endTransmission(true);

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(FILTER_CONFIG);
    Wire.write(static_cast<uint8_t>(FilterScale::HZ_260));
    Wire.endTransmission(true);

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(GYRO_CONFIG);
    Wire.write(GYRO_SCALE_250_DEG);
    Wire.endTransmission(true);

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(ACCEL_CONFIG);
    Wire.write(ACCEL_SCALE_2_G);
    Wire.endTransmission(true);
}

bool IMUManager::readData() {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(ACCEL_OUTPUT);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, I2C_BYTES, static_cast<uint8_t>(true));

    if (Wire.available() != 14) {
        return false;
    }

    for (uint8_t i = 0; i < 14; i++) {
        m_i2cBytes[i] = Wire.read();
    }

    return true;
}

IMUManager::RawIMUData IMUManager::getRawData() {
    RawIMUData rawData;

    rawData.accX = static_cast<float>( static_cast<int16_t>( ( (m_i2cBytes[0] << 8) | m_i2cBytes[1]) ) ) / ACCEL_SENS_2_G;
    rawData.accY = static_cast<float>( static_cast<int16_t>( ( (m_i2cBytes[2] << 8) | m_i2cBytes[3]) ) ) / ACCEL_SENS_2_G;
    rawData.accZ = static_cast<float>( static_cast<int16_t>( ( (m_i2cBytes[4] << 8) | m_i2cBytes[5]) ) ) / ACCEL_SENS_2_G;
    rawData.gyrX = static_cast<float>( static_cast<int16_t>( ( (m_i2cBytes[8] << 8) | m_i2cBytes[9]) ) ) / GYRO_SENS_250_DEG;
    rawData.gyrY = static_cast<float>( static_cast<int16_t>( ( (m_i2cBytes[10] << 8) | m_i2cBytes[11]) ) ) / GYRO_SENS_250_DEG;
    rawData.gyrZ = static_cast<float>( static_cast<int16_t>( ( (m_i2cBytes[12] << 8) | m_i2cBytes[13]) ) ) / GYRO_SENS_250_DEG;

    return rawData;
}

bool IMUManager::calibrate() {
    constexpr uint16_t calibCounter = 1000;
    uint16_t successfulReads = 0;
    float accX = 0.0F, accY = 0.0F, accZ = 0.0F;
    float gyrX = 0.0F, gyrY = 0.0F, gyrZ = 0.0F;

    unsigned long lastLoop = micros();

    for (uint16_t i = 0; i < calibCounter; i++) {
        while (micros() - lastLoop <= WAIT_TIME_US);
        lastLoop = micros();
        if (readData()) {
            RawIMUData rawData = getRawData();

            accX += rawData.accX;
            accY += rawData.accY;
            accZ += rawData.accZ;
            gyrX += rawData.gyrX;
            gyrY += rawData.gyrY;
            gyrZ += rawData.gyrZ;

            successfulReads++;
        }
    }

    if (successfulReads >= CALIB_MIN_READS) {
        m_offsetAccX = accX / successfulReads;
        m_offsetAccY = accY / successfulReads;
        m_offsetAccZ = (accZ / successfulReads) - 1.0F;
        m_offsetGyrX = gyrX / successfulReads;
        m_offsetGyrY = gyrY / successfulReads;
        m_offsetGyrZ = gyrZ / successfulReads;

        return true;
    }

    return false;
}                                
                                 
void IMUManager::update() {
    if (!readData()) {
        return;
    }

    float accelPitch = 0.0F, accelRoll = 0.0F;

    float elapsedTime = static_cast<float>(micros() - m_previousTime) / MICROS_TO_SEC;
    m_previousTime = micros();

    RawIMUData rawData = getRawData();

    float accX = rawData.accX - m_offsetAccX;
    float accY = rawData.accY - m_offsetAccY;
    float accZ = rawData.accZ - m_offsetAccZ;
    float gyrX = rawData.gyrX - m_offsetGyrX;
    float gyrY = rawData.gyrY - m_offsetGyrY;
    float gyrZ = rawData.gyrZ - m_offsetGyrZ;

    accelPitch = atan2f(accY, accZ) * RAD_TO_DEG;
    accelRoll = atan2f(-accX, sqrt(accY*accY + accZ*accZ)) * RAD_TO_DEG;

    m_data.pitch = ALPHA * (m_data.pitch + gyrX * elapsedTime) + (1.0F - ALPHA) * accelPitch;
    m_data.roll = ALPHA * (m_data.roll + gyrY * elapsedTime) + (1.0F - ALPHA) * accelRoll;
    m_data.yaw += gyrZ * elapsedTime;
    m_data.elapsedTime = elapsedTime;
}

IMUData IMUManager::getData() {
    return m_data;
}

void IMUManager::begin() {
    setupConfig();
    while (!calibrate());
    m_previousTime = micros();
}