#include "touch_app.h"
#include "touch_driver.h"
#include "esp_log.h"

static const char *TAG = "Touch_App";

void vTouchButton( void *pvParameters ) {
    if(touch_button_init() == ESP_OK) {
        ESP_LOGI(TAG, "Initialize Touch Button");
    }

    bool button_state = false;
    bool old_button_state = false;
    long time_start = 0;
    long time_stop = 0;
    while(1) {
        touch_button_read(&button_state);

        // Get step up and down
        switch(old_button_state) {
            case 0:
                if(button_state) {
                    time_start = xTaskGetTickCount();
                }
            break;
            case 1:
                if(!button_state) {
                    time_stop = xTaskGetTickCount();
                    ESP_LOGD(TAG, "Button pressed for %d ms", pdTICKS_TO_MS(time_stop - time_start));
                }
            break;
            default:
            break;
        }

        old_button_state = button_state;

        vTaskDelay( 100 / portTICK_RATE_MS );
    }
}
