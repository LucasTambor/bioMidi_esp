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
#include "battery_app.h"

#include "blemidi.h"

TaskHandle_t xTaskLedControlHandle;
TaskHandle_t xTaskTouchHandle;
TaskHandle_t xTaskMicHandle;
TaskHandle_t xTaskBMPHandle;
TaskHandle_t xTaskMPUHandle;
TaskHandle_t xTaskHeartRateHandle;
TaskHandle_t xTaskBatteryHandle;

//**********************************************************************************************************

static const char* TAG = "MidiControler";
static midi_controller_state_e bio_midi_state = STATE_IDLE;

QueueHandle_t xQueueAppData;
static app_data_t dataReceived;

// Flag connected to BLE
static int8_t midi_connected = 0;
//**********************************************************************************************************

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

const char * state_to_string[STATE_MAX] = {"IDLE","WAITING_CONNECTION","CONNECTED", "DISCONNECTED", "RUN", "CALIBRATION", "SLEEP"};

static void set_state(midi_controller_state_e new_state) {
    ESP_LOGI(TAG, "State [%s] -> [%s]", state_to_string[bio_midi_state], state_to_string[new_state]);
    bio_midi_state = new_state;

    // Set App Group bit
    if(new_state == STATE_RUN) {
        xEventGroupSetBits(xEventGroupApp, BIT_APP_SEND_DATA);
    }else{
        xEventGroupClearBits(xEventGroupApp, BIT_APP_SEND_DATA);
    }

    //Led Color according to state
    switch(new_state) {
            case STATE_IDLE:
            {
                set_led_color(LED_RAINBOW, 0);
            }
            break;
            case STATE_WAITING_CONNECTION:
                set_led_color(LED_RAINBOW, 0);

            break;
            case STATE_CONNECTED:
                set_led_color_rgb(96, 0, 128, 0);
            break;
            case STATE_DISCONNECTED:
                set_led_color(LED_RED, 1000);
            break;
            case STATE_RUN:

                set_led_color_rgb(96, 0, 128, 0);
            break;
            case STATE_SLEEP:
            break;
            case STATE_CALIBRATION:
                set_led_color(LED_BLUE, 500);
            break;
            default:
                ESP_LOGE(TAG, "Unknown state");
            break;
    }
}



//**********************************************************************************************************
// Touch Callbacks

