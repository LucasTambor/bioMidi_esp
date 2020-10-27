#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "cmd_system.h"
#include "cmd_wifi.h"
#include "cmd_nvs.h"

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"

void vTaskConsole(void *pvParameters);

#ifdef __cplusplus
}
#endif



