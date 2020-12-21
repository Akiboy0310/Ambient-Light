#ifndef _INC_MIC_H
#define _INC_MIC_H

#include "esp_err.h"
#include "esp_http_server.h"
#include "led_strip.h"

#define SEND_QUEUE_SIZE 250
#define WS_SENDBUF_SIZE 1000

#define SEND_INTERVAL 10

#define LENGTH_OF_LED_STRIP 39

void set_led_strip_color(uint8_t r,uint8_t g, uint8_t b,uint8_t area,float bridness);
uint8_t led_init(void);
extern int sendTaskRunning;
extern struct led_strip_t led_strip;

esp_err_t init_neopixel();
void sendTask(void *param);

/*
 * Structure holding server handle and internal socket fd in order
 * to use out of request send
 */   
struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
    httpd_handle_t h;
};
#endif
