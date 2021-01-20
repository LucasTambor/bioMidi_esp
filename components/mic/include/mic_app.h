#ifndef _MIC_APP_H_
#define _MIC_APP_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t xMicDataStreamEnableMutex;

void vMic( void *pvParameters );

#endif //_MIC_APP_H_