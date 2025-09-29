#ifndef IMU_DATA_H_
#define IMU_DATA_H_

struct IMUData {
    float deltaTime{}, pitch{}, roll{}, yaw{};
};

#endif