#include "bmp280_app.h"
#include "bmp280_driver.h"
#include "esp_log.h"

static const char *TAG = "BMP280_App";


void vBMP280Task( void *pvParameters ) {
    float temperature = 0;
    float pressure = 0;
    bmp280_params_t params;
    BMP280_compensation_t comp;

    bmp280_init_default_params(&params);
    if(bmp280_init(&params, &comp) == ESP_OK) {
        ESP_LOGI(TAG, "Initialize BMP280");
    }


    while(1) {
        ESP_ERROR_CHECK(bmp280_read_data(&comp, &temperature, &pressure));

        ESP_LOGI(TAG, "Temperature: %.2f | Pressure: %.2f", temperature, pressure);

        vTaskDelay(pdMS_TO_TICKS(1000));

    }
}
