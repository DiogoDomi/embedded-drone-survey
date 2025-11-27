#include <Arduino.h>
#include "FlightManager.h"
#include "Pins.h"
#include "Utils.h"

namespace {
    namespace Pwm {
        constexpr uint16_t FREQUENCY = 400;
        constexpr uint16_t RANGE = 2500;

        constexpr uint16_t MAX = 2000;
        constexpr uint16_t MIN = 1000;
        constexpr uint16_t IDLE = 1100;
        constexpr uint16_t MAX_TEST = 1700;
    };

    constexpr uint8_t ABS_JOYSTICK_RANGE = 100;
    constexpr uint8_t JOYSTICK_DEADZONE = 3;

    constexpr float PR_ANGLE = 10.0F;
    constexpr float Y_RATE = 30.0F;

    constexpr float PR_CHANGE_PER_LOOP = 25.0F;
    constexpr float Y_CHANGE_PER_LOOP = 70.0F;

    constexpr float YAW_PID_SCALE = 5.0F;
    constexpr float PITCH_PID_SCALE = 5.0F;
    constexpr float ROLL_PID_SCALE = 5.0F;

    constexpr float MICROS_TO_SEC_MULTIPLIER = 0.000001F;

    // strongest motor pulls 150g with 1218us

    constexpr float FL_CORRECTION = 1229.0F/1218.0F;
    constexpr float FR_CORRECTION = 1220.0F/1218.0F;
    constexpr float BR_CORRECTION = 1218.0F/1218.0F;
    constexpr float BL_CORRECTION = 1218.0F/1218.0F;

    constexpr float JOY_TO_ANGLE_FACTOR = (PR_ANGLE * 2.0F) / 200.0F;
    constexpr float JOY_TO_RATE_FACTOR  = (Y_RATE * 2.0F) / 200.0F;
}

FlightManager::FlightManager(IMUManager& imu) :
    m_currentState(State::DISARMED),

    m_imu(imu),

    m_pidYaw(3.0F, 0.0F, 0.0F),
    m_pidPitch(1.0F, 0.0F, 0.05F),
    m_pidRoll(1.0F, 0.0F, 0.05F)
    {}

void FlightManager::begin() {
    setupMotors();
    m_currentState = State::DISARMED;
    setMotorState();
    m_previousTime = micros();
}

void FlightManager::setupMotors() {
    analogWriteFreq(Pwm::FREQUENCY);
    analogWriteRange(Pwm::RANGE);

    pinMode(Pins::ESC::MOTOR_FL_PIN, OUTPUT);
    pinMode(Pins::ESC::MOTOR_FR_PIN, OUTPUT);
    pinMode(Pins::ESC::MOTOR_BR_PIN, OUTPUT);
    pinMode(Pins::ESC::MOTOR_BL_PIN, OUTPUT);
}

void FlightManager::setMotorState() {
    switch (m_currentState) {
        case State::DISARMED:
            analogWrite(Pins::ESC::MOTOR_FL_PIN, Pwm::MIN);
            analogWrite(Pins::ESC::MOTOR_FR_PIN, Pwm::MIN);
            analogWrite(Pins::ESC::MOTOR_BR_PIN, Pwm::MIN);
            analogWrite(Pins::ESC::MOTOR_BL_PIN, Pwm::MIN);
            break;
        case State::ARMED:
            analogWrite(Pins::ESC::MOTOR_FL_PIN, Pwm::IDLE);
            analogWrite(Pins::ESC::MOTOR_FR_PIN, Pwm::IDLE);
            analogWrite(Pins::ESC::MOTOR_BR_PIN, Pwm::IDLE);
            analogWrite(Pins::ESC::MOTOR_BL_PIN, Pwm::IDLE);
            break;
    }
}

