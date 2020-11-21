#include "led_strip.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
void app_main(void){ 
    struct led_strip_t led_strip_t;
    led_strip_t.gpio=GPIO_NUM_32;
    led_strip_t.rmt_channel= RMT_CHANNEL_2;
    led_strip_init(&led_strip_t);
    while(1){
        led_strip_set_pixel_rgb(&led_strip_t,39, 255, 250,0);
        vTaskDelay(1000/portTICK_PERIOD_MS); //delay 1000ms
    }
}
