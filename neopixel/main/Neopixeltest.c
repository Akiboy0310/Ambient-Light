#include "led_strip.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include <stdlib.h>

#define LENGTH_OF_LED_STRIP 39 
// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
/** @brief Double buffering for neopixels - buffer 1 */
struct led_color_t *neop_buf1 = NULL;
/** @brief Double buffering for neopixels - buffer 2 */
struct led_color_t *neop_buf2 = NULL;
struct led_strip_t led_strip = {
      .rgb_led_type = RGB_LED_TYPE_WS2812,
      .rmt_channel = RMT_CHANNEL_7,
      .gpio = GPIO_NUM_32,
      .led_strip_length = LENGTH_OF_LED_STRIP,
  };
uint8_t led_init(void){
    neop_buf1 = malloc(sizeof(struct led_color_t)*LENGTH_OF_LED_STRIP);
    neop_buf2 = malloc(sizeof(struct led_color_t)*LENGTH_OF_LED_STRIP);
  if(neop_buf1 == NULL || neop_buf2 == NULL)
  {
    printf("Not enough memory to initialize Neopixel buffer");
    return ESP_FAIL;
  }
  
  //init remaining stuff of led strip driver struct
  led_strip.access_semaphore = xSemaphoreCreateBinary();
  led_strip.led_strip_buf_1 = neop_buf1;
  led_strip.led_strip_buf_2 = neop_buf2;

  //initialize module
  if(led_strip_init(&led_strip) == false)
  {
    printf("Error initializing led strip (Neopixels)!");
    return ESP_FAIL;
  }
    return 0; 
}

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

void app_main(void){ 
    /*++++ init Neopixel driver ++++*/
    led_init();
    while(1){
        set_led_strip_color(145,185,4,1);
        set_led_strip_color(145,15,54,2);
        set_led_strip_color(45,185,54,3);
        set_led_strip_color(105,235,94,4);

        //show new color
        led_strip_show(&led_strip);
        vTaskDelay(1000/portTICK_PERIOD_MS); //delay 1000ms
    }
}
