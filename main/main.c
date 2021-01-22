/**
 * BioMidi
 *
*/

#include <stdio.h>
#include <string.h>
#include "soc/soc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_vfs_fat.h"
#include "esp_vfs_dev.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "common.h"
#include "task_console.h"
#include "i2c_app.h"
#include "i2c_driver.h"
#include "uart_app.h"
#include "midi_controller.h"

//**********************************************************************************************************

static const char* TAG = "Main";

//**********************************************************************************************************
// Prototypes
static void initialize_filesystem(void);
static void initialize_nvs(void);

//**********************************************************************************************************
// Task Handlers
TaskHandle_t xTaskConsoleHandle;
TaskHandle_t xTaskMPU6050Handle;
TaskHandle_t xTaskI2CReadHandle;
TaskHandle_t xTaskI2CWriteHandle;
TaskHandle_t xTaskUartHandle;


//**********************************************************************************************************
// Event Groups
EventGroupHandle_t xEventGroupTasks;
EventGroupHandle_t xEventGroupApp;

//**********************************************************************************************************

static void initialize_filesystem(void) {

    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount(MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "Filesystem Initialized");
}

//**********************************************************************************************************

static void initialize_nvs(void) {

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_LOGI(TAG, "NVS Initialized");
}

//**********************************************************************************************************

void app_main(void)
{
    initialize_nvs();

    initialize_filesystem();

    i2c_init();

    // Initialize tasks event group
    xEventGroupTasks = xEventGroupCreate();
    if(xEventGroupTasks == NULL) {
        ESP_LOGE(TAG, "ERROR Creating xEventGroupTasks");
    }
    // Initialize app event group
    xEventGroupApp = xEventGroupCreate();
    if(xEventGroupApp == NULL) {
        ESP_LOGE(TAG, "ERROR Creating xEventGroupApp");
    }

    //Core 1
    xTaskCreatePinnedToCore(vI2CWrite,
                            "vI2CWrite",
                            STACK_SIZE_2048 * 2,
                            NULL,
                            osPriorityHigh,
                            &xTaskI2CWriteHandle,
                            APP_CPU_NUM
                            );

    xTaskCreatePinnedToCore(vI2CRead,
                            "vI2CRead",
                            STACK_SIZE_2048 * 2,
                            NULL,
                            osPriorityHigh,
                            &xTaskI2CReadHandle,
                            APP_CPU_NUM
                            );

    xTaskCreatePinnedToCore(vTaskConsole,                           /* Function that implements the task. */
                            "TaskConsole",                          /* Text name for the task. */
                            configMINIMAL_STACK_SIZE + 10000,       /* Number of indexes in the xStack array. */
                            NULL,                                   /* Parameter passed into the task. */
                            osPriorityNormal,                       /* Priority at which the task is created. */
                            &xTaskConsoleHandle,                    /* Variable to hold the task's data structure. */
                            APP_CPU_NUM);                           /*  0 for PRO_CPU, 1 for APP_CPU, or tskNO_AFFINITY which allows the task to run on both */



    xTaskCreatePinnedToCore(vDataStream,
                            "vDataStream",
                            STACK_SIZE_2048 * 2,
                            NULL,
                            osPriorityHigh,
                            &xTaskUartHandle,
                            APP_CPU_NUM);

    //Core 0
    xTaskCreatePinnedToCore(vMidiController,
                            "vMidiController",
                            STACK_SIZE_2048 * 8,
                            NULL,
                            osPriorityHigh,
                            &xTaskUartHandle,
                            PRO_CPU_NUM);



    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
