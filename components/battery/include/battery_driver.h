#ifndef _BATTERY_DRIVER_H_
#define _BATTERY_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BATTERY_ADC_CHANNEL      ADC1_CHANNEL_7

esp_err_t battery_init();
uint32_t battery_read_raw();
uint32_t battery_read_voltage();

#ifdef __cplusplus
}
#endif

#endif //_BATTERY_DRIVER_H_