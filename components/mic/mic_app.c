#include "mic_app.h"
#include "mic_driver.h"
#include "uart_app.h"
#include "esp_log.h"
#include "string.h"
#include "esp_dsp.h"
#include <math.h>
#include "common.h"

//******************************************************************************************************************

TaskHandle_t xTaskFFTHandle;

SemaphoreHandle_t xMicDataStreamEnableMutex;
SemaphoreHandle_t xMicFFTStreamEnableMutex;

// Queue to send sound data to FFT
static QueueHandle_t xQueueAudioData;

//******************************************************************************************************************
static bool mic_fft_stream = false;
static const char *TAG_FFT = "FFT_TASK";
void mic_enable_fft_stream() {
    if(xSemaphoreTake(xMicFFTStreamEnableMutex, portMAX_DELAY) == pdTRUE) {
        mic_fft_stream = true;
        ESP_LOGI(TAG_FFT, "Enable fft stream");
        xSemaphoreGive(xMicFFTStreamEnableMutex);
    }
}

void mic_disable_fft_stream() {
    if(xSemaphoreTake(xMicFFTStreamEnableMutex, portMAX_DELAY) == pdTRUE) {
        mic_fft_stream = false;
        ESP_LOGI(TAG_FFT, "Disable fft stream");
        xSemaphoreGive(xMicFFTStreamEnableMutex);
    }
}
typedef struct audio_data_s{
    int32_t * data;
    size_t len;
} audio_data_t;

void vTaskFFT(void *pvParameters) {
    esp_err_t err = ESP_OK;
    float wind[I2S_AUDIO_BUFFER_SIZE];
    float y_cf[I2S_AUDIO_BUFFER_SIZE*2];
    // Pointers to result arrays
    float* y1_cf = &y_cf[0];
    float* y2_cf = &y_cf[I2S_AUDIO_BUFFER_SIZE];

    err = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (err  != ESP_OK) {
        ESP_LOGE(TAG_FFT, "Not possible to initialize FFT. Error = %i", err);
        return;
    }
    // Generate Hann window
    dsps_wind_hann_f32(wind, I2S_AUDIO_BUFFER_SIZE);

    audio_data_t audio;
    while(1) {
        if (xQueueReceive(xQueueAudioData, (void*)&audio, portMAX_DELAY) == pdPASS){
            for (int i=0 ; i<I2S_AUDIO_BUFFER_SIZE ; i++)
            {
                y_cf[i*2 + 0] = (float)audio.data[i] * wind[i];
                y_cf[i*2 + 1] = 0;
            }
            dsps_fft2r_fc32(y_cf, I2S_AUDIO_BUFFER_SIZE);

            // Bit Reverse
            dsps_bit_rev_fc32(y_cf, I2S_AUDIO_BUFFER_SIZE);

            // Convert one complex vector to two complex vectors
            dsps_cplx2reC_fc32(y_cf, I2S_AUDIO_BUFFER_SIZE);

            // y1_cf - is your result in log scale
            // y2_cf - magnitude of your signal in linear scale

            for (int i = 0 ; i < I2S_AUDIO_BUFFER_SIZE/2 ; i++) {
                y1_cf[i] = 10 * log10f((y1_cf[i * 2 + 0] * y1_cf[i * 2 + 0] + y1_cf[i * 2 + 1] * y1_cf[i * 2 + 1])/I2S_AUDIO_BUFFER_SIZE);
                // y2_cf[i] = ((y1_cf[i * 2 + 0] * y1_cf[i * 2 + 0] + y1_cf[i * 2 + 1] * y1_cf[i * 2 + 1])/I2S_AUDIO_BUFFER_SIZE);

                // ESP_LOGW(TAG_FFT, "Signal %d in log scale: %.2f", i, y1_cf[i]);
                // ESP_LOGW(TAG_FFT, "Signal %d in absolute scale: %.2f", i, y2_cf[i]);
            }

            // Calculate main freq
            float max = 0;
            int max_index = 0;
            for (int i = 1 ; i < I2S_AUDIO_BUFFER_SIZE/2 ; i++) { //ignore dc
                if(max < y1_cf[i]) {
                    max = y1_cf[i];
                    max_index = i;
                }
            }
            float main_freq = ((float)I2S_SAMPLE_RATE / (float)I2S_AUDIO_BUFFER_SIZE) * max_index;
            // ESP_LOGI(TAG_FFT, "MAIN_FREQ: %2.2f - %2.2f | 31.5Hz: %2.2f | 63Hz: %2.2f | 94,5Hz: %2.2f | 126Hz: %2.2f | 257,5Hz: %2.2f ", main_freq, max, y1_cf[1], y1_cf[2], y1_cf[3], y1_cf[4], y1_cf[5]  );

            // Send to App
            app_data_t fft_data = {
                .id = DATA_ID_FFT,
                .data = y1_cf[1],
            };
            if (xQueueSend( xQueueAppData, (void *)&fft_data, portMAX_DELAY ) == pdFAIL) {
                ESP_LOGE(TAG_FFT, "ERROR sendig FFT data to APP queue");
            }
            // Send to FFT Stream
            if(xSemaphoreTake(xMicFFTStreamEnableMutex, portMAX_DELAY) == pdTRUE) {
                if(mic_fft_stream) {
                    freq_t freq_info[5] = {{.name="31.5", .value=y1_cf[1]},
                                            {.name="63", .value=y1_cf[2]},
                                            {.name="94,5", .value=y1_cf[3]},
                                            {.name="126", .value=y1_cf[4]},
                                            {.name="257,5", .value=y1_cf[5]}};
                    uart_fft_data_t stream_data = {
                        .freq = freq_info,
                        .len = 5
                    };

                    if (xQueueSend( xQueueUartStreamFFTBuffer, (void *)&stream_data, portMAX_DELAY ) == pdFAIL) {
                        ESP_LOGE(TAG_FFT, "ERROR sendig FFT data to queue");
                    }
                }
            }
            xSemaphoreGive(xMicFFTStreamEnableMutex);


            // ESP_LOGW(TAG_FFT, "Signal x1 in log scale");
            // dsps_view(y1_cf, I2S_AUDIO_BUFFER_SIZE/2, 64, 30,  -60, 120, '|');
            // ESP_LOGW(TAG_FFT, "Signal x1 in absolute scale");
            // dsps_view(y2_cf, I2S_AUDIO_BUFFER_SIZE/2,64, 10,  -60, 80, '|');
        }
    }
}

