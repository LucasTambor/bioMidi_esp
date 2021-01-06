#ifndef _TOUCH_APP_H_
#define _TOUCH_APP_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void vTouchButton( void *pvParameters );

void touch_add_callback(uint16_t ms, void (*func)());

#endif //_TOUCH_APP_H_
