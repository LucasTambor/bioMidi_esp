/* cmd_i2ctools.c

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "argtable3/argtable3.h"
#include "i2c_driver.h"
#include "i2c_app.h"
#include "esp_console.h"
#include "esp_log.h"

static const char *TAG = "cmd_i2ctools";


static int do_i2cdetect_cmd(int argc, char **argv)
{
    uint8_t address;
    bool ret = 0;
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");
    for (int i = 0; i < 128; i += 16) {
        printf("%02x: ", i);
        for (int j = 0; j < 16; j++) {
            fflush(stdout);
            address = i + j;
            ret = i2c_detect(address);
            if (!ret) {
                printf("%02x ", address);
            } else {
                printf("-- ");
            }
        }
        printf("\r\n");
    }

    return 0;
}

static void register_i2cdectect(void)
{
    const esp_console_cmd_t i2cdetect_cmd = {
        .command = "i2cdetect",
        .help = "Scan I2C bus for devices",
        .hint = NULL,
        .func = &do_i2cdetect_cmd,
        .argtable = NULL
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&i2cdetect_cmd));
}

static struct {
    struct arg_int *chip_address;
    struct arg_int *register_address;
    struct arg_int *data_length;
    struct arg_end *end;
} i2cget_args;

static int do_i2cget_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&i2cget_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, i2cget_args.end, argv[0]);
        return 0;
    }

    /* Check chip address: "-c" option */
    int chip_addr = i2cget_args.chip_address->ival[0];
    /* Check register address: "-r" option */
    int data_addr = -1;
    if (i2cget_args.register_address->count) {
        data_addr = i2cget_args.register_address->ival[0];
    }
    /* Check data length: "-l" option */
    int len = 1;
    if (i2cget_args.data_length->count) {
        len = i2cget_args.data_length->ival[0];
    }
    uint8_t *data = malloc(len);

    portBASE_TYPE xStatus;
    I2C_Status i2c_command;
    memset(&i2c_command, 0, sizeof(I2C_Status));

    i2c_command.device_address = chip_addr;
    i2c_command.register_address = data_addr;
    i2c_command.data = data;
    i2c_command.size = len;

    xStatus = xQueueSend( xQueueI2CReadBuffer, (void *)&i2c_command, portMAX_DELAY );

    // Wait for end of writing process
    xStatus = xSemaphoreTake( xBinarySemaphoreI2CAppEndOfRead, portMAX_DELAY );

    if (xStatus == pdFAIL)
	{
		return 1;
	}

    for (int i = 0; i < len; i++) {
        printf("0x%02x ", data[i]);
        if ((i + 1) % 16 == 0) {
            printf("\r\n");
        }
    }
    printf("\n");
    free(data);
    return 0;
}

static void register_i2cget(void)
{
    i2cget_args.chip_address = arg_int1("c", "chip", "<chip_addr>", "Specify the address of the chip on that bus");
    i2cget_args.register_address = arg_int0("r", "register", "<register_addr>", "Specify the address on that chip to read from");
    i2cget_args.data_length = arg_int0("l", "length", "<length>", "Specify the length to read from that data address");
    i2cget_args.end = arg_end(1);
    const esp_console_cmd_t i2cget_cmd = {
        .command = "i2cget",
        .help = "Read registers visible through the I2C bus",
        .hint = NULL,
        .func = &do_i2cget_cmd,
        .argtable = &i2cget_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&i2cget_cmd));
}

static struct {
    struct arg_int *chip_address;
    struct arg_int *register_address;
    struct arg_int *data;
    struct arg_end *end;
} i2cset_args;

static int do_i2cset_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&i2cset_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, i2cset_args.end, argv[0]);
        return 0;
    }

    /* Check chip address: "-c" option */
    int chip_addr = i2cset_args.chip_address->ival[0];
    /* Check register address: "-r" option */
    int data_addr = 0;
    if (i2cset_args.register_address->count) {
        data_addr = i2cset_args.register_address->ival[0];
    }
    /* Check data: "-d" option */
    int len = i2cset_args.data->count;


    portBASE_TYPE xStatus;
    I2C_Status i2c_command;
    memset(&i2c_command, 0, sizeof(I2C_Status));
    uint8_t * data = (uint8_t *) i2cset_args.data->ival;


    i2c_command.device_address = chip_addr;
    i2c_command.register_address = data_addr;
    i2c_command.data = data;
    i2c_command.size = len;

    xStatus = xQueueSend( xQueueI2CWriteBuffer, (void *)&i2c_command, portMAX_DELAY );

    // Wait for end of writing process
    xStatus = xSemaphoreTake( xBinarySemaphoreI2CAppEndOfWrite, portMAX_DELAY );

    if (xStatus == pdFAIL)
	{
		return 1;
	}
    
    return 0;
}

