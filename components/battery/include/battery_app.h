#ifndef _BATTERY_APP_
#define _BATTERY_APP_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t xBatteryMeasurementDataMutex;

void vBatteryMeasurementTask( void *pvParameters );
esp_err_t battery_app_read_percent(uint8_t * data);
esp_err_t battery_app_read_voltage(uint16_t * data);

#endif //_BATTERY_APP_
