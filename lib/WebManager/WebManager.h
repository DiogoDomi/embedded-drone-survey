#ifndef WEB_MANAGER_H_
#define WEB_MANAGER_H_

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include "JoystickData.h"
#include "TelemetryData.h"

class WebManager {
    static const uint8_t JSON_JOYSTICK_SIZE = 50;
    static const uint8_t JSON_TELEMETRY_SIZE = 160;

    private:

        AsyncWebServer& m_server;
        AsyncWebSocket& m_socket;

        JoystickData m_joystickData{};
        volatile bool m_stateChangeRequested{};

        StaticJsonDocument<JSON_JOYSTICK_SIZE> m_requestDoc{};
        StaticJsonDocument<JSON_TELEMETRY_SIZE> m_serializeDoc{};
        char m_outputBuffer[JSON_TELEMETRY_SIZE]{};

    private:

        void setupServer();
        void setupSocket();
        void onEventHandler(AsyncWebSocket* socket, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
        void handleWebSocketMessage(void* arg, uint8_t* data, size_t len);
        void onConnectSendTelemetry(AsyncWebSocketClient* client);

    public:

        WebManager(AsyncWebServer& server, AsyncWebSocket& socket);
        void begin();

        inline void update() {
            m_socket.cleanupClients();
        }

        void cacheTelemetry(const TelemetryData& telemetry);
        void sendTelemetry(const TelemetryData& telemetry);

        inline bool hasStateChangeRequest() {
            noInterrupts();
            bool requested = m_stateChangeRequested;
            if (requested) {
                m_stateChangeRequested = false;
            }
            interrupts();
            return requested;
        }

        inline JoystickData getJoystickData() const { 
            noInterrupts();
            JoystickData tempData = m_joystickData;
            interrupts();
            return tempData;
        }

};

#endif