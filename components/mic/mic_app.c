#include "mic_app.h"
#include "mic_driver.h"
#include "uart_app.h"
#include "esp_log.h"
#include "string.h"

SemaphoreHandle_t xMicDataStreamEnableMutex;

static bool mic_data_stream = false;
static const char *TAG = "mic_task";

void mic_enable_data_stream() {
    if(xSemaphoreTake(xMicDataStreamEnableMutex, portMAX_DELAY) == pdTRUE) {
        mic_data_stream = true;
        ESP_LOGI(TAG, "Enable data stream");
        xSemaphoreGive(xMicDataStreamEnableMutex);
    }
}

void mic_disable_data_stream() {
    if(xSemaphoreTake(xMicDataStreamEnableMutex, portMAX_DELAY) == pdTRUE) {
        mic_data_stream = false;
        ESP_LOGI(TAG, "Disable data stream");
        xSemaphoreGive(xMicDataStreamEnableMutex);
    }
}

void vMic( void *pvParameters ) {
    if(mic_init() != ESP_OK) {
        ESP_LOGE(TAG, "ERROR Initializing Mic Driver");
        return;
    }

    xMicDataStreamEnableMutex = xSemaphoreCreateMutex();
    if(xMicDataStreamEnableMutex == NULL) {
        ESP_LOGE(TAG, "ERROR Creating Mutex");
        return;
    }

    uint8_t sample_buffer[I2S_READ_BUFFER_SIZE] = {0};
    uint8_t sample_data = 0;

    uart_mic_data_t stream_data = {
        .data = NULL,
        .len = 0,
        .id = UART_DATA_ID_MIC
    };

    portBASE_TYPE xStatus;
    i2s_event_t evt;
    size_t bytes_read;
    while(1) {
        // Wait for I2S event
        if (xQueueReceive(xQueueI2S, &evt, portMAX_DELAY) == pdPASS)
        {
            if (evt.type == I2S_EVENT_RX_DONE) {
                do {
                    ESP_ERROR_CHECK(mic_read_buffer(sample_buffer, &bytes_read));
                    // Proccess data
                    int32_t *samples_32 = (int32_t *)sample_buffer;

                    for (int i = 0; i < bytes_read/4; i++) {
                        // you may need to vary the >> 11 to fit your volume - ideally we'd have some kind of AGC here
                        samples_32[i] = samples_32[i]>>8;
                    }
                    if(xSemaphoreTake(xMicDataStreamEnableMutex, portMAX_DELAY) == pdTRUE) {
                        if(mic_data_stream) {
                            stream_data.data = samples_32;
                            stream_data.len = bytes_read/4;
                            xStatus = xQueueSend( xQueueUartStreamMicBuffer, (void *)&stream_data, portMAX_DELAY );
                            if (xStatus == pdFAIL) {
                                ESP_LOGE(TAG, "ERROR sendig mic data to queue");
                            }
                        }
                    }
                    xSemaphoreGive(xMicDataStreamEnableMutex);


                }while(bytes_read > 0);
            }
        }

    }

}
