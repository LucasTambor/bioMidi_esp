#ifndef _COMMON_H_
#define _COMMON_H_

#include "freertos/queue.h"
#include "freertos/event_groups.h"

/**
 * Event Group Tasks Bit Definition
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

/**
 * Event Group App Bit Definition
 *
 * Necessary for task synchronization
*/

#define BIT_APP_SEND_DATA      (1<<0)

extern EventGroupHandle_t xEventGroupApp;

#define STACK_SIZE_2048 2048

#define  osPriorityIdle             configMAX_PRIORITIES - 6
#define  osPriorityLow              configMAX_PRIORITIES - 5
#define  osPriorityBelowNormal      configMAX_PRIORITIES - 4
#define  osPriorityNormal       	configMAX_PRIORITIES - 3
#define  osPriorityAboveNormal      configMAX_PRIORITIES - 2
#define  osPriorityHigh             configMAX_PRIORITIES - 1
#define  osPriorityRealtime         configMAX_PRIORITIES - 0

// Queue for the application to receive data from the other tasks
extern QueueHandle_t xQueueAppData;

typedef enum {
    DATA_ID_ROLL = 0,
    DATA_ID_PITCH,
    DATA_ID_YAW,
    DATA_ID_TEMPERATURE,
    DATA_ID_PRESSURE,
    DATA_ID_FFT,
    DATA_ID_HEART_RATE,
    DATA_ID_MAX,
}data_id_e;


typedef struct {
    data_id_e id;
    float data;
}app_data_t;
#endif //_COMMON_H_