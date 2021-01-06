/**
 * midi_controller.c
 *
*/
#include "midi_controller.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "common.h"
#include "task_console.h"
#include "i2c_app.h"
#include "i2c_driver.h"
#include "led_app.h"
#include "touch_app.h"
#include "mic_app.h"
#include "bmp280_app.h"
#include "mpu6050_app.h"
#include "hr_app.h"

#include "blemidi.h"

TaskHandle_t xTaskLedControlHandle;
TaskHandle_t xTaskTouchHandle;
TaskHandle_t xTaskMicHandle;
TaskHandle_t xTaskBMPHandle;
TaskHandle_t xTaskMPUHandle;
TaskHandle_t xTaskHeartRateHandle;

//**********************************************************************************************************

static const char* TAG = "MidiControler";
static midi_controller_state_e bio_midi_state = STATE_IDLE;

//**********************************************************************************************************
// Touch Callbacks

void callback_2000ms(){
    LED_Status led;
    memset(&led, 0, sizeof(LED_Status));

    led.color = 0;
    led.freq = 1000;

    if(xQueueSend( xQueueLedBuffer, (void *)&led, portMAX_DELAY ) == pdFAIL) {
        ESP_LOGE(TAG, "Error sending color to queue");
    }
}

void callback_5000ms(){
    LED_Status led;
    memset(&led, 0, sizeof(LED_Status));

    led.color = 2;
    led.freq = 1000;

    if(xQueueSend( xQueueLedBuffer, (void *)&led, portMAX_DELAY ) == pdFAIL) {
        ESP_LOGE(TAG, "Error sending color to queue");
    }
}

//**********************************************************************************************************
const char * state_to_string[STATE_MAX] = {"IDLE","CONNECTED", "DISCONNECTED", "RUN", "CALIBRATION", "SLEEP"};

static void set_state(midi_controller_state_e new_state) {
    ESP_LOGI(TAG, "State [%s] -> [%s]", state_to_string[bio_midi_state], state_to_string[new_state]);
    bio_midi_state = new_state;
}

static void set_led_color(led_color_e color, uint16_t freq) {
    LED_Status led;
    memset(&led, 0, sizeof(LED_Status));

    led.color = color;
    led.freq = freq;

    if(xQueueSend( xQueueLedBuffer, (void *)&led, portMAX_DELAY ) == pdFAIL) {
        ESP_LOGE(TAG, "Error sending color to queue");
    }
}

static void set_led_color_rgb(uint8_t r, uint8_t g, uint8_t b, uint16_t freq) {
    LED_Status led;
    memset(&led, 0, sizeof(LED_Status));

    led.color = LED_RGB;
    led.freq = freq;
    led.r = r;
    led.g = g;
    led.b = b;

    if(xQueueSend( xQueueLedBuffer, (void *)&led, portMAX_DELAY ) == pdFAIL) {
        ESP_LOGE(TAG, "Error sending color to queue");
    }
}
//**********************************************************************************************************
// BLEMidi Callbacks
void onConnect(){
    set_led_color_rgb(96, 0, 128, 0);
    set_state(STATE_CONNECTED);
}
void onDisconnect(){
    set_led_color(LED_RED, 1000);
    set_state(STATE_DISCONNECTED);
}

//**********************************************************************************************************
// Midi Controller Task

void vMidiController(void * pvParameters) {

    xTaskCreatePinnedToCore(vMPU6050Task,
                            "vMPU6050Task",
                            STACK_SIZE_2048,
                            NULL,
                            osPriorityNormal,
                            &xTaskMPUHandle,
                            0
                            );

    xTaskCreatePinnedToCore(vLedControlTask,
                            "vLedControlTask",
                            STACK_SIZE_2048,
                            NULL,
                            osPriorityNormal,
                            &xTaskLedControlHandle,
                            APP_CPU_NUM
                            );

    xTaskCreatePinnedToCore(vTouchButton,
                            "vTouchButton",
                            STACK_SIZE_2048,
                            NULL,
                            osPriorityNormal,
                            &xTaskTouchHandle,
                            APP_CPU_NUM
                            );

    xTaskCreatePinnedToCore(vMic,
                            "vMic",
                            STACK_SIZE_2048 * 4,
                            NULL,
                            osPriorityNormal,
                            &xTaskMicHandle,
                            APP_CPU_NUM
                            );

    xTaskCreatePinnedToCore(vBMP280Task,
                            "vBMP280Task",
                            STACK_SIZE_2048 * 2,
                            NULL,
                            osPriorityNormal,
                            &xTaskBMPHandle,
                            APP_CPU_NUM
                            );

    xTaskCreatePinnedToCore(vHeartRateTask,
                            "vHeartRateTask",
                            STACK_SIZE_2048,
                            NULL,
                            osPriorityNormal,
                            &xTaskHeartRateHandle,
                            APP_CPU_NUM
                            );

    //TODO - Clear memory after complete tasks initialization

    // Add touch callbacks
    touch_add_callback(2000, callback_2000ms);
    touch_add_callback(5000, callback_5000ms);

    while(1) {
        switch(bio_midi_state) {
            case STATE_IDLE:
            {
                // Connect BLE
                int status = blemidi_init(NULL, onConnect, onDisconnect);
                if( status < 0 ) {
                    ESP_LOGE(TAG, "BLE MIDI Driver returned status=%d", status);
                } else {
                    ESP_LOGI(TAG, "BLE MIDI Driver initialized successfully");
                    set_state(STATE_RUN);
                }

            }
            break;
            case STATE_CONNECTED:
            break;
            case STATE_DISCONNECTED:

            break;
            case STATE_RUN:
            break;
            case STATE_SLEEP:
            break;
            case STATE_CALIBRATION:
            break;
            default:
                ESP_LOGE(TAG, "Unknown state");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}