static void register_i2cset(void)
{
    i2cset_args.chip_address = arg_int1("c", "chip", "<chip_addr>", "Specify the address of the chip on that bus");
    i2cset_args.register_address = arg_int0("r", "register", "<register_addr>", "Specify the address on that chip to read from");
    i2cset_args.data = arg_intn(NULL, NULL, "<data>", 0, 256, "Specify the data to write to that data address");
    i2cset_args.end = arg_end(2);
    const esp_console_cmd_t i2cset_cmd = {
        .command = "i2cset",
        .help = "Set registers visible through the I2C bus",
        .hint = NULL,
        .func = &do_i2cset_cmd,
        .argtable = &i2cset_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&i2cset_cmd));
}

// static struct {
//     struct arg_int *chip_address;
//     struct arg_int *size;
//     struct arg_end *end;
// } i2cdump_args;

// static int do_i2cdump_cmd(int argc, char **argv)
// {
//     int nerrors = arg_parse(argc, argv, (void **)&i2cdump_args);
//     if (nerrors != 0) {
//         arg_print_errors(stderr, i2cdump_args.end, argv[0]);
//         return 0;
//     }

//     /* Check chip address: "-c" option */
//     int chip_addr = i2cdump_args.chip_address->ival[0];
//     /* Check read size: "-s" option */
//     int size = 1;
//     if (i2cdump_args.size->count) {
//         size = i2cdump_args.size->ival[0];
//     }
//     if (size != 1 && size != 2 && size != 4) {
//         ESP_LOGE(TAG, "Wrong read size. Only support 1,2,4");
//         return 1;
//     }
//     i2c_driver_install(i2c_port, I2C_MODE_MASTER, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
//     i2c_master_driver_initialize();
//     uint8_t data_addr;
//     uint8_t data[4];
//     int32_t block[16];
//     printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f"
//            "    0123456789abcdef\r\n");
//     for (int i = 0; i < 128; i += 16) {
//         printf("%02x: ", i);
//         for (int j = 0; j < 16; j += size) {
//             fflush(stdout);
//             data_addr = i + j;
//             i2c_cmd_handle_t cmd = i2c_cmd_link_create();
//             i2c_master_start(cmd);
//             i2c_master_write_byte(cmd, chip_addr << 1 | WRITE_BIT, ACK_CHECK_EN);
//             i2c_master_write_byte(cmd, data_addr, ACK_CHECK_EN);
//             i2c_master_start(cmd);
//             i2c_master_write_byte(cmd, chip_addr << 1 | READ_BIT, ACK_CHECK_EN);
//             if (size > 1) {
//                 i2c_master_read(cmd, data, size - 1, ACK_VAL);
//             }
//             i2c_master_read_byte(cmd, data + size - 1, NACK_VAL);
//             i2c_master_stop(cmd);
//             esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 50 / portTICK_RATE_MS);
//             i2c_cmd_link_delete(cmd);
//             if (ret == ESP_OK) {
//                 for (int k = 0; k < size; k++) {
//                     printf("%02x ", data[k]);
//                     block[j + k] = data[k];
//                 }
//             } else {
//                 for (int k = 0; k < size; k++) {
//                     printf("XX ");
//                     block[j + k] = -1;
//                 }
//             }
//         }
//         printf("   ");
//         for (int k = 0; k < 16; k++) {
//             if (block[k] < 0) {
//                 printf("X");
//             }
//             if ((block[k] & 0xff) == 0x00 || (block[k] & 0xff) == 0xff) {
//                 printf(".");
//             } else if ((block[k] & 0xff) < 32 || (block[k] & 0xff) >= 127) {
//                 printf("?");
//             } else {
//                 printf("%c", block[k] & 0xff);
//             }
//         }
//         printf("\r\n");
//     }
//     i2c_driver_delete(i2c_port);
//     return 0;
// }

// static void register_i2cdump(void)
// {
//     i2cdump_args.chip_address = arg_int1("c", "chip", "<chip_addr>", "Specify the address of the chip on that bus");
//     i2cdump_args.size = arg_int0("s", "size", "<size>", "Specify the size of each read");
//     i2cdump_args.end = arg_end(1);
//     const esp_console_cmd_t i2cdump_cmd = {
//         .command = "i2cdump",
//         .help = "Examine registers visible through the I2C bus",
//         .hint = NULL,
//         .func = &do_i2cdump_cmd,
//         .argtable = &i2cdump_args
//     };
//     ESP_ERROR_CHECK(esp_console_cmd_register(&i2cdump_cmd));
// }

void register_i2ctools(void)
{
    register_i2cdectect();
    register_i2cget();
    register_i2cset();
    // register_i2cdump();
}
