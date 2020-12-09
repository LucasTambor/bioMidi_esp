#ifndef _H_R_APP_H_
#define _H_R_APP_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t xHeartRateDataMutex;

void vHeartRateTask( void *pvParameters );
esp_err_t heart_rate_app_read_data(float * data);

#endif //_H_R_APP_H_
