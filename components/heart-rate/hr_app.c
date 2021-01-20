#include "hr_app.h"
#include "hr_driver.h"
#include "uart_app.h"
#include "esp_log.h"
#include "common.h"

static const char *TAG = "HEART_RATE_App";
SemaphoreHandle_t xHeartRateDataMutex;

static uint16_t heart_rate_data;

static void heart_rate_data_stream() {
    uart_data_t uart_data_heart_rate = {
        .id = UART_DATA_ID_HEART_RATE,
        .data = (double)heart_rate_data
    };
    if (xQueueSend( xQueueUartWriteBuffer, (void *)&uart_data_heart_rate, portMAX_DELAY ) == pdFAIL) {
        ESP_LOGE(TAG, "ERROR sendig data to queue");
        return;
    }
}

void vHeartRateTask( void *pvParameters ) {
    esp_err_t err = ESP_OK;

    xHeartRateDataMutex = xSemaphoreCreateMutex();
    if(xHeartRateDataMutex == NULL) {
        ESP_LOGE(TAG, "ERROR Creating Mutex");
        return;
    }

    err = heart_rate_init();
    if(err != ESP_OK) {
        ESP_LOGI(TAG, " Error Initialize Heart Rate");
        return;
    }

    // Wait for dependent tasks
    xEventGroupWaitBits(xEventGroupTasks,
                        BIT_TASK_DATA_STREAM,
                        pdFALSE,
                        pdTRUE,
                        portMAX_DELAY);
                        
	// Signalize task successfully creation
	xEventGroupSetBits(xEventGroupTasks, BIT_TASK_HEART_RATE);
    ESP_LOGI(TAG, "Heart Rate Initialized");

    while(1) {
        err = heart_rate_read(&heart_rate_data);

        // Check if Data Stream is enabled
        if(xEventGroupGetBits(xEventGroupDataStreamMode) & BIT_UART_STREAM_MODE_ALL) {
            heart_rate_data_stream();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

