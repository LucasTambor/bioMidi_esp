#include "battery_driver.h"
#include <esp_log.h>

//***************************************************************************************************************
#define DEFAULT_VREF    1100
#define NUM_OF_SAMPLES  64

static const char *TAG = "Battery_driver";

static esp_adc_cal_characteristics_t *adc_chars;

esp_err_t battery_init() {
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_10Bit));
    ESP_ERROR_CHECK(adc1_config_channel_atten(BATTERY_ADC_CHANNEL, ADC_ATTEN_DB_11));

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_10Bit, DEFAULT_VREF, adc_chars);
    return ESP_OK;
}

uint32_t battery_read_raw() {
    uint32_t adc_reading = 0;
    for(uint8_t i = 0; i<NUM_OF_SAMPLES; i++) {
        adc_reading += adc1_get_raw(BATTERY_ADC_CHANNEL);
    }

    return adc_reading/NUM_OF_SAMPLES;
}

uint32_t battery_read_voltage() {
    return esp_adc_cal_raw_to_voltage(battery_read_raw(), adc_chars);
}

