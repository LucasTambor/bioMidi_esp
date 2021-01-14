#include "mpu6050_app.h"
#include "math.h"
#include "mpu6050_driver.h"
#include "uart_app.h"
#include "esp_log.h"
#include "common.h"
#include <string.h>

//****************************************************************************************************************

static const char *TAG = "MPU6050_App";
SemaphoreHandle_t xMPU6050DataMutex;

static mpu6050_data_t mpu6050_data;
static mpu6050_angle_data_t mpu6050_angle_data;

// Error correction values
static double accel_error_x = 0;
static double accel_error_y = 0;
static double gyr_error_x = 0;
static double gyr_error_y = 0;
static double gyr_error_z = 0;

//****************************************************************************************************************

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

esp_err_t mpu6050_send_data(mpu6050_angle_data_t real_angle) {
    app_data_t data_roll = {
        .id = DATA_ID_ROLL,
        .data = real_angle.roll
    };
    if (xQueueSend( xQueueAppData, (void *)&data_roll, 0 ) == pdFAIL) {
        ESP_LOGE(TAG, "ERROR sendig data to queue");
        return ESP_ERR_TIMEOUT;
    }
    app_data_t data_pitch = {
        .id = DATA_ID_PITCH,
        .data = real_angle.pitch
    };
    if (xQueueSend( xQueueAppData, (void *)&data_pitch, 0 ) == pdFAIL) {
        ESP_LOGE(TAG, "ERROR sendig data to queue");
        return ESP_ERR_TIMEOUT;
    }
    app_data_t data_yaw = {
        .id = DATA_ID_YAW,
        .data = real_angle.yaw
    };
    if (xQueueSend( xQueueAppData, (void *)&data_yaw, 0 ) == pdFAIL) {
        ESP_LOGE(TAG, "ERROR sendig data to queue");
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

// Calculate Roll and Pitch from the accelerometer data
static void mpu6050_calculate_angle_accel(mpu6050_accel_data_t * accel_data, mpu6050_angle_data_t * angle_data) {
    angle_data->roll = (atan(accel_data->y / sqrt(pow(accel_data->x, 2) + pow(accel_data->z, 2))) * 180 / M_PI) - accel_error_x;
    angle_data->pitch = (atan(-1 * accel_data->x / sqrt(pow(accel_data->y, 2) + pow(accel_data->z, 2))) * 180 / M_PI) - accel_error_y;
}

// Calculate Roll and Pitch from the gyroscope data
static void mpu6050_calculate_angle_gyr(mpu6050_gyr_data_t * gyr_data, mpu6050_angle_data_t * angle_data) {
    // Correct values
    angle_data->roll -= gyr_error_x;
    angle_data->pitch -= gyr_error_y;
    angle_data->yaw -= gyr_error_z;

    angle_data->roll = angle_data->roll + (gyr_data->x * MPU_6050_TASK_PERIOD_MS/1000);
    angle_data->pitch = angle_data->pitch + (gyr_data->y * MPU_6050_TASK_PERIOD_MS/1000);
    angle_data->yaw = angle_data->yaw + (gyr_data->z * MPU_6050_TASK_PERIOD_MS/1000);
}

// Complementary filter - combine acceleromter and gyro angle values
static void mpu6050_calculate_angle(mpu6050_angle_data_t * gyr_angle, mpu6050_angle_data_t * accel_angle, mpu6050_angle_data_t * real_angle ) {
    real_angle->roll = 0.96 * gyr_angle->roll + 0.04 * accel_angle->roll;
    real_angle->pitch = 0.96 * gyr_angle->pitch + 0.04 * accel_angle->pitch;
    real_angle->yaw = gyr_angle->yaw;
    // ESP_LOGI(TAG, "Bleu");

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

    mpu6050_gyr_data_t gyr_data = {0};
    mpu6050_accel_data_t accel_data = {0};
    mpu6050_angle_data_t gyr_angle = {0};
    mpu6050_angle_data_t accel_angle = {0};
    mpu6050_angle_data_t real_angle = {0};
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

            // Calculate angles
            mpu6050_calculate_angle_accel(&accel_data, &accel_angle);
            mpu6050_calculate_angle_gyr(&gyr_data, &gyr_angle);
            mpu6050_calculate_angle(&gyr_angle, &accel_angle, &real_angle);

            mpu6050_angle_data = real_angle;
            xSemaphoreGive(xMPU6050DataMutex);
        }

        // Wait main application is ready to receive data
        if(xEventGroupGetBits(xEventGroupApp) & BIT_APP_SEND_DATA) {
            mpu6050_send_data(real_angle);
        }
        // mpu6050_data_stream();

        vTaskDelay(pdMS_TO_TICKS(MPU_6050_TASK_PERIOD_MS));
    }
}

