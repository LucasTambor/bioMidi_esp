#include "bmp280_app.h"
#include "bmp280_driver.h"
#include "esp_log.h"

static const char *TAG = "BMP280_App";
SemaphoreHandle_t xBMP280DataMutex;

static bmp280_data_t bmp_data;

void vBMP280Task( void *pvParameters ) {
    esp_err_t err = ESP_OK;
    bmp280_params_t params;
    BMP280_compensation_t comp;

    xBMP280DataMutex = xSemaphoreCreateMutex();
    if(xBMP280DataMutex == NULL) {
        ESP_LOGE(TAG, "ERROR Creating Mutex");
        return;
    }

    bmp280_init_default_params(&params);
    if(bmp280_init(&params, &comp) != ESP_OK) {
        ESP_LOGI(TAG, " Error Initialize BMP280");
        return;
    }

    ESP_LOGI(TAG, "Initialize BMP280");


    while(1) {
        if(xSemaphoreTake(xBMP280DataMutex, portMAX_DELAY) == pdTRUE) {
            err = bmp280_read_data(&comp, &bmp_data.temperature, &bmp_data.pressure);
            if(err != ESP_OK) {
                ESP_LOGE(TAG, "ERROR: %s", esp_err_to_name(err));
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

    data->temperature = bmp_data.temperature;
    data->pressure = bmp_data.pressure;

    xSemaphoreGive(xBMP280DataMutex);

    return ESP_OK;
}
