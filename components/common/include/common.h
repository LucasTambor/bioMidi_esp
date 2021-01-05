#ifndef _COMMON_H_
#define _COMMON_H_

#include "freertos/event_groups.h"

/**
 * Event Group Bit Definition
 *
 * Necessary for task synchronization
*/

#define BIT_TASK_I2C_WRITE      (1<<0)
#define BIT_TASK_I2C_READ       (1<<1)
#define BIT_TASK_MPU6050        (1<<2)
#define BIT_TASK_LED_CONTROL    (1<<3)
#define BIT_TASK_TOUCH_BUTTON   (1<<4)
#define BIT_TASK_MIC            (1<<5)
#define BIT_TASK_BMP280         (1<<6)
#define BIT_TASK_HEART_RATE     (1<<7)
#define BIT_TASK_DATA_STREAM    (1<<8)
#define BIT_TASK_CONSOLE        (1<<9)



extern EventGroupHandle_t xEventGroupTasks;



#endif //_COMMON_H_