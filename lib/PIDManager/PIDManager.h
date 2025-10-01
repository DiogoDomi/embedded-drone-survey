#ifndef PID_MANAGER_H_
#define PID_MANAGER_H_

class PIDManager {
    private:
        const float m_kP{};
        const float m_kI{};
        const float m_kD{};

        float m_accumulatedError{};
        float m_previousError{};

    public:
        PIDManager(float kP, float kI, float kD);
        float compute(float realValue, float setpointValue, float deltaTime);
        void reset();
};

#endif