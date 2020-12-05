/* 
   mic.c: I2S Microphone handling, send sample data asynchronously via websocket

   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "driver/i2s.h"

#include "demo_signal.h"
#include "gpio.h"
#include "config.h"

static const char *TAG = "send_task";

int sendTaskRunning=0;
QueueHandle_t sendQueue;
uint8_t buf[WS_SENDBUF_SIZE];

struct async_resp_arg asyncResponseConnection;

/*
 * Initialisation of the demo singal send queue
 */
esp_err_t init_demoSignal()
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

    int64_t timerTicks =  esp_timer_get_time();
    while (sendTaskRunning) {
    
        vTaskDelay(1);
        if (config.signal_mode==1)
            actSampleValue = 128 + (int) (sin((float)i/100.0f*2*M_PI*config.frequency) * config.gain);
        else actSampleValue = (int) ((float)((i*config.frequency)%100)/100.0f*config.gain);
        i=(i+1)%100;

        xQueueSendToBack(sendQueue,&actSampleValue,0);                    
        if (++sendCount>=SEND_INTERVAL) {
            sendCount=0;
            httpd_queue_work(asyncResponseConnection.h, ws_send_data_async, &asyncResponseConnection);
        }

        /* print performance data every second */
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
