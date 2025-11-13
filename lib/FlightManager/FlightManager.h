#ifndef FLIGHT_MANAGER_H_
#define FLIGHT_MANAGER_H_

#include "IMUManager.h"
#include "PIDManager.h"
#include "State.h"
#include "JoystickData.h"

class FlightManager {
    private:

        IMUManager& m_imu;

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

        unsigned long m_previousDebugTime{};

        unsigned long m_previousTime{};
        float m_deltaTime{};

    private:
        // void calibrateESCs();
        float fmap(float x, float in_min, float in_max, float out_min, float out_max);

        void setupMotors();
        void setMotorState();
        void processStateLogic(bool stateChangeRequested, const JoystickData& joystickData);
        void readSensors();
        void mapJoystick(const JoystickData& joystickData);
        void calculatePID();
        void writeMotors();

        // void printDebug();

    public:

        FlightManager(IMUManager& imu);
        void begin();
        void update(bool stateChangeRequested, const JoystickData& joystickData);
        State getStateData() const;

};

#endif