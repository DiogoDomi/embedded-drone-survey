#ifndef WEB_MANAGER_H_
#define WEB_MANAGER_H_

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "JoystickData.h"
#include "GPSData.h"
#include "State.h"
#include "TelemetryData.h"

class WebManager {
        static const uint8_t JSON_JOYSTICK_SIZE = 140;
        static const uint8_t JSON_TELEMETRY_SIZE = 160;

    private:

        AsyncWebServer& m_server;
        AsyncWebSocket& m_socket;

        TelemetryData m_telemetryCache{};
        JoystickData m_data{};
        bool m_stateChangeRequested{};

        StaticJsonDocument<JSON_JOYSTICK_SIZE> m_requestDoc{};

    private:

        void setupServer();
        void setupSocket();
        void onEventHandler(AsyncWebSocket* socket, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
        void handleWebSocketMessage(void* arg, uint8_t* data, size_t len);
        void onConnectSendTelemetry(AsyncWebSocketClient* client) const;
        void onConnectSendJoystickData(AsyncWebSocketClient* client) const;

    public:

        WebManager(AsyncWebServer& server, AsyncWebSocket& socket);
        void begin();
        void update();
        void cacheTelemetry(const GPSData& gps, int8_t rssi, State state);
        void sendTelemetry(const GPSData& gps, int8_t rssi, State state) const;
        bool hasStateChangeRequest();
        JoystickData getData() const;

};

#endif