#ifndef WEB_MANAGER_H_
#define WEB_MANAGER_H_

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "JoyData.h"
#include "GPSData.h"
#include "State.h"

class WebManager {
    private:
        struct TelemetryCache {
            State state{};
            uint8_t rssi{};
            GPSData gps{};
        };

    private:

        AsyncWebServer& m_server;
        AsyncWebSocket& m_socket;

        TelemetryCache m_telemetryCache{};
        JoyData m_data{};
        bool m_stateChangeRequest{};

        JsonDocument m_requestDoc{};

    private:

        void setupServer();
        void setupSocket();
        void onEventHandler(AsyncWebSocket* socket, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
        void handleWebSocketMessage(void* arg, uint8_t* data, size_t len);
        void onConnectSendTelemetry(AsyncWebSocketClient* client);
        void onConnectSendJoyData(AsyncWebSocketClient* client);

    public:

        WebManager(AsyncWebServer& server, AsyncWebSocket& socket);
        void begin();
        void update();
        void cacheTelemetry(const GPSData& gps, int8_t rssi, State state);
        void sendTelemetry(const GPSData& gps, int8_t rssi, State state) const;
        JoyData getData() const;
        bool consumeStateChangeRequest();

};

#endif