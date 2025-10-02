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
            onConnectSendJoystickData(client);
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

void WebManager::onConnectSendTelemetry(AsyncWebSocketClient* client) const {
    StaticJsonDocument<JSON_TELEMETRY_SIZE> doc{};
    doc["state"] = static_cast<uint8_t>(m_cachedState);
    doc["rssi"] = m_cachedTelemetry.rssi;
    doc["lat"] = m_cachedTelemetry.gps.lat;
    doc["lon"] = m_cachedTelemetry.gps.lon;
    doc["alt"] = m_cachedTelemetry.gps.alt;

    char output[JSON_TELEMETRY_SIZE]{};
    serializeJson(doc, output);
    client->text(output);
}

void WebManager::onConnectSendJoystickData(AsyncWebSocketClient* client) const {
    StaticJsonDocument<JSON_JOYSTICK_SIZE> doc{};
    doc["lx"] = m_joystickData.lx;
    doc["ly"] = m_joystickData.ly;
    doc["rx"] = m_joystickData.rx;
    doc["ry"] = m_joystickData.ry;

    char output[JSON_JOYSTICK_SIZE]{};
    serializeJson(doc, output);
    client->text(output);
}

void WebManager::handleWebSocketMessage(void* arg, uint8_t* data, size_t len) {
    AwsFrameInfo* info = static_cast<AwsFrameInfo*>(arg);

    if (info->final && info->index == 0 && info->len == len && info->opcode == AwsFrameType::WS_TEXT) {
        data[len] = 0;
        char* msg = (char*)data;

        DeserializationError docHasError =  deserializeJson(m_requestDoc, msg);

        if (docHasError || m_requestDoc.isNull()) { return; }

        if (!m_requestDoc["state"].isNull()) { m_stateChangeRequested = true; }
        if (!m_requestDoc["lx"].isNull()) { m_joystickData.lx = m_requestDoc["lx"]; }
        if (!m_requestDoc["ly"].isNull()) { m_joystickData.ly = m_requestDoc["ly"]; }
        if (!m_requestDoc["rx"].isNull()) { m_joystickData.rx = m_requestDoc["rx"]; }
        if (!m_requestDoc["ry"].isNull()) { m_joystickData.ry = m_requestDoc["ry"]; }
    }
}

void WebManager::update() {
    m_socket.cleanupClients();
}

void WebManager::cacheTelemetry(const TelemetryData& telemetry, State state) {
    m_cachedState = state;
    m_cachedTelemetry = telemetry;
}

void WebManager::sendTelemetry(const TelemetryData& telemetry, State state) const {
    StaticJsonDocument<JSON_TELEMETRY_SIZE> doc{};
    doc["state"] = static_cast<uint8_t>(state);
    doc["rssi"] = telemetry.rssi;
    doc["lat"] = telemetry.gps.lat;
    doc["lon"] = telemetry.gps.lon;
    doc["alt"] = telemetry.gps.alt;

    char output[JSON_TELEMETRY_SIZE]{};
    serializeJson(doc, output);
    m_socket.textAll(output);
}

bool WebManager::hasStateChangeRequest() {
    if (m_stateChangeRequested) {
        m_stateChangeRequested = false;
        return true;
    }
    return false;
}

JoystickData WebManager::getJoystickData() const { return m_joystickData; }