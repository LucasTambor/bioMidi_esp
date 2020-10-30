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

#ifndef ESP32RMTChannel_H
#define ESP32RMTChannel_H

#include <stdint.h>
#include "driver/rmt.h"
#include "soc/rmt_struct.h"

class ESP32RMTChannel
{
public:
    ESP32RMTChannel(void);
    ~ESP32RMTChannel(void);

    /**
     * @brief Initialize the channel properties
     *
     * For WS2812x digital LEDs the length of data buffer should be computed as follow:
     * - every LED needs 24 bits = (8 * (strip's bytesPerLED)) bits
     * - every bit needs one item (a `rmt_item32_t`)
     * The length should be equal to (24 * number_of_LEDS).
     *
     * @param rmt_channel   Must be between RMT_CHANNEL_0 and RMT_CHANNEL_7;
     * @param gpio_pin      Must be between GPIO_NUM_0 and GPIO_NUM_39;
     * @param numberOfItems The length of data buffer.
     */
    bool Initialize(rmt_channel_t rmt_channel, gpio_num_t gpio_pin, uint32_t numberOfItems);

    /**
     * @brief Clear the channel properties
     */
    void Cleanup(void);

    /**
     * @brief Configure for WS2812x digital LEDS
     *
     * This function sets a RMT clock duration of 50 ns.
     */
    bool ConfigureForWS2812x(void);

    /**
     * @brief Sends the data
     *
     * This function will block the task and wait for sending done
     */
    bool SendData(void);

    /** Returns the data buffer */
    rmt_item32_t* GetDataBuffer(void);
    /** Returns the length of data buffer */
    uint32_t      GetDataBufferLen(void);

    /**
     * @brief Set an item of data buffer
     *
     * Safer but slower way to set the content of the data buffer.
     *
     * @param index The index of the item
     * @param value The item's new value
     */
    void SetDataBuffer(uint32_t index, rmt_item32_t value);

protected:
    bool initialized;
	rmt_item32_t  *data;     /*!< The buffer to be passed to the RMT driver for sending */
	uint32_t      length;    /*!< Length of data buffer */
    rmt_channel_t channel;
    gpio_num_t    pin;
    bool          driverInstalled;

	/**
	 * @brief Create the data buffer
     *
	 * @param numberOfItems The length of data buffer.
	 */
    bool CreateBuffer(uint32_t numberOfItems);

    /**
     * @brief Destroy the data buffer
     */
    void DestroyBuffer(void);

    /**
     * @brief Set GPIO as output
     *
     * @param level    Output level - use 0 for low and 1 for high.
     */
    bool SetGPIO_Out(uint32_t level);

    /**
     * @brief Install RMT driver for specified channel
     *
     * Create... functions are installing a RMT driver.
     * This is the function those should use.
     */
    bool InstallDriver(void);

    /**
     * @brief Uninstall RMT driver for specified channel
     *
     * Create... functions are installing a RMT driver.
     * If not used any more the driver cand be uninstalled to free the allocated resources.
     */
    bool UninstallDriver(void);
};

#endif
