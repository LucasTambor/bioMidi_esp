/**
This file is part of ESP32RMT esp-idf component
(https://github.com/CalinRadoni/ESP32RMT)
Copyright (C) 2019+ by Calin Radoni

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ESP32RMTChannel.h"

#include <new>
#include "esp_log.h"
#include "driver/rmt.h"
#include "soc/rmt_struct.h"
#include "driver/gpio.h"

// -----------------------------------------------------------------------------

static const char *TAG  = "ESP32RMT";

/*
 * Standard APB clock (as needed by WiFi and BT to work) is 80 MHz.
 * This gives a 12.5 ns * rmt_config_t::clk_div for computing RMT durations, like this:
 *         duration = required_duration_in_ns / (12.5 * rmt_config_t::clk_div)
 */
const uint8_t  rmt_clk_divider  = 4;
const uint16_t rmt_clk_duration = 50; // ns,  12.5 * 4( = config.clk_div in rmt_config_for_digital_led_strip)

// -----------------------------------------------------------------------------

ESP32RMTChannel::ESP32RMTChannel(void)
{
    initialized = false;
    data    = nullptr;
    length  = 0;
    channel = RMT_CHANNEL_MAX;
    pin     = GPIO_NUM_MAX;
    driverInstalled = false;
}

ESP32RMTChannel::~ESP32RMTChannel(void)
{
    Cleanup();
}

bool ESP32RMTChannel::Initialize(rmt_channel_t rmt_channel, gpio_num_t gpio_pin, uint32_t numberOfItems)
{
    if (initialized) {
        ESP_LOGE(TAG, "Initialize - Allready initialized");
        return false;
    }

    if ((rmt_channel < RMT_CHANNEL_0) || (rmt_channel >= RMT_CHANNEL_MAX)) {
        ESP_LOGE(TAG, "Initialize - Wrong rmt_channel");
        return false;
    }

    if ((gpio_pin < GPIO_NUM_0) || (gpio_pin >= GPIO_NUM_MAX)) {
        ESP_LOGE(TAG, "Initialize - Wrong gpio_pin");
        return false;
    }

    if (!CreateBuffer(numberOfItems)) {
        return false;
    }

    channel = rmt_channel;
    pin     = gpio_pin;

    initialized = true;

    return true;
}

void ESP32RMTChannel::Cleanup(void)
{
    UninstallDriver();
    DestroyBuffer();

    channel = RMT_CHANNEL_MAX;
    pin     = GPIO_NUM_MAX;
    initialized = false;
}

bool ESP32RMTChannel::CreateBuffer(uint32_t numberOfItems)
{
    DestroyBuffer();

    if (numberOfItems == 0) return false;

    data = new (std::nothrow) rmt_item32_t[numberOfItems];
	if (data == nullptr) {
        ESP_LOGE(TAG, "CreateBuffer - Allocation failed");
		return false;
    }
    length = numberOfItems;
    return true;
}

void ESP32RMTChannel::DestroyBuffer(void)
{
	if (data != nullptr) {
		delete[] data;
		data = nullptr;
	}
	length = 0;
}

bool ESP32RMTChannel::SetGPIO_Out(uint32_t level)
{
    gpio_pad_select_gpio(pin);

    esp_err_t err = gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[0x%x] gpio_set_direction failed", err);
        return false;
    }

    err = gpio_set_level(pin, level);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[0x%x] gpio_set_level failed", err);
        return false;
    }

    return true;
}

bool ESP32RMTChannel::ConfigureForWS2812x()
{
    if (!initialized) return false;

	if (!SetGPIO_Out(0)) return false;

    rmt_config_t config;

    config.rmt_mode = rmt_mode_t::RMT_MODE_TX;
    config.channel  = channel;
    config.clk_div  = rmt_clk_divider;
    config.gpio_num = pin;

    /* One memory block is 64 words * 32 bits each; the type is rmt_item32_t (defined in rmt_struct.h).
    A channel can use more memory blocks by taking from the next channels, so channel 0 can have 8
    memory blocks and channel 7 just one. */
    config.mem_block_num = 1;

    config.tx_config.loop_en              = false;
    config.tx_config.carrier_en           = false;
    config.tx_config.carrier_freq_hz      = 0;
    config.tx_config.carrier_duty_percent = 0;
    config.tx_config.carrier_level        = rmt_carrier_level_t::RMT_CARRIER_LEVEL_LOW;
    config.tx_config.idle_level           = rmt_idle_level_t::RMT_IDLE_LEVEL_LOW;
    config.tx_config.idle_output_en       = true;

    esp_err_t err;

    if (driverInstalled) {
        // stop RX and TX on this rmt channel

        err = rmt_rx_stop(channel);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "[0x%x] rmt_rx_stop failed", err);
            return false;
        }
        err = rmt_tx_stop(channel);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "[0x%x] rmt_tx_stop failed", err);
            return false;
        }

        // disable rmt interrupts for this channel
        rmt_set_rx_intr_en(    channel, 0);
        rmt_set_err_intr_en(   channel, 0);
        rmt_set_tx_intr_en(    channel, 0);
        rmt_set_tx_thr_intr_en(channel, 0, 0xffff);

        // set rmt memory to normal (not power-down) mode
        err = rmt_set_mem_pd(channel, false);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "[0x%x] rmt_set_mem_pd failed", err);
            return false;
        }
    }

    /* The rmt_config function internally:
     * - enables the RMT module by calling periph_module_enable(PERIPH_RMT_MODULE);
     * - sets data mode with rmt_set_data_mode(RMT_DATA_MODE_MEM);
     * - associates the gpio pin with the rmt channel using rmt_set_pin(channel, mode, gpio_num);
     */
    err = rmt_config(&config);
    if (err != ESP_OK) {
    	ESP_LOGE(TAG, "[0x%x] rmt_config failed", err);
    	return false;
    }

    if (!InstallDriver()) return false;

    return true;
}

bool ESP32RMTChannel::InstallDriver(void)
{
    if (driverInstalled) return true;

    esp_err_t err = rmt_driver_install(channel, 0, 0);
    if (err != ESP_OK) {
    	ESP_LOGE(TAG, "[0x%x] rmt_driver_install failed", err);
    	return false;
    }

    driverInstalled = true;
    return true;
}

bool ESP32RMTChannel::UninstallDriver(void)
{
    if (!driverInstalled) return true;

    esp_err_t err = rmt_driver_uninstall(channel);
    if (err != ESP_OK) {
    	ESP_LOGE(TAG, "[0x%x] rmt_driver_uninstall failed", err);
    	return false;
    }

    driverInstalled = false;
    return true;
}

bool ESP32RMTChannel::SendData(void)
{
    if (!initialized) return false;

	esp_err_t err = rmt_write_items(channel, data, length, true);
    if (err != ESP_OK) {
    	ESP_LOGE(TAG, "[0x%x] rmt_write_items failed", err);
    	return false;
    }
    return true;
}

rmt_item32_t* ESP32RMTChannel::GetDataBuffer(void)
{
    return data;
}
uint32_t ESP32RMTChannel::GetDataBufferLen(void)
{
    return length;
}
void ESP32RMTChannel::SetDataBuffer(uint32_t index, rmt_item32_t value)
{
    if (data != nullptr)
        if (index < length)
            data[index] = value;
}
