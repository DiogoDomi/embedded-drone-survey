#include "FlightManager.h"
#include "Pins.h"

namespace {
    namespace Pwm {
        constexpr uint16_t MAX = 2000;
        constexpr uint16_t MIN = 1000;
        constexpr uint16_t IDLE = 1100;
        constexpr uint16_t MAX_TEST = 1700;
    };

    constexpr uint8_t ABS_JOYSTICK_RANGE = 100;
    constexpr uint8_t JOYSTICK_DEADZONE = 3;

    constexpr float PR_ANGLE = 20.0F;
    constexpr float Y_RATE = 200.0F;

    constexpr float YAW_PID_SCALE = 1.0F;
    constexpr float PITCH_PID_SCALE = 1.0F;
    constexpr float ROLL_PID_SCALE = 2.0F;

    constexpr uint16_t DEBUG_PRINT_INTERVAL = 1000;
}

FlightManager::FlightManager(IMUManager& imu) :
    m_imu(imu),

    m_pidY(0.0F, 0.0F, 0.0F),
    m_pidP(0.0F, 0.0F, 0.0F),
    m_pidR(0.0F, 0.0F, 0.0F)
    {}

void FlightManager::begin() {
    m_imu.begin();
    setupMotors();

    m_currentState = State::DISARMED;
    setMotorState();
}

void FlightManager::setupMotors() {
    m_motorFL.attach(Pins::ESC::MOTOR_FL_PIN);
    m_motorFR.attach(Pins::ESC::MOTOR_FR_PIN);
    m_motorBL.attach(Pins::ESC::MOTOR_BL_PIN);
    m_motorBR.attach(Pins::ESC::MOTOR_BR_PIN);
}

void FlightManager::setMotorState() {
    switch (m_currentState) {
        case State::DISARMED:
            m_motorFL.writeMicroseconds(Pwm::MIN);
            m_motorFR.writeMicroseconds(Pwm::MIN);
            m_motorBL.writeMicroseconds(Pwm::MIN);
            m_motorBR.writeMicroseconds(Pwm::MIN);
            break;
        case State::ARMED:
            m_motorFL.writeMicroseconds(Pwm::IDLE);
            m_motorFR.writeMicroseconds(Pwm::IDLE);
            m_motorBL.writeMicroseconds(Pwm::IDLE);
            m_motorBR.writeMicroseconds(Pwm::IDLE);
            break;
    }
}

void FlightManager::processStateLogic(bool stateChangeRequest, const JoyData& joyData) {
    if (!stateChangeRequest) { return; }

    if (joyData.ly > (-ABS_JOYSTICK_RANGE + JOYSTICK_DEADZONE)) {
        return;
    }

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
    IMUData data = m_imu.getData();
    m_imuData = data;
}

void FlightManager::mapJoystick(const JoyData& joyData) {
    m_throttleMap = map(joyData.ly, -ABS_JOYSTICK_RANGE, ABS_JOYSTICK_RANGE, Pwm::IDLE, Pwm::MAX_TEST);
    m_yawMap = fmap(joyData.lx, -ABS_JOYSTICK_RANGE, ABS_JOYSTICK_RANGE, -Y_RATE, Y_RATE);
    m_pitchMap = fmap(joyData.ry, -ABS_JOYSTICK_RANGE, ABS_JOYSTICK_RANGE, -PR_ANGLE, PR_ANGLE);
    m_rollMap = fmap(joyData.rx, -ABS_JOYSTICK_RANGE, ABS_JOYSTICK_RANGE, -PR_ANGLE, PR_ANGLE);
}

void FlightManager::calculatePID() {
    float yawRate = (m_imuData.yaw - m_previousYaw) / m_imuData.deltaTime;
    m_previousYaw = m_imuData.yaw;

    m_yawPidOutput = m_pidY.compute(yawRate, m_yawMap, m_imuData.deltaTime);
    m_pitchPidOutput = m_pidP.compute(m_imuData.pitch, m_pitchMap, m_imuData.deltaTime);
    m_rollPidOutput = m_pidR.compute(m_imuData.roll, m_rollMap, m_imuData.deltaTime);
}

void FlightManager::writeMotors() {
    float scaledYaw = m_yawPidOutput * YAW_PID_SCALE;
    float scaledPitch = m_pitchPidOutput * PITCH_PID_SCALE;
    float scaledRoll = m_rollPidOutput * ROLL_PID_SCALE;

    float motor_FL_F = static_cast<float>(m_throttleMap) + scaledYaw - scaledPitch + scaledRoll;
    float motor_FR_F = static_cast<float>(m_throttleMap) - scaledYaw - scaledPitch - scaledRoll;
    float motor_BL_F = static_cast<float>(m_throttleMap) + scaledYaw + scaledPitch - scaledRoll;
    float motor_BR_F = static_cast<float>(m_throttleMap) - scaledYaw + scaledPitch + scaledRoll;

    uint16_t motor_FL = static_cast<uint16_t>(constrain(motor_FL_F, Pwm::IDLE, Pwm::MAX_TEST));
    uint16_t motor_FR = static_cast<uint16_t>(constrain(motor_FR_F, Pwm::IDLE, Pwm::MAX_TEST));
    uint16_t motor_BL = static_cast<uint16_t>(constrain(motor_BL_F, Pwm::IDLE, Pwm::MAX_TEST));
    uint16_t motor_BR = static_cast<uint16_t>(constrain(motor_BR_F, Pwm::IDLE, Pwm::MAX_TEST));

    m_motorFL.writeMicroseconds(motor_FL);
    m_motorFR.writeMicroseconds(motor_FR);
    m_motorBL.writeMicroseconds(motor_BL);
    m_motorBR.writeMicroseconds(motor_BR);
}

void FlightManager::update(const JoyData& joyData, bool stateChangeRequest) {
    readSensors();
    processStateLogic(stateChangeRequest, joyData);
    if (m_currentState != State::ARMED) {
        m_pidY.reset();
        m_pidP.reset();
        m_pidR.reset();
        return;
    }

    mapJoystick(joyData);
    calculatePID();
    writeMotors();
    printDebug();
}

State FlightManager::getState() const {
    return m_currentState;
}

void FlightManager::printDebug() {
    if (millis() - m_previousDebugTime > DEBUG_PRINT_INTERVAL) {
        m_previousDebugTime = millis();

        Serial.print("M_FL : ");
        Serial.println(m_motorFL.readMicroseconds());
        Serial.print("M_FR : ");
        Serial.println(m_motorFR.readMicroseconds());
        Serial.print("M_BL : ");
        Serial.println(m_motorBL.readMicroseconds());
        Serial.print("M_BR : ");
        Serial.println(m_motorBR.readMicroseconds());
    }
}

void FlightManager::calibrateESCs() {
    m_motorFL.writeMicroseconds(Pwm::MAX);
    m_motorFR.writeMicroseconds(Pwm::MAX);
    m_motorBL.writeMicroseconds(Pwm::MAX);
    m_motorBR.writeMicroseconds(Pwm::MAX);

    delay(1000);

    m_motorFL.writeMicroseconds(Pwm::MIN);
    m_motorFR.writeMicroseconds(Pwm::MIN);
    m_motorBL.writeMicroseconds(Pwm::MIN);
    m_motorBR.writeMicroseconds(Pwm::MIN);

    delay(1000);
}

float FlightManager::fmap(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}