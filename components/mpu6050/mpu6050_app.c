#include "mpu6050_app.h"
#include "mpu6050_driver.h"
#include "esp_log.h"

static const char *TAG = "MPU6050_App";
SemaphoreHandle_t xMPU6050DataMutex;

static mpu6050_data_t mpu6050_data;

void vMPU6050Task( void *pvParameters ) {
    esp_err_t err = ESP_OK;

    xMPU6050DataMutex = xSemaphoreCreateMutex();
    if(xMPU6050DataMutex == NULL) {
        ESP_LOGE(TAG, "ERROR Creating Mutex");
        return;
    }
    // vTaskDelay(pdMS_TO_TICKS(2000));

    err = mpu6050_init();
    if(err != ESP_OK) {
        ESP_LOGI(TAG, " Error Initialize MPU6050");
        return;
    }

    ESP_LOGI(TAG, "Initialize MPU6050");

    mpu6050_gyr_data_t gyr_data;
    mpu6050_accel_data_t accel_data;
    double temp_data;
    while(1) {
        ESP_ERROR_CHECK(mpu6050_read_accel(&accel_data));
        ESP_ERROR_CHECK(mpu6050_read_gyr(&gyr_data));
        ESP_ERROR_CHECK(mpu6050_read_temp(&temp_data));
        if(xSemaphoreTake(xMPU6050DataMutex, portMAX_DELAY) == pdTRUE) {
            mpu6050_data.accel_x = accel_data.x;
            mpu6050_data.accel_y = accel_data.y;
            mpu6050_data.accel_z = accel_data.z;
            mpu6050_data.gyr_x = gyr_data.x;
            mpu6050_data.gyr_y = gyr_data.y;
            mpu6050_data.gyr_z = gyr_data.z;
            mpu6050_data.temperature = temp_data;
            xSemaphoreGive(xMPU6050DataMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

esp_err_t mpu6050_app_read_data(mpu6050_data_t * data) {
    if(xSemaphoreTake(xMPU6050DataMutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    *data = mpu6050_data;

    xSemaphoreGive(xMPU6050DataMutex);

    return ESP_OK;
}
