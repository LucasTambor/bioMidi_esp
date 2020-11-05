#ifndef _UART_APP_H_
#define _UART_APP_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern QueueHandle_t xQueueUartWriteBuffer;

typedef enum {
    UART_DATA_ID_ACCEL_X = 0,
    UART_DATA_ID_ACCEL_Y,
    UART_DATA_ID_ACCEL_Z,
    UART_DATA_ID_GYR_X,
    UART_DATA_ID_GYR_Y,
    UART_DATA_ID_GYR_Z,
    UART_DATA_ID_MIC,
    UART_DATA_ID_FFT,
    UART_DATA_ID_MAX
}uart_data_id_e;

typedef struct UART_data_s
{
    uart_data_id_e id;     // Data ID
    double data;           // Data value
}uart_data_t;

void vDataStream( void *pvParameters );


#endif //_UART_APP_
