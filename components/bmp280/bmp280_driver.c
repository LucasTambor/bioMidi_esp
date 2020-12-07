#include "bmp280_driver.h"
#include "i2c_app.h"
#include <esp_log.h>

//***************************************************************************************************************

static const char *TAG = "BMP280_Driver";

static esp_err_t bmp280_read(uint8_t reg, size_t len, uint8_t * data_buf) {

    I2C_Status i2c_msg = {
        .data = data_buf,
        .device_address = BMP280_I2C_ADDRESS_0,
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

static esp_err_t bmp280_write(uint8_t reg, size_t len, uint8_t * data_buf) {
    // Write data to i2c
    I2C_Status i2c_msg = {
        .data = data_buf,
        .device_address = BMP280_I2C_ADDRESS_0,
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

static esp_err_t read_calibration_data(BMP280_compensation_t * compensation)
{
    ESP_ERROR_CHECK(bmp280_read(BMP_280_REG_COMP_T1, 2, (uint8_t *)&compensation->dig_T1));
    ESP_ERROR_CHECK(bmp280_read(BMP_280_REG_COMP_T2, 2, (uint8_t *)&compensation->dig_T2));
    ESP_ERROR_CHECK(bmp280_read(BMP_280_REG_COMP_T3, 2, (uint8_t *)&compensation->dig_T3));
    ESP_ERROR_CHECK(bmp280_read(BMP_280_REG_COMP_P1, 2, (uint8_t *)&compensation->dig_P1));
    ESP_ERROR_CHECK(bmp280_read(BMP_280_REG_COMP_P2, 2, (uint8_t *)&compensation->dig_P2));
    ESP_ERROR_CHECK(bmp280_read(BMP_280_REG_COMP_P3, 2, (uint8_t *)&compensation->dig_P3));
    ESP_ERROR_CHECK(bmp280_read(BMP_280_REG_COMP_P4, 2, (uint8_t *)&compensation->dig_P4));
    ESP_ERROR_CHECK(bmp280_read(BMP_280_REG_COMP_P5, 2, (uint8_t *)&compensation->dig_P5));
    ESP_ERROR_CHECK(bmp280_read(BMP_280_REG_COMP_P6, 2, (uint8_t *)&compensation->dig_P6));
    ESP_ERROR_CHECK(bmp280_read(BMP_280_REG_COMP_P7, 2, (uint8_t *)&compensation->dig_P7));
    ESP_ERROR_CHECK(bmp280_read(BMP_280_REG_COMP_P8, 2, (uint8_t *)&compensation->dig_P8));
    ESP_ERROR_CHECK(bmp280_read(BMP_280_REG_COMP_P9, 2, (uint8_t *)&compensation->dig_P9));

    ESP_LOGD(TAG, "Calibration data received:");
    ESP_LOGD(TAG, "dig_T1=%d", compensation->dig_T1);
    ESP_LOGD(TAG, "dig_T2=%d", compensation->dig_T2);
    ESP_LOGD(TAG, "dig_T3=%d", compensation->dig_T3);
    ESP_LOGD(TAG, "dig_P1=%d", compensation->dig_P1);
    ESP_LOGD(TAG, "dig_P2=%d", compensation->dig_P2);
    ESP_LOGD(TAG, "dig_P3=%d", compensation->dig_P3);
    ESP_LOGD(TAG, "dig_P4=%d", compensation->dig_P4);
    ESP_LOGD(TAG, "dig_P5=%d", compensation->dig_P5);
    ESP_LOGD(TAG, "dig_P6=%d", compensation->dig_P6);
    ESP_LOGD(TAG, "dig_P7=%d", compensation->dig_P7);
    ESP_LOGD(TAG, "dig_P8=%d", compensation->dig_P8);
    ESP_LOGD(TAG, "dig_P9=%d", compensation->dig_P9);

    return ESP_OK;
}

esp_err_t bmp280_init_default_params(bmp280_params_t *params) {
    if(!params) return ESP_ERR_INVALID_ARG;

    params->mode = BMP280_MODE_FORCED;
    params->filter = BMP280_FILTER_4;
    params->oversampling_pressure = BMP280_ULTRA_HIGH_RES;
    params->oversampling_temperature = BMP280_ULTRA_HIGH_RES;
    params->standby = BMP280_STANDBY_250;

    return ESP_OK;
}
/**
 * Compensation algorithm is taken from BMP280 datasheet.
 *
 * Return value is in degrees Celsius.
 */
static inline int32_t compensate_temperature(BMP280_compensation_t *comp, int32_t adc_temp, int32_t *fine_temp)
{
    int32_t var1, var2;

    var1 = ((((adc_temp >> 3) - ((int32_t)comp->dig_T1 << 1))) * (int32_t)comp->dig_T2) >> 11;
    var2 = (((((adc_temp >> 4) - (int32_t)comp->dig_T1) * ((adc_temp >> 4) - (int32_t)comp->dig_T1)) >> 12) * (int32_t)comp->dig_T3) >> 14;

    *fine_temp = var1 + var2;
    return (*fine_temp * 5 + 128) >> 8;
}

/**
 * Compensation algorithm is taken from BMP280 datasheet.
 *
 * Return value is in Pa, 24 integer bits and 8 fractional bits.
 */
static inline uint32_t compensate_pressure(BMP280_compensation_t *comp, int32_t adc_press, int32_t fine_temp)
{
    int64_t var1, var2, p;

    var1 = (int64_t)fine_temp - 128000;
    var2 = var1 * var1 * (int64_t)comp->dig_P6;
    var2 = var2 + ((var1 * (int64_t)comp->dig_P5) << 17);
    var2 = var2 + (((int64_t)comp->dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)comp->dig_P3) >> 8) + ((var1 * (int64_t)comp->dig_P2) << 12);
    var1 = (((int64_t)1 << 47) + var1) * ((int64_t)comp->dig_P1) >> 33;

    if (var1 == 0)
    {
        return 0;  // avoid exception caused by division by zero
    }

    p = 1048576 - adc_press;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = ((int64_t)comp->dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    var2 = ((int64_t)comp->dig_P8 * p) >> 19;

    p = ((p + var1 + var2) >> 8) + ((int64_t)comp->dig_P7 << 4);
    return p;
}

esp_err_t bmp280_init(bmp280_params_t * params, BMP280_compensation_t * comp) {
    if(!params) return ESP_ERR_INVALID_ARG;

    // Check presence
    uint8_t id;
    bmp280_read(BMP280_REG_ID, 1, &id);
    if(id != BMP280_CHIP_ID) {
        ESP_LOGE(TAG, "Invalid chip ID: expected: 0x%x (BME280) got: 0x%x", BMP280_CHIP_ID, id);
    }

    // Soft reset
    uint8_t reset_value = BMP280_RESET_VALUE;
    bmp280_write(BMP280_REG_RESET, 1, &reset_value);

    // Wait until finished copying over the NVP data.
    while (1)
    {
        uint8_t status;
        ESP_ERROR_CHECK(bmp280_read(BMP280_REG_STATUS, 1, &status));
        if((status & 1) == 0) break;
    }

    // Read calibration data
    ESP_ERROR_CHECK(read_calibration_data(comp));

    // Write Config
    uint8_t config = (params->standby << 5) | (params->filter << 2);
    ESP_LOGD(TAG, "Writing config reg=%x", config);
    ESP_ERROR_CHECK(bmp280_write(BMP280_REG_CONFIG, 1, &config));

    if (params->mode == BMP280_MODE_FORCED)
    {
        params->mode = BMP280_MODE_SLEEP;  // initial mode for forced is sleep
    }

    uint8_t ctrl = (params->oversampling_temperature << 5) | (params->oversampling_pressure << 2) | (params->mode);
    ESP_ERROR_CHECK(bmp280_write(BMP280_REG_CTRL, 1, &ctrl));

    return ESP_OK;
}

static esp_err_t bmp280_force_measurement() {
    uint8_t ctrl;
    ESP_ERROR_CHECK(bmp280_read(BMP280_REG_CTRL, 1, &ctrl));
    ctrl &= ~0b11;  // clear two lower bits
    ctrl |= BMP280_MODE_FORCED;
    ESP_ERROR_CHECK(bmp280_write(BMP280_REG_CTRL, 1, &ctrl));
    return ESP_OK;
}

static bool bmp280_is_measuring() {
    uint8_t status;
    ESP_ERROR_CHECK(bmp280_read(BMP280_REG_STATUS, 1, &status));

    // check 'measuring' bit in status register
    return (status & (1 << 3));
}

esp_err_t bmp280_read_data(BMP280_compensation_t *comp, float * temperature, float * pressure) {
    if(!temperature || !pressure) return ESP_ERR_INVALID_ARG;

    int32_t adc_pressure;
    int32_t adc_temp;
    uint8_t data[6];

    bmp280_force_measurement();

    while(bmp280_is_measuring()) {
        ESP_LOGD(TAG, "Conversion Running");
        vTaskDelay(pdMS_TO_TICKS(30));
    }

    ESP_ERROR_CHECK(bmp280_read(BMP280_REG_PRESS_MSB, 6, data));
    adc_pressure = data[0] << 12 | data[1] << 4 | data[2] >> 4;
    adc_temp = data[3] << 12 | data[4] << 4 | data[5] >> 4;

    int32_t fine_temp;
    *temperature = (float)compensate_temperature(comp, adc_temp, &fine_temp)/100;
    *pressure = (float)compensate_pressure(comp, adc_pressure, fine_temp)/256;

    return ESP_OK;
}