// Calibrate Sensors
void callback_2000ms(){
    set_state(STATE_CALIBRATION);
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
// BLEMidi Callbacks
void onConnect(){
    set_state(STATE_CONNECTED);
}
void onDisconnect(){
    set_state(STATE_DISCONNECTED);
}

//**********************************************************************************************************
// Midi Data processing
const char * data_id_to_string[DATA_ID_MAX] = {"DATA_ID_ROLL","DATA_ID_PITCH", "DATA_ID_YAW", "DATA_ID_TEMPERATURE",
                                              "DATA_ID_PRESSURE", "DATA_ID_FFT", "DATA_ID_HEART_RATE"};

static uint8_t map_value_u8(float value, float in_min, float in_max, uint8_t out_min, uint8_t out_max) {
    if(value > in_max) return out_max;
    if(value < in_min) return out_min;
    return (uint8_t) ((value - in_min) * ((float) out_max - (float) out_min) / (in_max - in_min) + (float) out_min);
}

esp_err_t midi_proccess_data(data_id_e id, float value ) {
    // ESP_LOGI(TAG, "Received data from %s: %.2f", data_id_to_string[id], value);
    midi_message_t midi_message = {
        .status.status.type = MIDI_STATUS_CONTROL_CHANGE,
        .status.status.channel = BIOMIDI_MIDI_CHANNEL,
        .data = 0,
        .control_number = 0
    };
    static uint8_t last_value[DATA_ID_MAX] = {0};
    switch(id) {
        case DATA_ID_ROLL:
            midi_message.control_number = MIDI_CONTROLER_GPC_1;
            midi_message.data = map_value_u8(value, -180, 180, 0, 127);
        break;
        case DATA_ID_PITCH:
            midi_message.control_number = MIDI_CONTROLER_GPC_2;
            midi_message.data = map_value_u8(value, -180, 180, 0, 127);

        break;
        case DATA_ID_YAW:
            midi_message.control_number = MIDI_CONTROLER_GPC_3;
            midi_message.data = map_value_u8(value, -180, 180, 0, 127);

        break;
        case DATA_ID_TEMPERATURE:
            midi_message.control_number = MIDI_CONTROLER_GPC_5;
            midi_message.data = map_value_u8(value, 36, 37.5, 0, 127);

        break;
        case DATA_ID_PRESSURE:
            midi_message.control_number = MIDI_CONTROLER_GPC_6;
            midi_message.data = map_value_u8(value, 36, 37.5, 0, 127);
        break;
        case DATA_ID_FFT:

            midi_message.control_number = MIDI_CONTROLER_GPC_7;
            midi_message.data = map_value_u8(value, 150, 200, 0, 127);
            // ESP_LOGE(TAG, "FFT");
        break;
        case DATA_ID_HEART_RATE:
            midi_message.control_number = MIDI_CONTROLER_GPC_8;
            midi_message.data = map_value_u8(value, 20, 200, 0, 127);
        break;
        case DATA_ID_MAX:

        break;
        default:
            ESP_LOGE(TAG, "Unknow DATA ID");
            return ESP_ERR_INVALID_STATE;
        break;
    }

    uint8_t message[3];
    message[0] = (uint8_t) midi_message.status.midi_status;
    message[1] = (uint8_t) midi_message.control_number;
    message[2] = midi_message.data;

    // Do not send repeated messages
    if(last_value[id] == midi_message.data){
        last_value[id] = midi_message.data;
        return ESP_OK;
    }
    last_value[id] = midi_message.data;
    ESP_LOGI(TAG, "0x%X | 0x%X | 0x%X - %d ", message[0], message[1], message[2], message[2]);
    blemidi_tick();
    if(blemidi_send_message(0, message, 3) < 0 ) {
        ESP_LOGE(TAG, "ERROR sending message to BLEMIDI");
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

//**********************************************************************************************************
// Midi Controller Task

void vMidiController(void * pvParameters) {

    // init app queue
    xQueueAppData = xQueueCreate(16, sizeof(app_data_t));

    xTaskCreatePinnedToCore(vMPU6050Task,
                            "vMPU6050Task",
                            STACK_SIZE_2048 * 2,
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
                            osPriorityHigh,
                            &xTaskMicHandle,
                            APP_CPU_NUM
                            );

    xTaskCreatePinnedToCore(vBMP280Task,
                            "vBMP280Task",
                            STACK_SIZE_2048 * 3,
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

    xTaskCreatePinnedToCore(vBatteryMeasurementTask,
                            "vBattery",
                            STACK_SIZE_2048,
                            NULL,
                            osPriorityLow,
                            &xTaskBatteryHandle,
                            APP_CPU_NUM);

    //TODO - Clear memory after complete tasks initialization

    // Add touch callbacks
    touch_add_callback(2000, callback_2000ms);
    touch_add_callback(5000, callback_5000ms);

    set_state(STATE_IDLE);
    // esp_log_level_set(TAG, ESP_LOG_DEBUG);

    while(1) {

        // ESP_LOGI(TAG, "BioMidi State: %s", state_to_string[bio_midi_state], value);
        uint8_t battery_level;
        if(battery_app_read_percent(&battery_level) != ESP_OK) {
            ESP_LOGE(TAG, "ERROR Reading Battery Level");
        }
        blemidi_send_battery_level(battery_level);

        switch(bio_midi_state) {
            case STATE_IDLE:
            {
                // Connect BLE
                int status = blemidi_init(NULL, onConnect, onDisconnect);
                if( status < 0 ) {
                    ESP_LOGE(TAG, "BLE MIDI Driver returned status=%d", status);
                } else {
                    ESP_LOGI(TAG, "BLE MIDI Driver initialized successfully");
                    set_state(STATE_WAITING_CONNECTION);
                }

            }
            break;
            case STATE_WAITING_CONNECTION:
                midi_connected = 0;
                vTaskDelay(pdMS_TO_TICKS(1000));

            break;
            case STATE_CONNECTED:
                midi_connected = 1;
                set_state(STATE_RUN);
            break;
            case STATE_DISCONNECTED:
                midi_connected = -1;
                vTaskDelay(pdMS_TO_TICKS(1000));

            break;
            case STATE_RUN:
            {
                if( xQueueReceive( xQueueAppData, (void *)&dataReceived, portMAX_DELAY ) == pdPASS ) {
                    midi_proccess_data(dataReceived.id, dataReceived.data);
                }
            }
            break;
            case STATE_SLEEP:
            break;
            case STATE_CALIBRATION:
            {
                esp_err_t err = ESP_OK;
                err = mpu6050_app_calibrate(NULL);
                if(err != ESP_OK) {
                    ESP_LOGE(TAG, "ERROR Calibrating: %s", esp_err_to_name(err));
                }
                if(midi_connected < 0){
                    set_state(STATE_DISCONNECTED);
                } else if(midi_connected > 0) {
                    set_state(STATE_CONNECTED);
                }else {
                    set_state(STATE_WAITING_CONNECTION);
                }
            }
            break;
            default:
                ESP_LOGE(TAG, "Unknown state");
            break;
        }
        // vTaskDelay(pdMS_TO_TICKS(100));
    }

}