esp_err_t mpu6050_app_read_data(mpu6050_data_t * data, mpu6050_angle_data_t * angle_data) {
    if(xSemaphoreTake(xMPU6050DataMutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    *data = mpu6050_data;
    *angle_data = mpu6050_angle_data;

    xSemaphoreGive(xMPU6050DataMutex);

    return ESP_OK;
}

esp_err_t mpu6050_app_calibrate(mpu6050_data_t * data_error) {
    mpu6050_gyr_data_t gyr_data;
    mpu6050_accel_data_t accel_data;
    mpu6050_angle_data_t gyr_angle;
    mpu6050_angle_data_t accel_angle;

    memset(&gyr_data, 0, sizeof(mpu6050_gyr_data_t));
    memset(&accel_data, 0, sizeof(mpu6050_accel_data_t));
    memset(&gyr_angle, 0, sizeof(mpu6050_angle_data_t));
    memset(&accel_angle, 0, sizeof(mpu6050_angle_data_t));

    for(uint8_t i = 0; i<2; i++) {
        ESP_ERROR_CHECK(mpu6050_read_accel(&accel_data));
        ESP_LOGI(TAG, "Ax: %.2f | Ay: %.2f | Az: %.2f  ", accel_data.x, accel_data.y, accel_data.z);
        accel_angle.roll += (atan(accel_data.y / sqrt(pow(accel_data.x, 2) + pow(accel_data.z, 2))) * 180 / M_PI);
        accel_angle.pitch += (atan(-1 * accel_data.x / sqrt(pow(accel_data.y, 2) + pow(accel_data.z, 2))) * 180 / M_PI);
        ESP_LOGI(TAG, "Ar: %.2f | Ap: %.2f", accel_angle.roll, accel_angle.pitch);
    }
    accel_angle.roll = accel_angle.roll / 2;
    accel_angle.pitch = accel_angle.pitch / 2;


    for(uint8_t i = 0; i<50; i++) {
        ESP_ERROR_CHECK(mpu6050_read_gyr(&gyr_data));
        gyr_angle.roll += gyr_data.x;
        gyr_angle.pitch += gyr_data.y;
        gyr_angle.yaw += gyr_data.z;
    }

    gyr_angle.roll = gyr_angle.roll / 50;
    gyr_angle.pitch = gyr_angle.pitch / 50 ;
    gyr_angle.yaw = gyr_angle.yaw / 50;

    // Set local errors
    accel_error_x = accel_angle.roll;
    accel_error_y = accel_angle.pitch;
    gyr_error_x = gyr_angle.roll;
    gyr_error_y = gyr_angle.pitch;
    gyr_error_z = gyr_angle.yaw;

    ESP_LOGI(TAG, "Accel error x: %.2f \n \
                 Accel error y: %.2f \n \
                 Gyro error x: %.2f \n \
                 Gyro error y: %.2f \n \
                 Gyro error z: %.2f", accel_error_x, accel_error_y, gyr_error_x, gyr_error_y, gyr_error_z);
    // Return errors
    if(data_error) {
        data_error->accel_x = accel_angle.roll;
        data_error->accel_y = accel_angle.pitch;
        data_error->gyr_x = gyr_angle.roll;
        data_error->gyr_y = gyr_angle.pitch;
        data_error->gyr_z = gyr_angle.yaw;
    }

    return ESP_OK;
}

