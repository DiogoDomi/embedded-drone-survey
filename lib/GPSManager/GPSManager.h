#ifndef GPS_MANAGER_H_
#define GPS_MANAGER_H_

#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include "GPSData.h"

class GPSManager {
    private:

        TinyGPSPlus m_gps{};
        SoftwareSerial m_swSerial{};

        GPSData m_data{};

    public:

        GPSManager();
        void begin();
        void update();
        GPSData getData() const;

};

#endif