#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "driver/i2s.h"

#include "gpio.h"
#include "config.h"
#include "neopixel.h"
#include "rgb_sensor.h"
static const char *TAG = "send_task";
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
void set_led_strip_color(uint8_t r,uint8_t g, uint8_t b,uint8_t area,float brightness){
    /**
    * Funktion to set the color of the ledstrip. If area = 1 the first quarter will be set,
    * if area = 2 the second quarter etc.*/ 
    uint8_t startLED = (area-1) * (LENGTH_OF_LED_STRIP / 4);
    uint8_t endLED = area * (LENGTH_OF_LED_STRIP / 4);

    //ESP_LOGI("NP","Start: %d, %d", startLED,endLED);

    for(uint8_t i = startLED; i<=endLED; i++)
    {
        led_strip_set_pixel_rgb(&led_strip, i,r*brightness,g*brightness,b*brightness);
    }
}
int sendTaskRunning=0;
QueueHandle_t sendQueue;
uint8_t buf[WS_SENDBUF_SIZE];

struct async_resp_arg asyncResponseConnection;

/*
 * Initialisation of the demo singal send queue
 */
esp_err_t init_neopixel()
{
    ESP_LOGI(TAG, "Initializing demo signal send queue");
    sendQueue = xQueueCreate(SEND_QUEUE_SIZE,sizeof(int));
    return ESP_OK;
}

/*
 * async send function for data to be sent to WebGUI via websocket.
 * this is called by the httpd work queue!
 */
static void ws_send_data_async(void *arg)
{
    int actSample;
    struct async_resp_arg *resp_arg = arg;
    httpd_handle_t hd = resp_arg->hd;
    int fd = resp_arg->fd;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

    int messages = uxQueueMessagesWaiting(sendQueue);
    //    ESP_LOGI(TAG, "messages in queue: %d", messages);
    if (messages>=SEND_QUEUE_SIZE) {  // queue full... discard!
        ESP_LOGI(TAG, "*** emptying queue! ***");
        xQueueReset(sendQueue);
        return;
    }

    if (messages==0)  {
        // ESP_LOGI(TAG, "no message in queue!");
        return;
    }

    strcpy((char *) buf,"VALUES:");  // this indicates to the WebGUI that livevalues are being sent

    while (messages > 0) {
        if( xQueueReceive( sendQueue,&( actSample ),( TickType_t ) 0 ) == pdPASS ) {
            if (actSample == 0x0FFFFFFF) {
                strcat((char *)buf,"*,");     // mark first value if relevant (eg. for fft spectrum) TBD:improve!
            } else {
                char tmp[20];
                sprintf(tmp,"%d,",actSample);
                strcat((char *)buf,(char*)tmp);
            }
        }
        messages--;
    }
    ws_pkt.payload = buf;
    ws_pkt.len=strlen((char*)buf);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    ws_pkt.final=true;

    // ESP_LOGI(TAG, "async sent %d bytes",strlen((char*) buf));
    // ESP_LOGI(TAG, "async send: %s", (char *) buf);
    httpd_ws_send_frame_async(hd, fd, &ws_pkt);

}

/*
 * send task: this task generates a signal and sends it to the WebGui via a queue and websocket
 */
void sendTask(void *param)
{
    int sendCount=0,i=0;
    int actSampleValue=0;

    httpd_req_t * req = (httpd_req_t *)param;    // remeber the request handle to the websocket

    asyncResponseConnection.fd=httpd_req_to_sockfd(req);
    asyncResponseConnection.hd=req->handle;
    asyncResponseConnection.h =req->handle;

    ESP_LOGI(TAG,"send task running!"); 
    gpio_set_led(GPIO_LED_OK,1);


    sendTaskRunning=1;
    float r1, r2, r3,r4;
    float g1, g2,g3,g4;
    float b1, b2,b3,b4;
    uint8_t value;
    int64_t timerTicks =  esp_timer_get_time();
    while (sendTaskRunning) {
        float brightness = (float)config.frequency/100; // variable to set the brightness 
        vTaskDelay(1);
        if (config.onof==1 && config.colorsensing==1)
        {
            value=(Channel0);
            i2c_TCA9548_init(&(value));
            tcs34725(&r1, &g1, &b1);
            value=(Channel1);
            i2c_TCA9548_init(&(value));
            tcs34725(&r2, &g2, &b2);
            value=(Channel2);
            i2c_TCA9548_init(&(value));
            tcs34725(&r3, &g3, &b3);
            value=(Channel3);
            i2c_TCA9548_init(&(value));
            tcs34725(&r4, &g4, &b4);            
            set_led_strip_color(r1,g1,b1,1,brightness);
            set_led_strip_color(r2,g2,b2,2,brightness);
            set_led_strip_color(r3,g3,b3,3,brightness);
            set_led_strip_color(r4,g4,b4,4,brightness);
        }
        
        else if(config.onof==1||config.area1onof==1 ||config.area2onof==1 ||config.area3onof==1 || config.area4onof==1){
            set_led_strip_color(config.red,config.green,config.blue,1,brightness);
            set_led_strip_color(config.red,config.green,config.blue,2,brightness);
            set_led_strip_color(config.red,config.green,config.blue,3,brightness);
            set_led_strip_color(config.red,config.green,config.blue,4,brightness);
        }
        else {
            set_led_strip_color(0,0,0,1,0);
            set_led_strip_color(0,0,0,2,0);
            set_led_strip_color(0,0,0,3,0);
            set_led_strip_color(0,0,0,4,0);
        }
        //show new color
        led_strip_show(&led_strip);

        xQueueSendToBack(sendQueue,&actSampleValue,0);                    
        if (++sendCount>=SEND_INTERVAL) {
            sendCount=0;
            httpd_queue_work(asyncResponseConnection.h, ws_send_data_async, &asyncResponseConnection);
        }

        /*print performance data every second*/
        int timePassed =  esp_timer_get_time()-timerTicks;
        if (timePassed >= 1000000) {
            //heap_caps_print_heap_info(MALLOC_CAP_8BIT);
            ESP_LOGI(TAG,"free heap: %d", xPortGetFreeHeapSize());
            timerTicks+=timePassed;
        }
    }
    gpio_set_led(GPIO_LED_OK,0);
    ESP_LOGI(TAG,"quit send task!"); 
    vTaskDelete(NULL);
}


