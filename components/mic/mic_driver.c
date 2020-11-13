#include "mic_driver.h"

QueueHandle_t xQueueI2S;

esp_err_t mic_init() {

    const i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // default interrupt priority
        .dma_buf_count = 4,
        .dma_buf_len = I2S_READ_BUFFER_SIZE,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    const i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD
    };

    ESP_ERROR_CHECK(i2s_driver_install(I2S_PORT, &i2s_config, 4, &xQueueI2S));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_PORT, &pin_config));
    return ESP_OK;
}

esp_err_t mic_read_buffer(uint8_t * sample, size_t * bytes_read) {
    if(sample == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2s_read(I2S_PORT, (void*)sample, I2S_READ_BUFFER_SIZE, bytes_read, portMAX_DELAY);
}
