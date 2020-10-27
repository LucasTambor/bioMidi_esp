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

//**********************************************************************************************************

static const char* TAG = "Main";

//**********************************************************************************************************
// Prototypes
static void initialize_filesystem(void);
static void initialize_nvs(void);

//**********************************************************************************************************
// Task Handlers
TaskHandle_t xTaskConsoleHandle;


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

    //Core 1
    xTaskCreatePinnedToCore(vTaskConsole,
                            "TaskConsole",
                            configMINIMAL_STACK_SIZE + 10000,
                            NULL,
                            1,
                            &xTaskConsoleHandle,
                            APP_CPU_NUM);


    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
