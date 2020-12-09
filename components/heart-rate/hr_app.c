#include "hr_app.h"
#include "hr_driver.h"
#include "uart_app.h"
#include "esp_log.h"

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

    ESP_LOGI(TAG, "Initialize Heart Rate");

    while(1) {
        err = heart_rate_read(&heart_rate_data);
        // ESP_LOGI(TAG, "%d\n",heart_rate_data);

        heart_rate_data_stream();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

