#include "battery_app.h"
#include "battery_driver.h"
#include "esp_log.h"
#include "common.h"

#define MAX_BATTERY_VOLTAGE     4200

// #define USE_LDO_AMS1117
// #define USE_LDO_TLV757P
#define USE_LDO_DEBUG

// The dropout voltage from the linear voltage regulator changes the minimal voltage value.
#ifdef USE_LDO_AMS1117
static float min_ldo_voltage = 4600; //mV
#endif

#ifdef USE_LDO_TLV757P
static float min_ldo_voltage = 3800; //mV
#endif

#ifdef USE_LDO_DEBUG
static float min_ldo_voltage = 0; //mV
#endif

static const char *TAG = "Battery_App";
SemaphoreHandle_t xBatteryMeasurementDataMutex;

static uint32_t battery_voltage = 0;

static uint32_t map_value_u32(uint32_t value, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max) {
    return (uint32_t) ((value - in_min) * ((uint32_t) out_max - (uint32_t) out_min) / (in_max - in_min) + (uint32_t) out_min);
}

void vBatteryMeasurementTask( void *pvParameters ) {
    esp_err_t err = ESP_OK;

    xBatteryMeasurementDataMutex = xSemaphoreCreateMutex();
    if(xBatteryMeasurementDataMutex == NULL) {
        ESP_LOGE(TAG, "ERROR Creating Mutex");
        return;
    }

    err = battery_init();
    if(err != ESP_OK) {
        ESP_LOGI(TAG, " Error Initialize Battery Management");
        return;
    }

	// Signalize task successfully creation
	xEventGroupSetBits(xEventGroupTasks, BIT_TASK_BATTERY_MAN);
    ESP_LOGI(TAG, "Battery Management Initialized");

    while(1) {
        if(xSemaphoreTake(xBatteryMeasurementDataMutex, portMAX_DELAY) == pdTRUE) {
            battery_voltage = battery_read_voltage();
            xSemaphoreGive(xBatteryMeasurementDataMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

esp_err_t battery_app_read_percent(uint8_t * data) {
    esp_err_t err = ESP_OK;

    if(!data) return ESP_ERR_INVALID_ARG;


    if(xSemaphoreTake(xBatteryMeasurementDataMutex, portMAX_DELAY) == pdTRUE) {
        if(battery_voltage <=  min_ldo_voltage) {
            *data = 0;
        } else {
            *data = (uint8_t) map_value_u32(battery_voltage, 0, 3300, 0, 100);
        }
        ESP_LOGD(TAG, "Battery: %d%% | %d mV", *data, battery_voltage);
        xSemaphoreGive(xBatteryMeasurementDataMutex);
    }


    return ESP_OK;
}

esp_err_t battery_app_read_voltage(uint16_t * data) {
    if(!data) return ESP_ERR_INVALID_ARG;
    if(xSemaphoreTake(xBatteryMeasurementDataMutex, portMAX_DELAY) == pdTRUE) {
        *data = battery_voltage;
        xSemaphoreGive(xBatteryMeasurementDataMutex);
    }
    return ESP_OK;
}




