#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern QueueHandle_t xQueueLedBuffer;

// //// Those semaphores are used only for apps who would like to block waiting for end of
// //// transmission or reception if not required to use it's ok
// extern SemaphoreHandle_t xBinarySemaphoreLed;

/* Structure to manipulate buffer sent or received over I2C */

typedef enum {
    LED_RED = 0,
    LED_GREEN,
    LED_BLUE,
    LED_RAINBOW
} led_color_e;

typedef struct
{
    led_color_e color;
    uint8_t freq;
} LED_Status;

void vLedControlTask( void *pvParameters );


#ifdef __cplusplus
}
#endif