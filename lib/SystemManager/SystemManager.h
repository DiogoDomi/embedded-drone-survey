#ifndef SYSTEM_MANAGER_H_
#define SYSTEM_MANAGER_H_

class SystemManager;

class SystemManager {
    private:

        SystemManager();
        SystemManager(const SystemManager&) = delete;
        void operator=(const SystemManager&) = delete;

    public:

        static SystemManager& getInstance();
        void setup();
        void loop();

};

#endif