#include "touch_app.h"
#include "touch_driver.h"
#include "esp_log.h"

#define MAX_TOUCH_CALLBACKS 8
#define TOUCH_HISTERESIS    250 //ms
static const char *TAG = "Touch_App";

typedef struct {
    uint16_t ms;
    void (*func)();
} touch_callback_t;

static touch_callback_t callbacks[MAX_TOUCH_CALLBACKS];
static uint8_t callback_count = 0;

static void get_callback(uint16_t ms_read) {
    for(uint8_t i = 0; i<callback_count; i++) {
        if(ms_read <= callbacks[i].ms + TOUCH_HISTERESIS && ms_read >= callbacks[i].ms - TOUCH_HISTERESIS ) {
            ESP_LOGI(TAG, "Callback found for %d ms", ms_read);
            callbacks[i].func();
        }
    }
}

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
                    uint32_t ms_get = pdTICKS_TO_MS(time_stop - time_start);
                    ESP_LOGI(TAG, "Button pressed for %d ms", ms_get);
                    get_callback(ms_get);
                }
            break;
            default:
            break;
        }

        old_button_state = button_state;

        vTaskDelay( 100 / portTICK_RATE_MS );
    }
}


void touch_add_callback(uint16_t ms, void (*func)()) {
    if(callback_count / MAX_TOUCH_CALLBACKS) {
        ESP_LOGE(TAG, "Exceed number of callbacks");
        return;
    }
    touch_callback_t callback = {ms, func};
    callbacks[callback_count] = callback;
    callback_count++;
    ESP_LOGI(TAG, "Callbacks added for %d ms", ms);

}
