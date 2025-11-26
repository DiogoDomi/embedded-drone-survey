#include "WebManager.h"
#include <LittleFS.h>

WebManager::WebManager(AsyncWebServer& server, AsyncWebSocket& socket) :
    m_server(server),
    m_socket(socket)
    {}

void WebManager::begin() {
    LittleFS.begin();

    setupServer();
    setupSocket();

    m_server.begin();
}

void WebManager::setupServer() {
    m_server.on(
        "/",
        WebRequestMethod::HTTP_GET,
        [](AsyncWebServerRequest* request) {
            request->send(
                LittleFS,
                "/index.html",
                "text/html"
            );
        }
    );

    m_server.serveStatic("/", LittleFS, "/");
}

void WebManager::setupSocket() {
    m_socket.onEvent(
        [this](
            AsyncWebSocket* socket,
            AsyncWebSocketClient* client,
            AwsEventType type,
            void* arg,
            uint8_t* data,
            size_t len) {
                onEventHandler(socket, client, type, arg, data, len);
            }        
    );

    m_server.addHandler(&m_socket);
}

void WebManager::onEventHandler(AsyncWebSocket* socket, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case AwsEventType::WS_EVT_CONNECT:
            onConnectSendTelemetry(client);
            break;
        case AwsEventType::WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case AwsEventType::WS_EVT_PONG:
        case AwsEventType::WS_EVT_PING:
        case AwsEventType::WS_EVT_ERROR:
        case AwsEventType::WS_EVT_DISCONNECT:
            break;
    }
}

void WebManager::handleWebSocketMessage(void* arg, uint8_t* data, size_t len) {
    AwsFrameInfo* info = static_cast<AwsFrameInfo*>(arg);

    if (info->final && info->index == 0 && info->len == len && info->opcode == AwsFrameType::WS_TEXT) {
        m_requestDoc.clear();

        DeserializationError docHasError =  deserializeJson(m_requestDoc, data, len);

        if (docHasError) { return; }

        noInterrupts();

        if (!m_requestDoc["state"].isNull()) { m_stateChangeRequested = true; }
        if (!m_requestDoc["lx"].isNull()) { m_joystickData.lx = m_requestDoc["lx"]; }
        if (!m_requestDoc["ly"].isNull()) { m_joystickData.ly = m_requestDoc["ly"]; }
        if (!m_requestDoc["rx"].isNull()) { m_joystickData.rx = m_requestDoc["rx"]; }
        if (!m_requestDoc["ry"].isNull()) { m_joystickData.ry = m_requestDoc["ry"]; }

        interrupts();
    }
}

void WebManager::onConnectSendTelemetry(AsyncWebSocketClient* client) {
    if (!client || !client->canSend()) { return; }

    StaticJsonDocument<JSON_JOYSTICK_SIZE> doc{};

    doc["ly"] = m_joystickData.ly;
    char buffer[JSON_JOYSTICK_SIZE]{};

    serializeJson(doc, buffer, JSON_JOYSTICK_SIZE);
    client->text(buffer);
}

void WebManager::sendTelemetry(const TelemetryData& telemetry) {
    m_serializeDoc.clear();

    m_serializeDoc["st"]    = static_cast<int>(telemetry.state);
    // m_serializeDoc["mem"]   = telemetry.logsRemaining;
    // m_serializeDoc["rssi"]  = telemetry.rssi;
    // m_serializeDoc["lat"]   = telemetry.gps.lat;
    // m_serializeDoc["lon"]   = telemetry.gps.lon;
    // m_serializeDoc["alt"]   = telemetry.gps.alt;

    size_t len = serializeJson(m_serializeDoc, m_outputBuffer, JSON_TELEMETRY_SIZE);

    m_socket.textAll(m_outputBuffer, len);
}