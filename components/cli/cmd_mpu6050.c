
#include "cmd_mpu6050.h"
#include "mpu6050_app.h"
#include <stdio.h>
#include <string.h>
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "esp_log.h"

static const char *TAG = "cmd_mpu6050";

static int cmd_mpu6050_read_data(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    mpu6050_data_t data;

    ESP_ERROR_CHECK(mpu6050_app_read_data(&data));

    printf("\naccel_x: %.2f   \
    \naccel_y: %.2f   \
    \naccel_z: %.2f   \
    \ngyro_x: %.2f   \
    \ngyro_y: %.2f   \
    \ngyro_z: %.2f   \
    \ntempo: %.2f", data.accel_x, data.accel_y, data.accel_z, data.gyr_x, data.gyr_y, data.gyr_z, data.temperature);

    return 0;
}

static void register_mpu6050_read_data(void)
{
    const esp_console_cmd_t mpu6050_read_data_cmd = {
        .command = "mpu_data",
        .help = "Read data",
        .hint = NULL,
        .func = &cmd_mpu6050_read_data
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&mpu6050_read_data_cmd));
}

static int cmd_mpu6050_calibrate(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    mpu6050_data_t data;

    ESP_ERROR_CHECK(mpu6050_app_calibrate(&data));

    printf("\naccel_x: %.2f   \
    \naccel_y error: %.2f   \
    \ngyro_x error: %.2f   \
    \ngyro_y error: %.2f   \
    \ngyro_z error: %.2f", data.accel_x, data.accel_y, data.gyr_x, data.gyr_y, data.gyr_z);

    return 0;
}

static void register_mpu6050_calibrate(void)
{
    const esp_console_cmd_t mpu6050_calibrate_cmd = {
        .command = "mpu_calibrate",
        .help = "Calibrate error correction values",
        .hint = NULL,
        .func = &cmd_mpu6050_calibrate
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&mpu6050_calibrate_cmd));
}

void register_mpu6050(void)
{
    register_mpu6050_read_data();
    register_mpu6050_calibrate();
}
