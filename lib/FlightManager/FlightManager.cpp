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
    constexpr float ROLL_PID_SCALE = 1.0F;

    constexpr uint16_t DEBUG_PRINT_INTERVAL = 200;
}

FlightManager::FlightManager(IMUManager& imu) :
    m_imu(imu),

    m_pidY(5.0F, 0.5F, 0.05F),
    m_pidP(1.20F, 0.0F, 0.23F),
    m_pidR(1.20F, 0.0F, 0.23F)
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
    m_motorBR.attach(Pins::ESC::MOTOR_BR_PIN);
    m_motorBL.attach(Pins::ESC::MOTOR_BL_PIN);
}

void FlightManager::setMotorState() {
    switch (m_currentState) {
        case State::DISARMED:
            m_motorFL.writeMicroseconds(Pwm::MIN);
            m_motorFR.writeMicroseconds(Pwm::MIN);
            m_motorBR.writeMicroseconds(Pwm::MIN);
            m_motorBL.writeMicroseconds(Pwm::MIN);
            break;
        case State::ARMED:
            m_motorFL.writeMicroseconds(Pwm::IDLE);
            m_motorFR.writeMicroseconds(Pwm::IDLE);
            m_motorBR.writeMicroseconds(Pwm::IDLE);
            m_motorBL.writeMicroseconds(Pwm::IDLE);
            break;
    }
}

void FlightManager::processStateLogic(bool stateChangeRequested, const JoystickData& joystickData) {
    if (!stateChangeRequested) { return; }

    if (joystickData.ly > (-ABS_JOYSTICK_RANGE + JOYSTICK_DEADZONE)) {
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
    m_imuData = m_imu.getMPUData();
}

void FlightManager::mapJoystick(const JoystickData& joystickData) {
    m_throttleMap = map(joystickData.ly, -ABS_JOYSTICK_RANGE, ABS_JOYSTICK_RANGE, Pwm::IDLE, Pwm::MAX_TEST);
    m_yawMap = fmap(joystickData.lx, -ABS_JOYSTICK_RANGE, ABS_JOYSTICK_RANGE, -Y_RATE, Y_RATE);
    m_pitchMap = fmap(joystickData.ry, -ABS_JOYSTICK_RANGE, ABS_JOYSTICK_RANGE, PR_ANGLE, -PR_ANGLE);
    m_rollMap = fmap(joystickData.rx, -ABS_JOYSTICK_RANGE, ABS_JOYSTICK_RANGE, -PR_ANGLE, PR_ANGLE);
}

void FlightManager::calculatePID() {
    if (m_imuData.deltaTime <= 0 ) { return; }

    m_yawPidOutput = m_pidY.compute(m_imuData.gyroZ, m_yawMap, m_imuData.deltaTime);
    m_pitchPidOutput = m_pidP.compute(m_imuData.pitch, m_pitchMap, m_imuData.deltaTime);
    m_rollPidOutput = m_pidR.compute(m_imuData.roll, m_rollMap, m_imuData.deltaTime);
}

void FlightManager::writeMotors() {
    float scaledYaw = m_yawPidOutput * YAW_PID_SCALE;
    float scaledPitch = m_pitchPidOutput * PITCH_PID_SCALE;
    float scaledRoll = m_rollPidOutput * ROLL_PID_SCALE;

    float motor_FL_F = static_cast<float>(m_throttleMap) - scaledYaw - scaledPitch + scaledRoll;
    float motor_FR_F = static_cast<float>(m_throttleMap) + scaledYaw - scaledPitch - scaledRoll;
    float motor_BR_F = static_cast<float>(m_throttleMap) - scaledYaw + scaledPitch - scaledRoll;
    float motor_BL_F = static_cast<float>(m_throttleMap) + scaledYaw + scaledPitch + scaledRoll;
    
    uint16_t motor_FL = static_cast<uint16_t>(constrain(motor_FL_F, Pwm::IDLE, Pwm::MAX_TEST));
    uint16_t motor_FR = static_cast<uint16_t>(constrain(motor_FR_F, Pwm::IDLE, Pwm::MAX_TEST));
    uint16_t motor_BR = static_cast<uint16_t>(constrain(motor_BR_F, Pwm::IDLE, Pwm::MAX_TEST));
    uint16_t motor_BL = static_cast<uint16_t>(constrain(motor_BL_F, Pwm::IDLE, Pwm::MAX_TEST));

    m_motorFL.writeMicroseconds(motor_FL);
    m_motorFR.writeMicroseconds(motor_FR);
    m_motorBR.writeMicroseconds(motor_BR);
    m_motorBL.writeMicroseconds(motor_BL);
}

void FlightManager::update(bool stateChangeRequested,const JoystickData& joystickData) {
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

    printDebug();
}

State FlightManager::getStateData() const { return m_currentState; }

void FlightManager::printDebug() {
    if (millis() - m_previousDebugTime > DEBUG_PRINT_INTERVAL) {
        m_previousDebugTime = millis();

        // Serial.print("M_FL : ");
        // Serial.println(m_motorFL.readMicroseconds());
        // Serial.print("M_FR : ");
        // Serial.println(m_motorFR.readMicroseconds());
        // Serial.print("M_BR : ");
        // Serial.println(m_motorBR.readMicroseconds());
        // Serial.print("M_BL : ");
        // Serial.println(m_motorBL.readMicroseconds());

        // Serial.print("Yaw : ");
        // Serial.println(m_imuData.yaw);
        // Serial.print("Pitch : ");
        // Serial.println(m_imuData.pitch);
        // Serial.print("Roll : ");
        // Serial.println(m_imuData.roll);

        // Serial.print("PID_Y : ");
        // Serial.println(m_yawPidOutput);
        // Serial.print("PID_P : ");
        // Serial.println(m_pitchPidOutput);
        // Serial.print("PID_R : ");
        // Serial.println(m_rollPidOutput);

        // Serial.println();
    }
}

void FlightManager::calibrateESCs() {
    setupMotors();
    delay(1000);

    m_motorFL.writeMicroseconds(Pwm::MAX);
    m_motorFR.writeMicroseconds(Pwm::MAX);
    m_motorBR.writeMicroseconds(Pwm::MAX);
    m_motorBL.writeMicroseconds(Pwm::MAX);
    delay(3000);

    m_motorFL.writeMicroseconds(Pwm::MIN);
    m_motorFR.writeMicroseconds(Pwm::MIN);
    m_motorBR.writeMicroseconds(Pwm::MIN);
    m_motorBL.writeMicroseconds(Pwm::MIN);
    delay(3000);
}

float FlightManager::fmap(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}