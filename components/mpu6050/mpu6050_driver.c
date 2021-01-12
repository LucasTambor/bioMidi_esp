#include "mpu6050_driver.h"
#include "i2c_app.h"
#include <esp_log.h>

//***************************************************************************************************************

static const char *TAG = "MPU6050_Driver";

static esp_err_t mpu6050_read(uint8_t reg, size_t len, uint8_t * data_buf) {

    I2C_Status i2c_msg = {
        .data = data_buf,
        .device_address = MPU6050_SLAVE_ADDR,
        .register_address = reg,
        .size = len
    };

    if(xQueueSend( xQueueI2CReadBuffer, (void *)&i2c_msg, portMAX_DELAY ) == pdFAIL) {
        ESP_LOGE(TAG, "Error writing to I2C Buffer");
        return ESP_ERR_TIMEOUT;
    }

    if (xSemaphoreTake( xBinarySemaphoreI2CAppEndOfRead, portMAX_DELAY ) == pdFAIL) {
        ESP_LOGE(TAG, "Error waiting i2c end of write");
        return ESP_ERR_TIMEOUT;
	}

    return ESP_OK;
}

static esp_err_t mpu6050_write(uint8_t reg, size_t len, uint8_t * data_buf) {
    // Write data to i2c
    I2C_Status i2c_msg = {
        .data = data_buf,
        .device_address = MPU6050_SLAVE_ADDR,
        .register_address = reg,
        .size = len
    };

    if(xQueueSend( xQueueI2CWriteBuffer, (void *)&i2c_msg, portMAX_DELAY ) == pdFAIL) {
        ESP_LOGE(TAG, "Error writing to I2C Buffer");
        return ESP_ERR_TIMEOUT;
    }

    if (xSemaphoreTake( xBinarySemaphoreI2CAppEndOfWrite, portMAX_DELAY ) == pdFAIL) {
        ESP_LOGE(TAG, "Error waiting i2c end of write");
        return ESP_ERR_TIMEOUT;
	}
    return ESP_OK;
}

//***********************************************************************************************

esp_err_t mpu6050_init() {
    esp_err_t err = ESP_OK;
    uint8_t data[2] = {0};

    // Change status from sleep to running
    err = mpu6050_write(MPU6050_PWR_MGMT_1, 1, data);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Error setting MPU to running");
        return err;
    }
    // Configure accelerometer
    data[0] =_2G_SCALE;
    mpu6050_write(MPU6050_ACCEL_CONFIG, 1, data);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Error configuring accel");
        return err;
    }
    // Configure gyroscope
    data[0] =_250_DEGREE;
    err = mpu6050_write(MPU6050_GYRO_CONFIG, 1, data);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Error configuring Gyro");
        return err;
    }

    return ESP_OK;
}

esp_err_t mpu6050_read_accel(mpu6050_accel_data_t * data) {
    if(!data) return ESP_ERR_INVALID_ARG;

    esp_err_t err = ESP_OK;
    int16_t accel_read_x = 0;
    int16_t accel_read_y = 0;
    int16_t accel_read_z = 0;

    uint8_t data_read[6] = {0};
    mpu6050_read(MPU6050_ACCEL_XOUT_H, 6, data_read);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Error Reading Accel data");
        return err;
    }

    accel_read_x = ((uint16_t)(data_read[0] << 8) | ((uint16_t)data_read[1]));
    accel_read_y = ((uint16_t)(data_read[2] << 8) | ((uint16_t)data_read[3]));
    accel_read_z = ((uint16_t)(data_read[4] << 8) | ((uint16_t)data_read[5]));

    data->x = -accel_read_x /LSB_Sensitivity_2G;
    data->y = -accel_read_y /LSB_Sensitivity_2G;
    data->z = -accel_read_z /LSB_Sensitivity_2G;

    return ESP_OK;
}

esp_err_t mpu6050_read_gyr(mpu6050_gyr_data_t * data) {
    if(!data) return ESP_ERR_INVALID_ARG;

    esp_err_t err = ESP_OK;
    int16_t gyr_read_x = 0;
    int16_t gyr_read_y = 0;
    int16_t gyr_read_z = 0;

    uint8_t data_read[6] = {0};
    mpu6050_read(MPU6050_GYRO_XOUT_H, 6, data_read);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Error Reading Gyr data");
        return err;
    }
    gyr_read_x = ((uint16_t)(data_read[0] << 8) | ((uint16_t)data_read[1]));
    gyr_read_y = ((uint16_t)(data_read[2] << 8) | ((uint16_t)data_read[3]));
    gyr_read_z = ((uint16_t)(data_read[4] << 8) | ((uint16_t)data_read[5]));

    data->x = -gyr_read_x /LSB_Sensitivity_250;
    data->y = -gyr_read_y /LSB_Sensitivity_250;
    data->z = -gyr_read_z /LSB_Sensitivity_250;

    return ESP_OK;
}

esp_err_t mpu6050_read_temp(double * data) {
    if(!data) return ESP_ERR_INVALID_ARG;

    esp_err_t err = ESP_OK;

    uint8_t data_read[6] = {0};
    mpu6050_read(MPU6050_TEMP_OUT_H, 2, data_read);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Error Reading Temp data");
        return err;
    }
    int16_t data_raw = ((uint16_t) data_read[0] << 8 | ((uint16_t) data_read[1]));

    *data = ((double)data_raw/340) + 36.53;
    return ESP_OK;
}
