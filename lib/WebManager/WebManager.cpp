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
    doc["state"] = static_cast<uint8_t>(m_telemetryCache.state);
    doc["rssi"] = m_telemetryCache.rssi;
    doc["lat"] = m_telemetryCache.gps.lat;
    doc["lon"] = m_telemetryCache.gps.lon;
    doc["alt"] = m_telemetryCache.gps.alt;

    char output[JSON_TELEMETRY_SIZE]{};
    serializeJson(doc, output);
    client->text(output);
}

void WebManager::onConnectSendJoystickData(AsyncWebSocketClient* client) const {
    StaticJsonDocument<JSON_JOYSTICK_SIZE> doc{};
    doc["lx"] = m_data.lx;
    doc["ly"] = m_data.ly;
    doc["rx"] = m_data.rx;
    doc["ry"] = m_data.ry;

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
        if (!m_requestDoc["lx"].isNull()) { m_data.lx = m_requestDoc["lx"]; }
        if (!m_requestDoc["ly"].isNull()) { m_data.ly = m_requestDoc["ly"]; }
        if (!m_requestDoc["rx"].isNull()) { m_data.rx = m_requestDoc["rx"]; }
        if (!m_requestDoc["ry"].isNull()) { m_data.ry = m_requestDoc["ry"]; }
    }
}

void WebManager::update() {
    m_socket.cleanupClients();
}

void WebManager::cacheTelemetry(const GPSData& gps, int8_t rssi, State state) {
    m_telemetryCache.state = state;
    m_telemetryCache.rssi = rssi;
    m_telemetryCache.gps = gps;
}

void WebManager::sendTelemetry(const GPSData& gps, int8_t rssi, State state) const {
    StaticJsonDocument<JSON_TELEMETRY_SIZE> doc{};
    doc["state"] = static_cast<uint8_t>(state);
    doc["rssi"] = rssi;
    doc["lat"] = gps.lat;
    doc["lon"] = gps.lon;
    doc["alt"] = gps.alt;

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

JoystickData WebManager::getData() const { return m_data; }