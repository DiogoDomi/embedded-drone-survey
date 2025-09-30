#ifndef FLIGHT_MANAGER_H_
#define FLIGHT_MANAGER_H_

#include <Servo.h>
#include "IMUManager.h"
#include "PIDManager.h"
#include "State.h"
#include "JoystickData.h"

class FlightManager {
    private:

        IMUManager& m_imu;

        Servo m_motorFL{};
        Servo m_motorFR{};
        Servo m_motorBL{};
        Servo m_motorBR{};

        PIDManager m_pidY;
        PIDManager m_pidP;
        PIDManager m_pidR;

        State m_currentState{};

        IMUData m_imuData{};

        uint16_t m_throttleMap{};
        float m_yawMap{};
        float m_pitchMap{};
        float m_rollMap{};

        float m_yawPidOutput{};
        float m_pitchPidOutput{};
        float m_rollPidOutput{};

        float m_previousYaw{};

        unsigned long m_previousDebugTime{};

    private:
        void calibrateESCs();
        float fmap(float x, float in_min, float in_max, float out_min, float out_max);

        void setupMotors();
        void setMotorState();
        void processStateLogic(bool stateChangeRequested, const JoystickData& joystickData);
        void readSensors();
        void mapJoystick(const JoystickData& joystickData);
        void calculatePID();
        void writeMotors();

        void printDebug();

    public:

        FlightManager(IMUManager& imu);
        void begin();
        void update(bool stateChangeRequested, const JoystickData& joystickData);
        State getState() const;

};

#endif