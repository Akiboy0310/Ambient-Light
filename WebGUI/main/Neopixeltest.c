#include "led_strip.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/i2s.h"

#include "Neopixeltest.h"
#include "gpio.h"
#include "config.h"


void set_led_strip_color(uint8_t r,uint8_t g, uint8_t b,uint8_t area){
    /**
    * Funktion to set the color of the ledstrip. If area = 1 the first quarter will be set,
    * if area = 2 the second quarter etc.*/ 
    switch (area)
    {
    case 1:
        for(uint8_t i =0; i<LENGTH_OF_LED_STRIP*0.25; i++)
            {
                //set the same color for each LED
                led_strip_set_pixel_rgb(&led_strip, i,r,g,b);
            }
        break;
    case 2:
        for(uint8_t i =LENGTH_OF_LED_STRIP*0.25; i<LENGTH_OF_LED_STRIP*0.5; i++)
            {
                //set the same color for each LED
                led_strip_set_pixel_rgb(&led_strip, i,r,g,b);
            }
        break;
    case 3:
        for(uint8_t i =LENGTH_OF_LED_STRIP*0.5; i<LENGTH_OF_LED_STRIP*0.75; i++)
            {
                //set the same color for each LED
                led_strip_set_pixel_rgb(&led_strip, i,r,g,b);
            }
        break;
    case 4:
        for(uint8_t i =LENGTH_OF_LED_STRIP*0.75; i<LENGTH_OF_LED_STRIP; i++)
            {
                //set the same color for each LED
                led_strip_set_pixel_rgb(&led_strip, i,r,g,b);
            }
        break;
    default:
        printf("Please choose an area from 1 to 4");
        break;
    }
}
