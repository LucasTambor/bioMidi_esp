#ifndef _MIC_APP_H_
#define _MIC_APP_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t xMicDataStreamEnableMutex;

void vMic( void *pvParameters );

void mic_enable_data_stream();
void mic_disable_data_stream();

#endif //_MIC_APP_H_