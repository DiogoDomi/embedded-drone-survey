#include "WiFiManager.h"
#include "WebManager.h"
#include "IMUManager.h"
#include "FlightManager.h"

WiFiManager wifi{};

AsyncWebServer server(80);
AsyncWebSocket socket("/ws");
WebManager web(server, socket);

IMUManager imu;
FlightManager flight(imu);

static unsigned long previousTelemetryTime = 0L;
constexpr unsigned long TELEMETRY_INTERVAL = 1000;

GPSData gpsData = {10.3983, -50.2392, 120.19};

void setup() {
    Serial.begin(115200);
    Serial.println();

    flight.begin();
    wifi.begin();
    web.begin();

    delay(500);
    
    web.cacheTelemetry(gpsData, wifi.getRSSI(), flight.getState());

    previousTelemetryTime = millis();
}

void loop() {
    web.update();
    wifi.update();

    JoystickData joystickData = web.getData();
    bool stateChangeRequested = web.hasStateChangeRequest();

    flight.update(stateChangeRequested, joystickData);

    if (stateChangeRequested || (millis() - previousTelemetryTime >= TELEMETRY_INTERVAL)) {
        previousTelemetryTime = millis();

        web.cacheTelemetry(gpsData, wifi.getRSSI(), flight.getState());
        web.sendTelemetry(gpsData, wifi.getRSSI(), flight.getState());
    }
}