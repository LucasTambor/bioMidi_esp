
#include "cmd_uart.h"
#include "uart_app.h"
#include <stdio.h>
#include <string.h>
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "esp_log.h"

static const char *TAG = "cmd_uart";
static const char *UART_ID = "CONSOLE_CMD";

static struct {
    struct arg_dbl *data;
    struct arg_int *id;
    struct arg_end *end;
} uart_write_args;

static int cmd_uart_write(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&uart_write_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, uart_write_args.end, argv[0]);
        return 0;
    }

    uint8_t data_size = uart_write_args.data->count;
    uint8_t id_size = uart_write_args.id->count;

    if(data_size != id_size) {
        ESP_LOGE(TAG, "ID don't match DATA, id=%d | data=%d", id_size, data_size);
        return 1;
    }

    portBASE_TYPE xStatus;
    uart_data_t uart_data;
    memset(&uart_data, 0, sizeof(uart_data_t));

    for(uint8_t i = 0; i<data_size; i++) {
        uart_data.id = uart_write_args.id->ival[i];
        uart_data.data = uart_write_args.data->dval[i];

        xStatus = xQueueSend( xQueueUartWriteBuffer, (void *)&uart_data, portMAX_DELAY );
        if (xStatus == pdFAIL) {
            ESP_LOGE(TAG, "ERROR sendig data to queue");
        	return 1;
        }
    }
    return 0;
}

static void register_uart_write(void)
{
    uart_write_args.data = arg_dbln("d", "data", "<data>", 0, 256, "Specify the data to write to UART");
    uart_write_args.id = arg_intn("i", "id", "<ID>", 0, 256, "ID = 1 -> 8");
    uart_write_args.end = arg_end(1);
    const esp_console_cmd_t uart_write_cmd = {
        .command = "uart_wr",
        .help = "Write to UART",
        .hint = NULL,
        .func = &cmd_uart_write,
        .argtable = &uart_write_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&uart_write_cmd));
}

void register_uart(void)
{
    register_uart_write();
}
