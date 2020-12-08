#ifndef _MPU6050_DRIVER_H_
#define _MPU6050_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MPU6050_SLAVE_ADDR          0x68

#define MPU6050_ACCEL_XOUT_H        0x3B
#define MPU6050_ACCEL_XOUT_L        0x3C
#define MPU6050_ACCEL_YOUT_H        0x3D
#define MPU6050_ACCEL_YOUT_L        0x3E
#define MPU6050_ACCEL_ZOUT_H        0x3F
#define MPU6050_ACCEL_ZOUT_L        0x40
#define MPU6050_TEMP_OUT_H          0x41
#define MPU6050_TEMP_OUT_L          0x42
#define MPU6050_GYRO_XOUT_H         0x43
#define MPU6050_GYRO_XOUT_L         0x44
#define MPU6050_GYRO_YOUT_H         0x45
#define MPU6050_GYRO_YOUT_L         0x46
#define MPU6050_GYRO_ZOUT_H         0x47
#define MPU6050_GYRO_ZOUT_L         0x48
#define MPU6050_PWR_MGMT_1          0x6B
#define MPU6050_ACCEL_CONFIG        0x1C
#define MPU6050_GYRO_CONFIG         0x1B
#define MPU6050_SMPLRT_DIV          0x19
#define MPU6050_CONFIG              0x1A

#define _2G_SCALE               0x00
#define _4G_SCALE               0x08
#define _8G_SCALE               0x10
#define _16G_SCALE              0x18

#define _250_DEGREE             0x00
#define _500_DEGREE             0x08
#define _1000_DEGREE            0x10
#define _2000_DEGREE            0x18

#define LSB_Sensitivity_2G      16384.0
#define LSB_Sensitivity_4G      8192.0
#define LSB_Sensitivity_8G      4096.0
#define LSB_Sensitivity_16G     2048.0

#define LSB_Sensitivity_250     131.0
#define LSB_Sensitivity_500     65.5
#define LSB_Sensitivity_1000    32.8
#define LSB_Sensitivity_2000    16.4

typedef struct {
    double x;
    double y;
    double z;
} mpu6050_accel_data_t;

typedef struct {
    double x;
    double y;
    double z;
} mpu6050_gyr_data_t;

esp_err_t mpu6050_init();
esp_err_t mpu6050_read_accel(mpu6050_accel_data_t * data);
esp_err_t mpu6050_read_gyr(mpu6050_gyr_data_t * data);
esp_err_t mpu6050_read_temp(double * data);

#ifdef __cplusplus
}
#endif

#endif //_MPU6050_DRIVER_H_