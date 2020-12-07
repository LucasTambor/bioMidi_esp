
#include "cmd_bmp.h"
#include "bmp280_app.h"
#include <stdio.h>
#include <string.h>
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "esp_log.h"

static const char *TAG = "cmd_bmp";


static int cmd_bmp280_read(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    bmp280_data_t data;

    ESP_ERROR_CHECK(bmp280_app_read_data(&data));

    printf("\nTemperature: %.2f: \nPressure: %.2f\n", data.temperature, data.pressure);

    return 0;
}

static void register_bmp280_read(void)
{
    const esp_console_cmd_t bmp280_read_cmd = {
        .command = "bmp_read",
        .help = "Read Temperature and Pressure data",
        .hint = NULL,
        .func = &cmd_bmp280_read
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&bmp280_read_cmd));
}

void register_bmp280(void)
{
    register_bmp280_read();
}