void FlightManager::processStateLogic(bool stateChangeRequested, const JoystickData& joystickData) {
    if (!stateChangeRequested) { return; }

    if (joystickData.ly > (-ABS_JOYSTICK_RANGE + JOYSTICK_DEADZONE)) { return; }

    switch (m_currentState) {
        case State::DISARMED:
            m_currentState = State::ARMED;
            break;
        case State::ARMED:
            m_currentState = State::DISARMED;
            break;
    }
    setMotorState();
}

void FlightManager::readSensors() {
    m_imu.update();
    m_imuData = m_imu.getMPUData();
}

void FlightManager::mapJoystick(const JoystickData& joystickData) {
    m_throttleMap = static_cast<uint16_t>(
        Utils::mMap(
            static_cast<float>(joystickData.ly),
            static_cast<float>(-ABS_JOYSTICK_RANGE),
            static_cast<float>(ABS_JOYSTICK_RANGE),
            static_cast<float>(Pwm::IDLE),
            static_cast<float>(Pwm::MAX_TEST)
        )
    );
    // m_yawMap   = static_cast<float>(joystickData.lx) * JOY_TO_RATE_FACTOR;
    m_yawMap   = 0.0F;
    m_pitchMap = static_cast<float>(joystickData.ry) * JOY_TO_ANGLE_FACTOR;
    m_rollMap  = static_cast<float>(joystickData.rx) * JOY_TO_ANGLE_FACTOR;
}

void FlightManager::calculatePID() {
    if (m_deltaTime <= 0.000001F) { return; }

    if (m_throttleMap < (Pwm::IDLE + 50)) {
        m_pidYaw.reset();
        m_pidPitch.reset();
        m_pidRoll.reset();
    }

    m_actualGyroX = m_imuData.gyroX;
    m_actualGyroY = m_imuData.gyroY;
    m_actualGyroZ = m_imuData.gyroZ;
    m_actualPitch = m_imuData.pitch;
    m_actualRoll  = m_imuData.roll;

    if (std::abs(m_actualGyroX - m_lastGyroX) > Y_CHANGE_PER_LOOP) {
        m_actualGyroX = m_lastGyroX;
    } else {
        m_lastGyroX = m_actualGyroX;
    }
    if (std::abs(m_actualGyroY - m_lastGyroY) > Y_CHANGE_PER_LOOP) {
        m_actualGyroY = m_lastGyroY;
    } else {
        m_lastGyroY = m_actualGyroY;
    }
    if (std::abs(m_actualGyroZ - m_lastGyroZ) > Y_CHANGE_PER_LOOP) {
        m_actualGyroZ = m_lastGyroZ;
    } else {
        m_lastGyroZ = m_actualGyroZ;
    }
    if (std::abs(m_actualPitch - m_lastPitch) > PR_CHANGE_PER_LOOP) {
        m_actualPitch = m_lastPitch;
    } else {
        m_lastPitch = m_actualPitch;
    }
    if (std::abs(m_actualRoll - m_lastRoll) > PR_CHANGE_PER_LOOP) {
        m_actualRoll = m_lastRoll;
    } else {
        m_lastRoll = m_actualRoll;
    }

    m_yawPidOutput = m_pidYaw.compute(m_actualGyroZ, m_yawMap, m_actualGyroZ, m_deltaTime);
    m_pitchPidOutput = m_pidPitch.compute(m_actualPitch, m_pitchMap, m_actualGyroY, m_deltaTime);
    m_rollPidOutput = m_pidRoll.compute(m_actualRoll, m_rollMap, m_actualGyroX, m_deltaTime);
}

