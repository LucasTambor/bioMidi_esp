#ifndef _TOUCH_DRIVER_H_
#define _TOUCH_DRIVER_H_

#include <stdio.h>
#include "driver/touch_pad.h"

esp_err_t touch_button_init();

esp_err_t touch_button_read(bool * state);

#endif //_TOUCH_DRIVER_H_