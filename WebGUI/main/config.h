#ifndef _INC_CONFIG_H
#define _INC_CONFIG_H


#include "esp_http_server.h"

#define MAX_COMMANDBUF_SIZE 1024   // buffer must hold complete configuration command strings -> increase if necessary!
#define MAX_CONFIGPARAM_LEN 40

#define SPIFFS_BASE_PATH  "/spiffs"
#define CONFIG_FILE  "/spiffs/config.txt"

#define CONFIG_TYPE_NONE 0
#define CONFIG_TYPE_INT 1
#define CONFIG_TYPE_STRING 2

#define CONFIG_WIFI_MODE_AP  1
#define CONFIG_WIFI_MODE_STA 2

#define SEND_SINE 1
#define SEND_RECT 2


/* data structure holding current configuration parameters */
struct config_struct {
    int red;
    int green;
    int blue;
    int area1onof;
    int area2onof;
    int area3onof;
    int area4onof;
    int onof;
    int signal_mode;
    int bridness;
    int gain;
    int frequency;
    int wifi_mode;
    char mqttBroker[MAX_CONFIGPARAM_LEN];
    char mqttDelimiter[MAX_CONFIGPARAM_LEN];
    char wifiName[MAX_CONFIGPARAM_LEN];
    char wifiPassword[MAX_CONFIGPARAM_LEN];
};

extern struct config_struct config;    // defined in config.c

/* callback functions for supported commands, implemented in config.c  */
int callback_at();
int callback_frequency();
int callback_start();
int callback_stop();
int callback_load();
int callback_save();


/* config parameter structure for registering parameters, commands and callback functions */
struct parameter_struct {
    char cmd[MAX_CONFIGPARAM_LEN];
    int type;
    void * ref;
    int (*fp) ();
    struct parameter_struct * next;
};
esp_err_t register_config_parameter (char* cmd, int type, char* def, void * ref, int (*fp)());
esp_err_t init_spiffs(void);
esp_err_t init_config(void);

void parseConfigValue(char * line);
int parseCommandFromWebGUI(char * cmd, httpd_req_t *req);
esp_err_t save_config();
esp_err_t load_config(char * returnString);

#endif