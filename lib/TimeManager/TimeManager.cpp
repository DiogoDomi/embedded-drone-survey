#include "TimeManager.h"
#include <Arduino.h>

TimeManager::TimeManager() {};

void TimeManager::begin() {
    configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    m_timestamp = time(nullptr);
    while (m_timestamp < 100000) {
        delay(200);
        m_timestamp = time(nullptr);
    }
}

void TimeManager::update() {
    m_timestamp = time(nullptr);
}

time_t TimeManager::getTimeStamp() const { return m_timestamp; }