void FlightManager::writeMotors() {
    float scaledYaw = m_yawPidOutput * YAW_PID_SCALE;
    float scaledPitch = m_pitchPidOutput * PITCH_PID_SCALE;
    float scaledRoll = m_rollPidOutput * ROLL_PID_SCALE;

    float motor_FL_F = (static_cast<float>(m_throttleMap) * FL_CORRECTION) - scaledPitch + scaledRoll - scaledYaw;
    float motor_FR_F = (static_cast<float>(m_throttleMap) * FR_CORRECTION) - scaledPitch - scaledRoll + scaledYaw;
    float motor_BR_F = (static_cast<float>(m_throttleMap) * BR_CORRECTION) + scaledPitch - scaledRoll - scaledYaw;
    float motor_BL_F = (static_cast<float>(m_throttleMap) * BL_CORRECTION) + scaledPitch + scaledRoll + scaledYaw;
    
    uint16_t motor_FL = static_cast<uint16_t>(Utils::mConstrain(motor_FL_F, static_cast<float>(Pwm::IDLE), static_cast<float>(Pwm::MAX)));
    uint16_t motor_FR = static_cast<uint16_t>(Utils::mConstrain(motor_FR_F, static_cast<float>(Pwm::IDLE), static_cast<float>(Pwm::MAX)));
    uint16_t motor_BR = static_cast<uint16_t>(Utils::mConstrain(motor_BR_F, static_cast<float>(Pwm::IDLE), static_cast<float>(Pwm::MAX)));
    uint16_t motor_BL = static_cast<uint16_t>(Utils::mConstrain(motor_BL_F, static_cast<float>(Pwm::IDLE), static_cast<float>(Pwm::MAX)));

    analogWrite(Pins::ESC::MOTOR_FL_PIN, motor_FL);
    analogWrite(Pins::ESC::MOTOR_FR_PIN, motor_FR);
    analogWrite(Pins::ESC::MOTOR_BR_PIN, motor_BR);
    analogWrite(Pins::ESC::MOTOR_BL_PIN, motor_BL);
}

void FlightManager::update(bool stateChangeRequested,const JoystickData& joystickData) {
    unsigned long currentTime = micros();
    m_deltaTime = (currentTime - m_previousTime) * MICROS_TO_SEC_MULTIPLIER;
    m_previousTime = currentTime;
    
    readSensors();

    processStateLogic(stateChangeRequested, joystickData);
    if (m_currentState != State::ARMED) {
        m_pidYaw.reset();
        m_pidPitch.reset();
        m_pidRoll.reset();
        return;
    }

    mapJoystick(joystickData);
    calculatePID();
    writeMotors();
}

// void FlightManager::printDebug() {
//     static unsigned long previousDebugTime = 0L;
//     if (millis() - previousDebugTime > 50) {
//         previousDebugTime = millis();

//         Serial.print(">Pitch : ");Serial.println(m_imuData.pitch);
//         Serial.print(">GyroY : ");Serial.println(m_imuData.gyroY);
//         Serial.print(">PitchMap: ");Serial.println(m_pitchMap);
//         Serial.print(">PID_P : ");Serial.println(m_pitchPidOutput);

//         Serial.print(">Roll : ");Serial.println(m_imuData.roll);
//         Serial.print(">GyroX : ");Serial.println(m_imuData.gyroX);
//         Serial.print(">RollMap: ");Serial.println(m_rollMap);
//         Serial.print(">PID_R : ");Serial.println(m_rollPidOutput);

//         Serial.print(">Yaw : ");Serial.println(m_imuData.yaw);
//         Serial.print(">GyroZ : ");Serial.println(m_imuData.gyroZ);
//         Serial.print(">YawMap: ");Serial.println(m_yawMap);
//         Serial.print(">PID_Y : ");Serial.println(m_yawPidOutput);
//     }
// }

// void FlightManager::calibrateESCs() {
//     setupMotors();
//     Serial.println("Enviando sinal MÁXIMO (Pwm::MAX)...");
//     m_motorFL.writeMicroseconds(Pwm::MAX);
//     m_motorFR.writeMicroseconds(Pwm::MAX);
//     m_motorBR.writeMicroseconds(Pwm::MAX);
//     m_motorBL.writeMicroseconds(Pwm::MAX);
//     delay(8000);
//     m_motorFL.writeMicroseconds(Pwm::MIN);
//     m_motorFR.writeMicroseconds(Pwm::MIN);
//     m_motorBR.writeMicroseconds(Pwm::MIN);
//     m_motorBL.writeMicroseconds(Pwm::MIN);
// }