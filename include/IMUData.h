#ifndef IMU_DATA_H_
#define IMU_DATA_H_

struct IMUData {
    float pitch{};
    float roll{};
    float yaw{};
    float gyroX{};
    float gyroY{};
    float gyroZ{};
};

#endif