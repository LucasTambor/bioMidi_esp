#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "cmd_system.h"
#include "cmd_nvs.h"
// #include "cmd_mpu6050.h"
#include "cmd_i2ctools.h"
#include "cmd_led.h"
#include "cmd_uart.h"
#include "cmd_bmp.h"
#include "cmd_mpu6050.h"

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"

void vTaskConsole(void *pvParameters);

#ifdef __cplusplus
}
#endif



