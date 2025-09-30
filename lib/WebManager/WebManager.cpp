#include "WebManager.h"
#include <LittleFS.h>

namespace { constexpr uint16_t JSON_SIZE = 256; }

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
            onConnectSendJoyData(client);
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

void WebManager::onConnectSendTelemetry(AsyncWebSocketClient* client) {
    char output[JSON_SIZE];
    JsonDocument doc;

    doc["state"] = static_cast<uint8_t>(m_telemetryCache.state);
    doc["rssi"] = m_telemetryCache.rssi;
    doc["lat"] = m_telemetryCache.gps.lat;
    doc["lon"] = m_telemetryCache.gps.lon;
    doc["alt"] = m_telemetryCache.gps.alt;

    serializeJson(doc, output, JSON_SIZE);

    client->text(output);
}

void WebManager::onConnectSendJoyData(AsyncWebSocketClient* client) {
    char output[JSON_SIZE];
    JsonDocument doc;

    doc["lx"] = m_data.lx;
    doc["ly"] = m_data.ly;
    doc["rx"] = m_data.rx;
    doc["ry"] = m_data.ry;

    serializeJson(doc, output, JSON_SIZE);

    client->text(output);
}

void WebManager::handleWebSocketMessage(void* arg, uint8_t* data, size_t len) {
    AwsFrameInfo* info = static_cast<AwsFrameInfo*>(arg);

    if (info->final && info->index == 0 && info->len == len && info->opcode == AwsFrameType::WS_TEXT) {
        data[len] = 0;
        char* msg = (char*)data;

        DeserializationError docHasError =  deserializeJson(m_requestDoc, msg);

        if (docHasError) { return; }

        if (!m_requestDoc["state"].isNull()) {
            m_stateChangeRequest = true; 
        }
        m_data.lx = m_requestDoc["lx"];
        m_data.ly = m_requestDoc["ly"];
        m_data.rx = m_requestDoc["rx"];
        m_data.ry = m_requestDoc["ry"];
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
    char output[JSON_SIZE]{};
    JsonDocument doc{};

    doc["state"] = static_cast<uint8_t>(state);
    doc["rssi"] = rssi;
    doc["lat"] = gps.lat;
    doc["lon"] = gps.lon;
    doc["alt"] = gps.alt;

    serializeJson(doc, output, JSON_SIZE);

    m_socket.textAll(output);
}

JoyData WebManager::getData() const {
    return m_data;
}

bool WebManager::consumeStateChangeRequest() {
    if (m_stateChangeRequest) {
        m_stateChangeRequest = false;
        return true;
    }
    return false;
}