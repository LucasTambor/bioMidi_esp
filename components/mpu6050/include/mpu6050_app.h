#ifndef _MPU6050_APP_H_
#define _MPU6050_APP_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t xMPU6050DataMutex;

typedef struct {
    float temperature;
    float accel_x;
    float accel_y;
    float accel_z;
    float gyr_x;
    float gyr_y;
    float gyr_z;
} mpu6050_data_t;

void vMPU6050Task( void *pvParameters );
esp_err_t mpu6050_app_read_data(mpu6050_data_t * data);

#endif //_MPU6050_APP_H_
