#include "hr_driver.h"
#include <esp_log.h>

//***************************************************************************************************************

static const char *TAG = "HR_driver";

esp_err_t heart_rate_init() {
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_10Bit));
    ESP_ERROR_CHECK(adc1_config_channel_atten(HEART_RATE_ADC_CHANNEL, ADC_ATTEN_DB_11));
    return ESP_OK;
}

esp_err_t heart_rate_read(uint16_t * data) {
    *data = adc1_get_raw(HEART_RATE_ADC_CHANNEL);
    return ESP_OK;
}

