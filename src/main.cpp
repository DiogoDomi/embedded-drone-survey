#include "SystemManager.h"

void setup() {
    SystemManager::getInstance().setup();
}

void loop() {
    SystemManager::getInstance().loop();
}