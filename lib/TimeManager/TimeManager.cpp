#include "TimeManager.h"
#include <Arduino.h>

namespace {
    constexpr uint16_t TIME_UPDATE_INTERVAL = 2000;
}

TimeManager::TimeManager() :
    m_timestamp{0},
    m_lastTimeCheck{0}
    {}

void TimeManager::begin() {
    configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
}

void TimeManager::update() {
    unsigned long currentTime = millis();

    if (currentTime - m_lastTimeCheck >= TIME_UPDATE_INTERVAL) {
        m_lastTimeCheck = currentTime;
        m_timestamp = time(nullptr);
    }
}

time_t TimeManager::getTimestamp() const { return m_timestamp; }