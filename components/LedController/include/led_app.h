#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern QueueHandle_t xQueueLedBuffer;

typedef enum {
    LED_OFF = 0,
    LED_RED,
    LED_GREEN,
    LED_BLUE,
    LED_RAINBOW,
    LED_RGB
} led_color_e;

typedef struct
{
    bool status;
    led_color_e color;
    uint16_t freq;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} LED_Status;

void vLedControlTask( void *pvParameters );


#ifdef __cplusplus
}
#endif