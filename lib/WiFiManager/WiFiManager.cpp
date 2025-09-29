#include "WiFiManager.h"
#include <ESP8266WiFi.h>

namespace {
    const char *AP_SSID = "DroneController";
    const char *AP_PSWD = "12345678";

    const IPAddress AP_LOCAL_IP(192, 168, 4, 200);
    const IPAddress AP_GATEWAY(192, 168, 4, 1);
    const IPAddress AP_SUBNET(255, 255, 255, 0);

    const char *STA_SSID = "LEO_2_4G";
    const char *STA_PSWD = "Leovegildocesar1#";

    constexpr uint16_t RSSI_UPDATE_INTERVAL = 2000;
    constexpr uint16_t RECONNECT_INTERVAL = 15000;
}

WiFiManager::WiFiManager() {}

void WiFiManager::begin() {
    WiFi.mode(WiFiMode_t::WIFI_AP_STA);
    setupAP();
    setupSTA();
}

void WiFiManager::setupAP() {
    WiFi.softAPConfig(AP_LOCAL_IP, AP_GATEWAY, AP_SUBNET); 
    WiFi.softAP(AP_SSID, AP_PSWD);
}

void WiFiManager::setupSTA() {
    WiFi.begin(STA_SSID, STA_PSWD);
    m_lastReconAttempt = millis();
}

void WiFiManager::update() {
    m_staStatus = WiFi.status();

    if (m_staStatus == WL_CONNECTED) {
        if (millis() - m_lastRssiCheck > RSSI_UPDATE_INTERVAL) {
            m_rssi = WiFi.RSSI();
            m_lastRssiCheck = millis();
        }
    } else {
        m_rssi = 0;

        if (millis() - m_lastReconAttempt > RECONNECT_INTERVAL) {
            WiFi.reconnect();
            m_lastReconAttempt = millis();
        }
    }
}

int8_t WiFiManager::getRSSI() const {
    return m_rssi;
}

bool WiFiManager::isStaConnected() const {
    return m_staStatus == WL_CONNECTED;
}