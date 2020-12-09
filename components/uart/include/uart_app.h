#ifndef _UART_APP_H_
#define _UART_APP_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern QueueHandle_t xQueueUartWriteBuffer;
extern QueueHandle_t xQueueUartStreamMicBuffer;
extern QueueHandle_t xQueueUartStreamFFTBuffer;
extern SemaphoreHandle_t xUartModeMutex;

typedef enum {
    UART_DATA_ID_ACCEL_X = 0,
    UART_DATA_ID_ACCEL_Y,
    UART_DATA_ID_ACCEL_Z,
    UART_DATA_ID_GYR_X,
    UART_DATA_ID_GYR_Y,
    UART_DATA_ID_GYR_Z,
    UART_DATA_ID_MIC,
    UART_DATA_ID_HEART_RATE,
    UART_DATA_ID_MAX
}uart_data_id_e;

typedef enum {
    UART_MODE_DATA_STREAM = 0,
    UART_MODE_MIC_STREAM,
    UART_MODE_FFT_STREAM,
}uart_mode_e;

typedef struct UART_data_s
{
    uart_data_id_e id;     // Data ID
    double data;           // Data value
}uart_data_t;

typedef struct UART_mic_data_s
{
    uart_data_id_e id;     // Data ID
    int32_t * data;        // Data value
    size_t len;            // Data lenght
}uart_mic_data_t;

typedef struct freq_s{
    const char * name;
    float value;
}freq_t;

typedef struct UART_mic_fft_s
{
    freq_t * freq;              // Freq info
    size_t len;                 // Data lenght
}uart_fft_data_t;

void vDataStream( void *pvParameters );
void uart_set_stream_mode(uart_mode_e new_mode);


#endif //_UART_APP_
