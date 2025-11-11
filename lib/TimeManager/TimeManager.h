#ifndef TIME_MANAGER_H_
#define TIME_MANAGER_H_

#include <time.h>

class TimeManager {
    private:

        time_t m_timestamp{};
        unsigned long m_lastTimeCheck{};

    public:

        TimeManager();

        void begin();
        void update();
        time_t getTimestamp() const;

};

#endif