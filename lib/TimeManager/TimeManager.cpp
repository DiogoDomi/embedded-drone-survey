#include "TimeManager.h"
#include <Arduino.h>

TimeManager::TimeManager() :
    m_timestamp{0}
    {}

void TimeManager::begin() {
    configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
}

void TimeManager::update() {
    m_timestamp = time(nullptr);
}

uint32_t TimeManager::getTimestamp() const { return m_timestamp; }