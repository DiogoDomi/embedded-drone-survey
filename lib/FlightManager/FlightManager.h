#ifndef FLIGHT_MANAGER_H_
#define FLIGHT_MANAGER_H_

#include "IMUManager.h"
#include "PIDManager.h"
#include "State.h"
#include "JoystickData.h"

class FlightManager {
    private:

        State m_currentState{};

        IMUManager& m_imu;

        PIDManager m_pidYaw;
        PIDManager m_pidPitch;
        PIDManager m_pidRoll;

        IMUData m_imuData{};

        uint16_t m_throttleMap{};
        float m_yawMap{};
        float m_pitchMap{};
        float m_rollMap{};

        float m_yawPidOutput{};
        float m_pitchPidOutput{};
        float m_rollPidOutput{};

        float m_lastPitch{};
        float m_lastRoll{};
        float m_lastGyroX{};
        float m_lastGyroY{};
        float m_lastGyroZ{};

        float m_actualPitch{};
        float m_actualRoll{};
        float m_actualGyroX{};
        float m_actualGyroY{};
        float m_actualGyroZ{};

        unsigned long m_previousTime{};
        float m_deltaTime{};

    private:
        // void calibrateESCs();
        // void printDebug();

        void setupMotors();
        void setMotorState();
        void processStateLogic(bool stateChangeRequested, const JoystickData& joystickData);
        void readSensors();
        void mapJoystick(const JoystickData& joystickData);
        void calculatePID();
        void writeMotors();

    public:

        FlightManager(IMUManager& imu);
        void begin();
        void update(bool stateChangeRequested, const JoystickData& joystickData);
        const inline State getStateData() const {
            return m_currentState;
        }

};

#endif