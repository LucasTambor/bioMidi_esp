#ifndef _MIC_DRIVER_H_
#define _MIC_DRIVER_H_

#include <stdio.h>
#include <driver/i2s.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define I2S_PORT I2S_NUM_0

#define I2S_WS      25
#define I2S_SD      33
#define I2S_SCK     26

#define I2S_READ_BUFFER_SIZE    (1024)
#define I2S_AUDIO_BUFFER_SIZE    (I2S_READ_BUFFER_SIZE/4)
#define I2S_SAMPLE_RATE         8000

esp_err_t mic_init();

esp_err_t mic_read_buffer(uint8_t * sample, size_t * bytes_read);

// i2s reader queue
extern QueueHandle_t xQueueI2S;

#endif //_MIC_DRIVER_H_