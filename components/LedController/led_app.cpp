#include "led_app.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "ESP32RMTChannel.h"
#include "DStrip.h"
#include "DLEDController.h"

#include "common.h"

//****************************************************************************************************************

#define LED_CTR_QUEUE_SIZE  1
#define LED_CTR_CYCLE_MS    20
const char* TAG = "LED_Control";

static const uint8_t  cfgOutputPin = 14;    // the GPIO where LEDs are connected
static const uint8_t  cfgChannel   = 0;     // ESP32 RMT's channel [0 ... 7]
static const uint16_t cfgLEDcount  = 1;    // 64 LEDS
static const uint8_t  cfgMaxCCV    = 32;    // maximum value allowed for color component

DStrip strip;
DLEDController LEDcontroller;
ESP32RMTChannel rmtChannel;

QueueHandle_t xQueueLedBuffer;
SemaphoreHandle_t xBinarySemaphoreLed;
TimerHandle_t xTimerLed;

static LED_Status actual_led = {0, LED_OFF, 0, 0 , 0, 0};

//****************************************************************************************************************

extern "C" {
    static void vLedTimerCallback(TimerHandle_t pxTimer) {
        actual_led.status = !actual_led.status;
    }

    static void set_led_color(led_color_e color, uint8_t r = 0, uint8_t g = 0, uint8_t b = 0) {
        switch(color) {
            case LED_OFF:
                strip.SetPixel(0, 0, 0, 0);
                break;
            case LED_RED:
                strip.SetPixel(0, 255, 0, 0);
                break;
            case LED_GREEN:
                strip.SetPixel(0, 0, 255, 0);
                break;
            case LED_BLUE:
                strip.SetPixel(0, 0, 0, 255);
                break;
            case LED_RGB:
                strip.SetPixel(0, r, g, b);
                break;
            default:
                break;
        }
    }

    void vLedControlTask(void *taskParameter) {
        LED_Status lReceivedValue;
	    portBASE_TYPE xStatus;

        // Create Queue
        xQueueLedBuffer = xQueueCreate(LED_CTR_QUEUE_SIZE, sizeof(LED_Status));


        // Initialize LED
        strip.Create(3, cfgLEDcount, cfgMaxCCV);
        rmtChannel.Initialize((rmt_channel_t)cfgChannel, (gpio_num_t)cfgOutputPin, cfgLEDcount * 24);
        rmtChannel.ConfigureForWS2812x();
        LEDcontroller.SetLEDType(LEDType::WS2812B);

        // Signalize task successfully creation
        xEventGroupSetBits(xEventGroupTasks, BIT_TASK_LED_CONTROL);
        ESP_LOGI(TAG, "LED Control Initialized");
        
        bool rainbow_status = true;
        uint16_t rainbow_step = 0;
        while(1) {
		    xStatus = xQueueReceive( xQueueLedBuffer, (void *)&lReceivedValue, LED_CTR_CYCLE_MS / portTICK_PERIOD_MS );
            if( xStatus == pdPASS ) {
                ESP_LOGI(TAG, "Recived from queue: Color %d, Freq: %d", lReceivedValue.color, lReceivedValue.freq);
                rainbow_status = false;
                actual_led.color = lReceivedValue.color;
                if(actual_led.freq != lReceivedValue.freq) {
                    actual_led.freq = lReceivedValue.freq;
                    if(xTimerLed != NULL) xTimerDelete(xTimerLed, 0);
                    if(actual_led.freq) {
                        // Create timer
                        xTimerLed = xTimerCreate("LedTimer", pdMS_TO_TICKS(actual_led.freq), pdTRUE, (void *)0, vLedTimerCallback);
                        xTimerStart(xTimerLed, 0);
                    }
                }
                if(!actual_led.freq) {
                    actual_led.status = true;
                }

                if(actual_led.color == LED_RAINBOW) {
                    rainbow_status = true;
                }

                actual_led.r = lReceivedValue.r;
                actual_led.g = lReceivedValue.g;
                actual_led.b = lReceivedValue.b;
            }
            if(rainbow_status) {
                strip.RainbowStep(rainbow_step);
                rainbow_step++;
            }else {
                if(actual_led.status) {
                    set_led_color(actual_led.color, actual_led.r, actual_led.g, actual_led.b);
                } else {
                    set_led_color(LED_OFF);
                }
            }

            LEDcontroller.SetLEDs(strip.description.data, strip.description.dataLen, &rmtChannel);
        }
        vTaskDelete(NULL);
    }

}
