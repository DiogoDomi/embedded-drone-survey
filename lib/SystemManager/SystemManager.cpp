#include "SystemManager.h"

SystemManager::SystemManager() {}

SystemManager& SystemManager::getInstance() {
    static SystemManager instance;
    return instance;
} 

void SystemManager::setup() {}
void SystemManager::loop() {}