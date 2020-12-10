/* 
   gpio.c: gpio initialisation, functions for LEDs and buttons

   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include "gpio.h"
#include "driver/gpio.h"


esp_err_t init_gpio() {
    gpio_pad_select_gpio(GPIO_LED_OK);
    gpio_set_direction(GPIO_LED_OK, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(GPIO_LED_REC);
    gpio_set_direction(GPIO_LED_REC, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_BUTTON_REC, GPIO_MODE_INPUT);
    return (ESP_OK);
}


esp_err_t gpio_set_led(uint8_t ledPin, uint8_t level){
    return gpio_set_level(ledPin, level);
}

uint8_t gpio_get_button(uint8_t buttonPin) {
    return gpio_get_level(buttonPin);
}