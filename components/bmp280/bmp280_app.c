#include "bmp280_app.h"
#include "bmp280_driver.h"
#include "esp_log.h"
#include "esp_err.h"
#include "common.h"

static const char *TAG = "BMP280_App";
SemaphoreHandle_t xBMP280DataMutex;

static bmp280_data_t bmp_data;

esp_err_t bmp280_send_data() {
    app_data_t data_temperature = {
        .id = DATA_ID_TEMPERATURE,
        .data = bmp_data.temperature
    };

    if (xQueueSend( xQueueAppData, (void *)&data_temperature, 0 ) == pdFAIL) {
        ESP_LOGE(TAG, "ERROR sendig data to queue");
        return ESP_ERR_TIMEOUT;
    }
    app_data_t data_pressure = {
        .id = DATA_ID_PRESSURE,
        .data = bmp_data.pressure
    };

    if (xQueueSend( xQueueAppData, (void *)&data_pressure, 0 ) == pdFAIL) {
        ESP_LOGE(TAG, "ERROR sendig data to queue");
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

void vBMP280Task( void *pvParameters ) {
    esp_err_t err = ESP_OK;
    bmp280_params_t params;
    BMP280_compensation_t comp;

    xBMP280DataMutex = xSemaphoreCreateMutex();
    if(xBMP280DataMutex == NULL) {
        ESP_LOGE(TAG, "ERROR Creating Mutex");
        return;
    }

    // Wait for I2C tasks
    xEventGroupWaitBits(xEventGroupTasks,
                        BIT_TASK_I2C_READ | BIT_TASK_I2C_WRITE,
                        pdFALSE,
                        pdTRUE,
                        portMAX_DELAY);

    bmp280_init_default_params(&params);
    if(bmp280_init(&params, &comp) != ESP_OK) {
        ESP_LOGI(TAG, " Error Initialize BMP280");
        return;
    }

	// Signalize task successfully creation
	xEventGroupSetBits(xEventGroupTasks, BIT_TASK_BMP280);
    ESP_LOGI(TAG, "BMP280 Initialized");


    while(1) {
        if(xSemaphoreTake(xBMP280DataMutex, portMAX_DELAY) == pdTRUE) {
            err = bmp280_read_data(&comp, &bmp_data.temperature, &bmp_data.pressure);
            if(err != ESP_OK) {
                ESP_LOGE(TAG, "ERROR: %s", esp_err_to_name(err));
            }
            // Wait main application is ready to receive data
            if(xEventGroupGetBits(xEventGroupApp) & BIT_APP_SEND_DATA) {
                // send to app queue
                bmp280_send_data();
            }
            xSemaphoreGive(xBMP280DataMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));

    }
}

esp_err_t bmp280_app_read_data(bmp280_data_t * data) {
    if(xSemaphoreTake(xBMP280DataMutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    *data = bmp_data;

    xSemaphoreGive(xBMP280DataMutex);

    return ESP_OK;
}
