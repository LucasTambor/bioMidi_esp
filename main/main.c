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

#include "task_console.h"
// #include "task_mpu6050.h"
// #include "mpu6050_application.h"
#include "i2c_app.h"
#include "i2c_driver.h"

#define STACK_SIZE_2048 2048

#define  osPriorityIdle             configMAX_PRIORITIES - 6
#define  osPriorityLow              configMAX_PRIORITIES - 5
#define  osPriorityBelowNormal      configMAX_PRIORITIES - 4
#define  osPriorityNormal       	configMAX_PRIORITIES - 3
#define  osPriorityAboveNormal      configMAX_PRIORITIES - 2
#define  osPriorityHigh             configMAX_PRIORITIES - 1
#define  osPriorityRealtime         configMAX_PRIORITIES - 0


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

static void initialize_filesystem(void) {
    ESP_LOGI(TAG, "initialize_filesystem");

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
}

static void initialize_nvs(void) {
    ESP_LOGI(TAG, "initialize_nvs");

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}



void app_main(void)
{
    initialize_nvs();

    initialize_filesystem();

    i2c_init();
    //Core 1
    xTaskCreatePinnedToCore(vTaskConsole,
                            "TaskConsole",
                            configMINIMAL_STACK_SIZE + 10000,
                            NULL,
                            osPriorityNormal,
                            &xTaskConsoleHandle,
                            APP_CPU_NUM);

    xTaskCreatePinnedToCore(vI2CWrite,            /* Function that implements the task. */
                            "vI2CWrite",          /* Text name for the task. */
                            STACK_SIZE_2048,      /* Number of indexes in the xStack array. */
                            NULL,                 /* Parameter passed into the task. */
                            osPriorityHigh,       /* Priority at which the task is created. */
                            &xTaskI2CWriteHandle, /* Variable to hold the task's data structure. */
                            0                     /*  0 for PRO_CPU, 1 for APP_CPU, or tskNO_AFFINITY which allows the task to run on both */
                            );

    xTaskCreatePinnedToCore(vI2CRead,            /* Function that implements the task. */
                            "vI2CRead",          /* Text name for the task. */
                            STACK_SIZE_2048,     /* Number of indexes in the xStack array. */
                            NULL,                /* Parameter passed into the task. */
                            osPriorityHigh,      /* Priority at which the task is created. */
                            &xTaskI2CReadHandle, /* Variable to hold the task's data structure. */
                            0                    /*  0 for PRO_CPU, 1 for APP_CPU, or tskNO_AFFINITY which allows the task to run on both */
                            );

    // xTaskCreatePinnedToCore(vTaskMPU6050,        /* Function that implements the task. */
    //                         "vTaskMPU6050",      /* Text name for the task. */
    //                         STACK_SIZE_2048,     /* Number of indexes in the xStack array. */
    //                         NULL,                /* Parameter passed into the task. */
    //                         osPriorityNormal,    /* Priority at which the task is created. */
    //                         &xTaskMPU6050Handle, /* Variable to hold the task's data structure. */
    //                         0                    /*  0 for PRO_CPU, 1 for APP_CPU, or tskNO_AFFINITY which allows the task to run on both */
    //                         );


    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
