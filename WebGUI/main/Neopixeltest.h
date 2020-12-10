#ifndef Neopixeldemo_H
#define Neopixeldemo_H
#define LENGTH_OF_LED_STRIP 39
extern struct led_strip_t led_strip;
#include "led_strip.h"
void set_led_strip_color(uint8_t r,uint8_t g, uint8_t b,uint8_t area);
uint8_t led_init(void);
#endif