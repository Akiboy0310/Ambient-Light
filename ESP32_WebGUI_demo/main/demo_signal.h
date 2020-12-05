
#ifndef _INC_MIC_H
#define _INC_MIC_H

#include "esp_err.h"
#include "esp_http_server.h"

#define SEND_QUEUE_SIZE 250
#define WS_SENDBUF_SIZE 1000

#define SEND_INTERVAL 10

extern int sendTaskRunning;

esp_err_t init_demoSignal();
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