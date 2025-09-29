#ifndef WEB_MANAGER_H_
#define WEB_MANAGER_H_

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "JoyData.h"
#include "GPSData.h"

class WebManager {
    private:

        AsyncWebServer& m_server;
        AsyncWebSocket& m_socket;

        JoyData m_data{};
        bool m_stateChangeRequest{};

        JsonDocument m_requestDoc{};

    private:

        void setupServer();
        void setupSocket();
        void onEventHandler(AsyncWebSocket* socket, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
        void handleWebSocketMessage(void* arg, uint8_t* data, size_t len);

    public:

        WebManager(AsyncWebServer& server, AsyncWebSocket& socket);
        void begin();
        void update();
        void sendTelemetry(const GPSData& gps, int8_t rssi);
        JoyData getData() const;
        bool consumeStateChangeRequest();

};

#endif