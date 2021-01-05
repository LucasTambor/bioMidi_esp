#include "mpu6050_app.h"
#include "mpu6050_driver.h"
#include "uart_app.h"
#include "esp_log.h"
#include "common.h"

//****************************************************************************************************************

static const char *TAG = "MPU6050_App";
SemaphoreHandle_t xMPU6050DataMutex;

static mpu6050_data_t mpu6050_data;

//****************************************************************************************************************

static void mpu6050_data_stream() {
    uart_data_t uart_data_accel_x = {
        .id = UART_DATA_ID_ACCEL_X,
        .data = mpu6050_data.accel_x
    };
    if (xQueueSend( xQueueUartWriteBuffer, (void *)&uart_data_accel_x, portMAX_DELAY ) == pdFAIL) {
        ESP_LOGE(TAG, "ERROR sendig data to queue");
        return;
    }

    uart_data_t uart_data_accel_y = {
        .id = UART_DATA_ID_ACCEL_Y,
        .data = mpu6050_data.accel_y
    };
    if (xQueueSend( xQueueUartWriteBuffer, (void *)&uart_data_accel_y, portMAX_DELAY ) == pdFAIL) {
        ESP_LOGE(TAG, "ERROR sendig data to queue");
        return;
    }
    uart_data_t uart_data_accel_z = {
        .id = UART_DATA_ID_ACCEL_Z,
        .data = mpu6050_data.accel_z
    };
    if (xQueueSend( xQueueUartWriteBuffer, (void *)&uart_data_accel_z, portMAX_DELAY ) == pdFAIL) {
        ESP_LOGE(TAG, "ERROR sendig data to queue");
        return;
    }
    uart_data_t uart_data_gyr_x = {
        .id = UART_DATA_ID_GYR_X,
        .data = mpu6050_data.gyr_x
    };
    if (xQueueSend( xQueueUartWriteBuffer, (void *)&uart_data_gyr_x, portMAX_DELAY ) == pdFAIL) {
        ESP_LOGE(TAG, "ERROR sendig data to queue");
        return;
    }
    uart_data_t uart_data_gyr_y = {
        .id = UART_DATA_ID_GYR_Y,
        .data = mpu6050_data.gyr_y
    };
    if (xQueueSend( xQueueUartWriteBuffer, (void *)&uart_data_gyr_y, portMAX_DELAY ) == pdFAIL) {
        ESP_LOGE(TAG, "ERROR sendig data to queue");
        return;
    }
    uart_data_t uart_data_gyr_z = {
        .id = UART_DATA_ID_GYR_Z,
        .data = mpu6050_data.gyr_z
    };
    if (xQueueSend( xQueueUartWriteBuffer, (void *)&uart_data_gyr_z, portMAX_DELAY ) == pdFAIL) {
        ESP_LOGE(TAG, "ERROR sendig data to queue");
        return;
    }

}

void vMPU6050Task( void *pvParameters ) {
    esp_err_t err = ESP_OK;

    xMPU6050DataMutex = xSemaphoreCreateMutex();
    if(xMPU6050DataMutex == NULL) {
        ESP_LOGE(TAG, "ERROR Creating Mutex");
        return;
    }

    // Wait for I2C tasks
    xEventGroupWaitBits(xEventGroupTasks,
                        BIT_TASK_I2C_READ | BIT_TASK_I2C_WRITE,
                        pdFALSE,
                        pdTRUE,
                        portMAX_DELAY);


    err = mpu6050_init();
    if(err != ESP_OK) {
        ESP_LOGI(TAG, " Error Initialize MPU6050");
        return;
    }

	// Signalize task successfully creation
	xEventGroupSetBits(xEventGroupTasks, BIT_TASK_MPU6050);
    ESP_LOGI(TAG, "MPU6050 Initialized");

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

        // mpu6050_data_stream();

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
