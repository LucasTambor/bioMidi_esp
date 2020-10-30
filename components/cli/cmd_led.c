/* cmd_i2ctools.c

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "cmd_led.h"
#include <stdio.h>
#include <string.h>
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "esp_log.h"
#include "led_app.h"

static const char *TAG = "cmd_i2ctools";

static struct {
    struct arg_int *color;
    struct arg_int *freq;
    struct arg_end *end;
} led_args;

static int cmd_led(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&led_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, led_args.end, argv[0]);
        return 0;
    }

    led_color_e color = (led_color_e) led_args.color->ival[0];
    uint8_t freq = led_args.freq->ival[0];

    portBASE_TYPE xStatus;
    LED_Status led;
    memset(&led, 0, sizeof(LED_Status));

    led.color = color;
    led.freq = freq;

    xStatus = xQueueSend( xQueueLedBuffer, (void *)&led, portMAX_DELAY );
    if (xStatus == pdFAIL)
	{
		return 1;
	}

    return 0;
}

static void register_led(void)
{
    led_args.color = arg_int1("c", "color", "<color>", "0-RED | 1-GREEN | 2-BLUE | 3-RAINBOW");
    led_args.freq = arg_int0("f", "freq", "<freq>", "Specify the frequency");
    led_args.end = arg_end(1);
    const esp_console_cmd_t led_cmd = {
        .command = "led",
        .help = "Set led color and frequency",
        .hint = NULL,
        .func = &cmd_led,
        .argtable = &led_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&led_cmd));
}

void register_leds(void)
{
    register_led();
}
