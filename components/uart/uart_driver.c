#include "uart_driver.h"


esp_err_t uart_init() {
    esp_err_t err = ESP_OK;
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    err = uart_param_config(UART_NUM_1, &uart_config);
    if(err != ESP_OK) {
        return err;
    }

    err = uart_set_pin(UART_NUM_1, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if(err != ESP_OK) {
        return err;
    }

    err = uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, TX_BUF_SIZE * 2, 0, NULL, 0);
    if(err != ESP_OK) {
        return err;
    }

    return ESP_OK;
}

int uart_write(char * data, uint8_t len) {
    if(data == NULL) return 0;
    return uart_write_bytes(UART_NUM_1, (const char *)data, len);
}
