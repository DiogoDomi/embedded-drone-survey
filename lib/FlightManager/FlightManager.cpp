#include <Arduino.h>
#include "FlightManager.h"
#include "Pins.h"

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
    constexpr float Y_RATE = 50.0F;

    constexpr float PR_CHANGE_PER_LOOP = 25.0F;
    constexpr float Y_CHANGE_PER_LOOP = 70.0F;

    constexpr float YAW_PID_SCALE = 1.0F;
    constexpr float PITCH_PID_SCALE = 1.0F;
    constexpr float ROLL_PID_SCALE = 1.0F;

    constexpr uint8_t DEBUG_PRINT_INTERVAL = 1;

    constexpr float MICROS_TO_SEC_DIVIDER = 1000000.0F;

    // strongest motor pulls 150g with 1218us

    constexpr float FL_CORRECTION = 1229.0F/1218.0F;
    constexpr float FR_CORRECTION = 1220.0F/1218.0F;
    constexpr float BR_CORRECTION = 1218.0F/1218.0F;
    constexpr float BL_CORRECTION = 1218.0F/1218.0F;
}

FlightManager::FlightManager(IMUManager& imu) :
    m_imu(imu),

    m_pidY(0.0F, 0.0F, 0.0F),
    m_pidP(0.4F, 0.0F, 0.0F),
    m_pidR(0.3F, 0.0F, 0.0F)
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
    m_imuData = m_imu.getMPUData();
}

void FlightManager::mapJoystick(const JoystickData& joystickData) {
    m_throttleMap = map(joystickData.ly, -ABS_JOYSTICK_RANGE, ABS_JOYSTICK_RANGE, Pwm::IDLE, Pwm::MAX_TEST);
    // m_yawMap = fmap(joystickData.lx, -ABS_JOYSTICK_RANGE, ABS_JOYSTICK_RANGE, -Y_RATE, Y_RATE);
    m_yawMap = 0.0F;
    m_pitchMap = fmap(joystickData.ry, -ABS_JOYSTICK_RANGE, ABS_JOYSTICK_RANGE, -PR_ANGLE, PR_ANGLE);
    m_rollMap = fmap(joystickData.rx, -ABS_JOYSTICK_RANGE, ABS_JOYSTICK_RANGE, -PR_ANGLE, PR_ANGLE);
}

void FlightManager::calculatePID() {
    if (m_deltaTime <= 0) { return; }
    m_actualGyroZ = m_imuData.gyroZ;
    m_actualPitch = m_imuData.pitch;
    m_actualRoll = m_imuData.roll;

    if (abs(m_actualGyroZ - m_lastGyroZ) > Y_CHANGE_PER_LOOP) {
        m_actualGyroZ = m_lastGyroZ;
    } else {
        m_lastGyroZ = m_actualGyroZ;
    }
    if (abs(m_actualPitch - m_lastPitch) > PR_CHANGE_PER_LOOP) {
        m_actualPitch = m_lastPitch;
    } else {
        m_lastPitch = m_actualPitch;
    }
    if (abs(m_actualRoll - m_lastRoll) > PR_CHANGE_PER_LOOP) {
        m_actualRoll = m_lastRoll;
    } else {
        m_lastRoll = m_actualRoll;
    }

    m_yawPidOutput = m_pidY.compute(m_actualGyroZ, m_yawMap, m_deltaTime);
    m_pitchPidOutput = m_pidP.compute(m_actualPitch, m_pitchMap, m_deltaTime);
    m_rollPidOutput = m_pidR.compute(m_actualRoll, m_rollMap, m_deltaTime);
}

void FlightManager::writeMotors() {
    float scaledYaw = m_yawPidOutput * YAW_PID_SCALE;
    float scaledPitch = m_pitchPidOutput * PITCH_PID_SCALE;
    float scaledRoll = m_rollPidOutput * ROLL_PID_SCALE;

    float motor_FL_F = (static_cast<float>(m_throttleMap) * FL_CORRECTION) - scaledPitch + scaledRoll - scaledYaw;
    float motor_FR_F = (static_cast<float>(m_throttleMap) * FR_CORRECTION) - scaledPitch - scaledRoll + scaledYaw;
    float motor_BR_F = (static_cast<float>(m_throttleMap) * BR_CORRECTION) + scaledPitch - scaledRoll - scaledYaw;
    float motor_BL_F = (static_cast<float>(m_throttleMap) * BL_CORRECTION) + scaledPitch + scaledRoll + scaledYaw;
    
    uint16_t motor_FL = static_cast<uint16_t>(constrain(motor_FL_F, Pwm::IDLE, Pwm::MAX));
    uint16_t motor_FR = static_cast<uint16_t>(constrain(motor_FR_F, Pwm::IDLE, Pwm::MAX));
    uint16_t motor_BR = static_cast<uint16_t>(constrain(motor_BR_F, Pwm::IDLE, Pwm::MAX));
    uint16_t motor_BL = static_cast<uint16_t>(constrain(motor_BL_F, Pwm::IDLE, Pwm::MAX));

    analogWrite(Pins::ESC::MOTOR_FL_PIN, motor_FL);
    analogWrite(Pins::ESC::MOTOR_FR_PIN, motor_FR);
    analogWrite(Pins::ESC::MOTOR_BR_PIN, motor_BR);
    analogWrite(Pins::ESC::MOTOR_BL_PIN, motor_BL);
}

void FlightManager::update(bool stateChangeRequested,const JoystickData& joystickData) {
    unsigned long currentTime = micros();
    m_deltaTime = (currentTime - m_previousTime) / MICROS_TO_SEC_DIVIDER;
    m_previousTime = currentTime;
    
    readSensors();

    processStateLogic(stateChangeRequested, joystickData);
    if (m_currentState != State::ARMED) {
        m_pidY.reset();
        m_pidP.reset();
        m_pidR.reset();
        return;
    }

    mapJoystick(joystickData);
    calculatePID();
    writeMotors();

    // printDebug();
}

State FlightManager::getStateData() const { return m_currentState; }

// void FlightManager::printDebug() {
//     if (millis() - m_previousDebugTime > DEBUG_PRINT_INTERVAL) {
//         m_previousDebugTime = millis();

//         Serial.print("Pitch : ");
//         Serial.print(m_imuData.pitch);
//         Serial.print(" | PitchMap: ");
//         Serial.print(m_pitchMap);
//         Serial.print(" | PID_P : ");
//         Serial.println(m_pitchPidOutput);

//         Serial.print("Roll : ");
//         Serial.print(m_imuData.roll);
//         Serial.print(" | RollMap: ");
//         Serial.print(m_rollMap);
//         Serial.print(" | PID_R : ");
//         Serial.println(m_rollPidOutput);

//         Serial.print("Yaw : ");
//         Serial.print(m_imuData.yaw);
//         Serial.print(" | GyroZ : ");
//         Serial.print(m_imuData.gyroZ);
//         Serial.print(" | YawMap: ");
//         Serial.print(m_yawMap);
//         Serial.print(" | PID_Y : ");
//         Serial.println(m_yawPidOutput);

//         Serial.println("---------------------------------------------");
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

float FlightManager::fmap(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}