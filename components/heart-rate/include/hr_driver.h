#ifndef _MPU6050_DRIVER_H_
#define _MPU6050_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include "driver/adc.h"


#ifdef __cplusplus
extern "C" {
#endif

#define HEART_RATE_ADC_CHANNEL      ADC1_CHANNEL_6

esp_err_t heart_rate_init();
esp_err_t heart_rate_read(uint16_t * data);

#ifdef __cplusplus
}
#endif

#endif //_MPU6050_DRIVER_H_