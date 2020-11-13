#ifndef _UART_DRIVER_H_
#define _UART_DRIVER_H_

#include <driver/uart.h>
#include "driver/gpio.h"

#define TX_PIN (GPIO_NUM_17)
#define RX_PIN (GPIO_NUM_16)
#define UART_BAUDRATE   115200

static const int RX_BUF_SIZE = 128;
static const int TX_BUF_SIZE = 128;

esp_err_t uart_init();

int uart_write(char * data, uint8_t len);

#endif // _UART_DRIVER_H_