//******************************************************************************************************************

static bool mic_data_stream = false;
static const char *TAG = "MIC_TASK";

void mic_enable_data_stream() {
    if(xSemaphoreTake(xMicDataStreamEnableMutex, portMAX_DELAY) == pdTRUE) {
        mic_data_stream = true;
        ESP_LOGI(TAG, "Enable data stream");
        xSemaphoreGive(xMicDataStreamEnableMutex);
    }
}

void mic_disable_data_stream() {
    if(xSemaphoreTake(xMicDataStreamEnableMutex, portMAX_DELAY) == pdTRUE) {
        mic_data_stream = false;
        ESP_LOGI(TAG, "Disable data stream");
        xSemaphoreGive(xMicDataStreamEnableMutex);
    }
}

void vMic( void *pvParameters ) {
    if(mic_init() != ESP_OK) {
        ESP_LOGE(TAG, "ERROR Initializing Mic Driver");
        return;
    }

    xMicDataStreamEnableMutex = xSemaphoreCreateMutex();
    if(xMicDataStreamEnableMutex == NULL) {
        ESP_LOGE(TAG, "ERROR Creating Mutex");
        return;
    }
    xMicFFTStreamEnableMutex = xSemaphoreCreateMutex();
    if(xMicFFTStreamEnableMutex == NULL) {
        ESP_LOGE(TAG, "ERROR Creating Mutex");
        return;
    }

    // Create FFT task
    xQueueAudioData = xQueueCreate(2, sizeof(audio_data_t));

    xTaskCreatePinnedToCore(vTaskFFT,
                            "vTaskFFT",
                            2048*4,
                            NULL,
                            configMAX_PRIORITIES - 3,
                            &xTaskFFTHandle,
                            APP_CPU_NUM
                            );


    // FFT Stream Var
    audio_data_t audio_data = {
        .data = NULL
    };

    // UART Stream Var
    uint8_t sample_buffer[I2S_READ_BUFFER_SIZE] = {0};
    uart_mic_data_t stream_data = {
        .data = NULL,
        .len = 0,
        .id = UART_DATA_ID_MIC
    };
    i2s_event_t evt;
    size_t bytes_read;

    // Signalize task successfully creation
	xEventGroupSetBits(xEventGroupTasks, BIT_TASK_MIC);
    ESP_LOGI(TAG, "Mic Initialized");

    while(1) {
        // Wait for I2S event
        if (xQueueReceive(xQueueI2S, &evt, portMAX_DELAY) == pdPASS)
        {
            if (evt.type == I2S_EVENT_RX_DONE) {
                do {
                    ESP_ERROR_CHECK(mic_read_buffer(sample_buffer, &bytes_read));
                    // Proccess data
                    int32_t *samples_32 = (int32_t *)sample_buffer;

                    for (int i = 0; i < bytes_read/4; i++) {
                        // you may need to vary the >> 11 to fit your volume - ideally we'd have some kind of AGC here
                        samples_32[i] = samples_32[i]>>5;
                    }

                    // Send to FFT
                    audio_data.data = samples_32;
                    stream_data.len = bytes_read/4;

                    if (xQueueSend( xQueueAudioData, (void *)&audio_data, portMAX_DELAY ) == pdFAIL) {
                        ESP_LOGE(TAG, "ERROR sending audio data to FFT");
                    }

                    // Send to Data Stream
                    if(xSemaphoreTake(xMicDataStreamEnableMutex, portMAX_DELAY) == pdTRUE) {
                        if(mic_data_stream) {
                            stream_data.data = samples_32;
                            stream_data.len = bytes_read/4;
                            if (xQueueSend( xQueueUartStreamMicBuffer, (void *)&stream_data, portMAX_DELAY ) == pdFAIL) {
                                ESP_LOGE(TAG, "ERROR sendig mic data to queue");
                            }
                        }
                    }
                    xSemaphoreGive(xMicDataStreamEnableMutex);


                }while(bytes_read > 0);
            }
        }

    }

}

//******************************************************************************************************************
