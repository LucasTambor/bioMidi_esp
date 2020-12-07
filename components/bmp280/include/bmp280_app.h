#ifndef _BMP280_APP_H_
#define _BMP280_APP_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t xBMP280DataMutex;

typedef struct {
    float temperature;
    float pressure;
} bmp280_data_t;

void vBMP280Task( void *pvParameters );
esp_err_t bmp280_app_read_data(bmp280_data_t * data);

#endif //_BMP280_APP_H_
