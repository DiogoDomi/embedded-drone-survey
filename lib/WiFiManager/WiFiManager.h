#ifndef WIFI_MANAGER_H_
#define WIFI_MANAGER_H_

#include <cstdint>

class WiFiManager {
    private:

        int8_t m_rssi{};
        uint8_t m_staStatus{};
        unsigned long m_lastReconAttempt{};
        unsigned long m_lastRssiCheck{};

    private:

        void setupAP();
        void setupSTA();

    public:

        WiFiManager();
        void begin();
        void update();
        int8_t getRSSI() const;
        bool isStaConnected() const;

};

#endif