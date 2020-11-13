#include "touch_driver.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TOUCH_THRESH_NO_USE 0
#define TOUCHPAD_FILTER_TOUCH_PERIOD 10
#define TOUCH_PRESSED_THR   200

esp_err_t touch_button_init() {
    ESP_ERROR_CHECK(touch_pad_init());
    ESP_ERROR_CHECK(touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V));
    ESP_ERROR_CHECK(touch_pad_config(TOUCH_PAD_NUM7, TOUCH_THRESH_NO_USE));
    ESP_ERROR_CHECK(touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD));
    return ESP_OK;
}

esp_err_t touch_button_read(bool * state) {
    if(state == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t touch_filtered;
    ESP_ERROR_CHECK(touch_pad_read_filtered(TOUCH_PAD_NUM7, &touch_filtered));
    *state = touch_filtered < TOUCH_PRESSED_THR;
    return ESP_OK;
}

