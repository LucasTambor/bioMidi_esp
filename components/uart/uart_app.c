#include "uart_app.h"
#include "uart_driver.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "UART_App";

QueueHandle_t xQueueUartWriteBuffer;
QueueHandle_t xQueueUartStreamMicBuffer;
QueueHandle_t xQueueUartStreamFFTBuffer;

SemaphoreHandle_t xUartModeMutex;

typedef struct {
    double data[UART_DATA_ID_MAX];
}data_stream_t;

static uart_mode_e mode = UART_MODE_DATA_STREAM;

// ******************************************************************************************************

void uart_set_stream_mode(uart_mode_e new_mode) {
    if(xSemaphoreTake(xUartModeMutex, portMAX_DELAY) == pdTRUE) {
        mode = new_mode;
        ESP_LOGI(TAG, "New Mode: %d", mode);
        xSemaphoreGive(xUartModeMutex);
    }
    // Clear Plotter
}
/**
 * Stream data in the format:
 * DATA_1,DATA_2,DATA_3
*/
void vDataStream( void *pvParameters ) {
    uart_data_t lReceivedValue;
    uart_mic_data_t lReceivedMicValue;
    uart_fft_data_t lReceivedFFTValue;

    // Init Queue
    xQueueUartWriteBuffer = xQueueCreate(16, sizeof(uart_data_t));
    xQueueUartStreamMicBuffer = xQueueCreate(16, sizeof(uart_data_t));
    xQueueUartStreamFFTBuffer = xQueueCreate(16, sizeof(uart_data_t));
    xUartModeMutex = xSemaphoreCreateMutex();
    if(xUartModeMutex == NULL) {
        ESP_LOGE(TAG, "ERROR Creating Mutex");
        return;
    }


    // Init UART
    ESP_ERROR_CHECK(uart_init());
    char data_buffer[256] = {0};
    data_stream_t data_packet;
    memset(&data_packet, 0, sizeof(data_stream_t));

    bool send_data = false;

    while(1) {
        switch(mode) {
            case UART_MODE_DATA_STREAM:
                while(uxQueueMessagesWaiting(xQueueUartWriteBuffer)) {
                    if( xQueueReceive( xQueueUartWriteBuffer, (void *)&lReceivedValue, portMAX_DELAY ) == pdPASS ) {
                        data_packet.data[lReceivedValue.id] = lReceivedValue.data;
                        send_data = true;
                    }
                }

                if(send_data) {
                    sprintf(data_buffer, "Accel_x:%2.2f,Accel_y:%2.2f,Accel_z:%2.2f,    \
                                        Gyro_x:%2.2f,Gyro_y:%2.2f,Gyro_z:%2.2f,         \
                                        MIC:%2.2f,FFT:%2.2f\n",
                                        data_packet.data[UART_DATA_ID_ACCEL_X],
                                        data_packet.data[UART_DATA_ID_ACCEL_Y],
                                        data_packet.data[UART_DATA_ID_ACCEL_Z],
                                        data_packet.data[UART_DATA_ID_GYR_X],
                                        data_packet.data[UART_DATA_ID_GYR_Y],
                                        data_packet.data[UART_DATA_ID_GYR_Z],
                                        data_packet.data[UART_DATA_ID_MIC],
                                        data_packet.data[UART_DATA_ID_FFT]);

                    if(!uart_write(data_buffer, strlen(data_buffer))) {
                        ESP_LOGE(TAG, "ERROR WRITING DATA");
                    }
                    // send_data = false;
                }

                vTaskDelay( 20 / portTICK_RATE_MS );
            break;
            case UART_MODE_MIC_STREAM:
                if( xQueueReceive( xQueueUartStreamMicBuffer, (void *)&lReceivedMicValue, portMAX_DELAY ) == pdPASS ) {
                    ESP_LOGD(TAG, "Received %d values from MIC", lReceivedMicValue.len);
                    if(lReceivedMicValue.len) {
                        for(uint32_t i = 0; i<lReceivedMicValue.len; i++) {
                            sprintf(data_buffer, "MIN:-100000,MAX:100000,MIC:%d\n", lReceivedMicValue.data[i]);
                            if(!uart_write(data_buffer, strlen(data_buffer))) {
                                ESP_LOGE(TAG, "ERROR WRITING DATA");
                                break;
                            }
                        }
                    }
                }
            break;
            case UART_MODE_FFT_STREAM:
                if( xQueueReceive( xQueueUartStreamFFTBuffer, (void *)&lReceivedFFTValue, portMAX_DELAY ) == pdPASS ) {
                    ESP_LOGD(TAG, "Received %d values from FFT", lReceivedFFTValue.len);

                    if(lReceivedFFTValue.len) {
                        sprintf(data_buffer, "31.5Hz:%2.2f,63Hz:%2.2f,94.5Hz:%2.2f,126Hz:%2.2f,257.5Hz:%2.2f\n",
                        lReceivedFFTValue.freq[0].value,
                        lReceivedFFTValue.freq[1].value,
                        lReceivedFFTValue.freq[2].value,
                        lReceivedFFTValue.freq[3].value,
                        lReceivedFFTValue.freq[4].value);

                        if(!uart_write(data_buffer, strlen(data_buffer))) {
                            ESP_LOGE(TAG, "ERROR WRITING DATA");
                            }
                    }
                }
            break;
            default:
                ESP_LOGE(TAG, "INVALID MODE");
            break;
        }
    }
}
