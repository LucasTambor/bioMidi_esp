#include "uart_app.h"
#include "uart_driver.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "UART_App";

QueueHandle_t xQueueUartWriteBuffer;

typedef struct {
    double data[UART_DATA_ID_MAX];
}data_stream_t;

/**
 * Stream data in the format:
 * DATA_1,DATA_2,DATA_3
*/
void vDataStream( void *pvParameters ) {
    uart_data_t lReceivedValue;
	portBASE_TYPE xStatus;

    // Init Queue
    xQueueUartWriteBuffer = xQueueCreate(16, sizeof(uart_data_t));

    // Init UART
    ESP_ERROR_CHECK(uart_init());
    char data_buffer[256] = {0};
    data_stream_t data_packet;
    memset(&data_packet, 0, sizeof(data_stream_t));

    bool send_data = false;
    while(1) {
        while(uxQueueMessagesWaiting(xQueueUartWriteBuffer)) {
            xStatus = xQueueReceive( xQueueUartWriteBuffer, (void *)&lReceivedValue, portMAX_DELAY );
            if( xStatus == pdPASS )
            {
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

            uart_write(data_buffer, strlen(data_buffer));
            send_data = false;
        }

        vTaskDelay( 100 / portTICK_RATE_MS );
    }
}
