#ifndef _INC_GPIO_H
#define _INC_GPIO_H

#include "esp_err.h"

#define GPIO_LED_OK 25
#define GPIO_LED_REC 26
#define GPIO_BUTTON_REC 0

esp_err_t init_gpio();
esp_err_t gpio_set_led(uint8_t ledPin, uint8_t level);
uint8_t gpio_get_button(uint8_t buttonPin);


#endif