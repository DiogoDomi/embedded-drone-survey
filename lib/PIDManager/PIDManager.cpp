#include "PIDManager.h"
#include "Arduino.h"

namespace { constexpr float MAX_ACCUMULATED_ERROR = 200.0F; }

PIDManager::PIDManager(float kP, float kI, float kD)
    :
    m_kP(kP),
    m_kI(kI),
    m_kD(kD)
    {}

float PIDManager::compute(float realValue, float setpointValue, float deltaTime) {
    float error = setpointValue - realValue;

    float P = m_kP * error;

    m_accumulatedError += error * deltaTime;
    m_accumulatedError = constrain(m_accumulatedError, -MAX_ACCUMULATED_ERROR, MAX_ACCUMULATED_ERROR);
    float I = m_kI * m_accumulatedError;

    float D = 0.0F;
    if (deltaTime > 0) {
        float derivatedError = (error - m_previousError) / deltaTime;
        D = m_kD * derivatedError;
    }

    m_previousError = error;

    return P + I + D;
}

void PIDManager::reset() {
    m_accumulatedError = 0.0F;
    m_previousError = 0.0F;
}