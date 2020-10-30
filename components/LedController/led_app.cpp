#include "led_app.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "ESP32RMTChannel.h"
#include "DStrip.h"
#include "DLEDController.h"

#define LED_CTR_QUEUE_SIZE  1

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

extern "C" {

    void vLedControlTask(void *taskParameter) {
        LED_Status lReceivedValue;
	    portBASE_TYPE xStatus;

        xQueueLedBuffer = xQueueCreate(LED_CTR_QUEUE_SIZE, sizeof(LED_Status));

        // xBinarySemaphoreLed = xSemaphoreCreateBinary();
	    // configASSERT( xBinarySemaphoreLed );
        // if( xBinarySemaphoreLed != NULL ) {
        //     xSemaphoreTake( xBinarySemaphoreLed, 0 );
        // }

        strip.Create(3, cfgLEDcount, cfgMaxCCV);
        rmtChannel.Initialize((rmt_channel_t)cfgChannel, (gpio_num_t)cfgOutputPin, cfgLEDcount * 24);
        rmtChannel.ConfigureForWS2812x();
        LEDcontroller.SetLEDType(LEDType::WS2812B);

        bool rainbow_status = true;
        uint16_t rainbow_step = 0;
        while(1) {
		    xStatus = xQueueReceive( xQueueLedBuffer, (void *)&lReceivedValue, 20 / portTICK_PERIOD_MS );
            if( xStatus == pdPASS ) {
                ESP_LOGI(TAG, "Recived from queue: Color %d, Freq: %d", lReceivedValue.color, lReceivedValue.freq);
                rainbow_status = false;
                switch(lReceivedValue.color) {
                    case LED_GREEN:
                        strip.SetPixel(0, 0, 255, 0);
                        break;
                    case LED_RED:
                        strip.SetPixel(0, 255, 0, 0);
                        break;
                    case LED_BLUE:
                        strip.SetPixel(0, 0, 0, 255);
                        break;
                    case LED_RAINBOW:
                        rainbow_status = true;
                        rainbow_step = 0;
                        break;
                    default:
                        break;
                }
            }
            if(rainbow_status) {
                strip.RainbowStep(rainbow_step);
                rainbow_step++;
            }
            LEDcontroller.SetLEDs(strip.description.data, strip.description.dataLen, &rmtChannel);
        }
        vTaskDelete(NULL);
    }

}
