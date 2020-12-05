/* 
   config.c: handling of configuration parameters and communication with WebGUI
             This c-file provides data structures and functions for
             loading and storing configuration parameters (which can be modified in the WebGUI)
             to a local c-structe as well as to the non-volatile SPI file system.
             Furthermore, function calls can be triggered by GUI actions (e.g. button clicked)
             Configuration parameters and callback functions can be registered dynamically.

             In order to perform the matching with the GUI elements (in html/javascript)
             have a look at the Webpage files index.html and const.js


   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_http_server.h"

#include "demo_signal.h"
#include "config.h"


static const char *TAG = "config";

struct config_struct config;  // the C-structure which holds current configuration values, see config.h


/*
 * This function registers all supported commands / parameters which are exchanges between frontend (WebGui) and backend.
 * A parameter is represented by a GUI element (slider, edit box, selection box etc.) in the WebGui and has a corresponding 
 * variable in the config struct. Currently data types integer and string are supported. Furthermore, a callback function
 * can be defined, which is called e.g. when a selection box changes or when a button is pressed in the GUI.
 * 
 * Please use the similar / intuitive names for the parameter in the WebGui (see const.js / index.html) and here.
 */
esp_err_t init_config(void)
{

 //                          parameter name,     data type,       default value,   pointer to variable,    pointer to callback function

    register_config_parameter ("signal_mode",   CONFIG_TYPE_INT,    "1",            &config.signal_mode,    NULL);
    register_config_parameter ("gain",          CONFIG_TYPE_INT,    "70",           &config.gain,           NULL);
    register_config_parameter ("frequency",     CONFIG_TYPE_INT,    "2",            &config.frequency,      NULL);
    register_config_parameter ("wifi_mode",     CONFIG_TYPE_INT,    "1",            &config.wifi_mode,      NULL);
    register_config_parameter ("mqtt_broker",   CONFIG_TYPE_STRING, "testBroker",   &config.mqttBroker,     NULL);
    register_config_parameter ("mqtt_delimiter",CONFIG_TYPE_STRING, ";",            &config.mqttDelimiter,  NULL);
    register_config_parameter ("wifi_name",     CONFIG_TYPE_STRING, "myWifi",       &config.wifiName,       NULL);
    register_config_parameter ("wifi_password", CONFIG_TYPE_STRING, "myPassword",   &config.wifiPassword,   NULL);
    register_config_parameter ("AT",            CONFIG_TYPE_NONE,   NULL,           NULL,                   callback_at);
    register_config_parameter ("start",         CONFIG_TYPE_NONE,   NULL,           NULL,                   callback_start);
    register_config_parameter ("stop",          CONFIG_TYPE_NONE,   NULL,           NULL,                   callback_stop);
    register_config_parameter ("load",          CONFIG_TYPE_NONE,   NULL,           NULL,                   callback_load);
    register_config_parameter ("save",          CONFIG_TYPE_NONE,   NULL,           NULL,                   callback_save);
    return (ESP_OK);
}


// dynamic list for parameter registration (pointer to first element)
struct parameter_struct * config_parameters=NULL;

// global variables to handle websocket communication (replies to WebGUI)
char * replyBuf=NULL;
httpd_req_t *actReq=NULL;


/*
 * Here come the callback functions (see registered parameters / commands above) 
 * Return value: 1 if the content of replyBuf should be sent back to the WebGUI
 *               0 if no reply to WebGUI
 */


// this replies the heartbeat message "OK" to the WebGui
int callback_at() {
    strcpy(replyBuf,"OK");
    return(1);
}

// this starts the send task (processing and sending of live data)
int callback_start() {
    if (!sendTaskRunning) {
        if (xTaskCreate(sendTask,"SEND_TASK",4096,actReq,10,NULL) == pdPASS) {
            ESP_LOGI(TAG,"Start reporting values! Created send task!"); 
        } else {
            ESP_LOGI(TAG,"Send task could not be created"); 
        }
    }
    return(0);
}

// this stops the send task
int callback_stop() {
    if (sendTaskRunning) {
        sendTaskRunning = 0;
        ESP_LOGI(TAG,"End reporting values! Stopping send task!"); 
    }
    return(0);
}

// this loads the configuration from SPI file systems and sends configuration strings to the WebGui
int callback_load() {
    load_config(replyBuf);
    return(1);    
}

// this save the configuration and feature values to the SPI file systems
int callback_save() {
    save_config();
    return(0);
}


/*
 * Registers one commands / parameter.
 * command name, type indicator, default value, pointer to c-varibale and callback function
 * are stored in a dynamic list.
 * 
*/
esp_err_t register_config_parameter (char* cmd, int type, char* def, void * ref, int (*fp)()) {
    struct parameter_struct ** p = &config_parameters;
    while (*p!=NULL) p= &(*p)->next;
    *p = calloc(1, sizeof(struct parameter_struct));
    if (!p) {
        ESP_LOGE(TAG, "Failed to allocate memory");
        return ESP_ERR_NO_MEM;
    }
    strcpy((*p)->cmd,cmd);
    (*p)->type=type;
    (*p)->fp=fp;

    if (type == CONFIG_TYPE_INT) {
        (*p)->ref=(int *) ref;
        *((int*) (*p)->ref)=atoi(def);
    } else if (type == CONFIG_TYPE_STRING) {
        (*p)->ref=(char *) ref;
        strcpy ((*p)->ref, def);
    } else (*p)->ref=NULL;

    (*p)->next=NULL;
    return(ESP_OK);
}

/*
 * Loads all parameters from the CONFIG_FILE (ASCII file, SPI file system)
 * Parameter name and parameter value are separated by ' ', then a '\n' is added.
 * 
*/
esp_err_t load_config(char * cmdString) {
    FILE *fd = NULL;
    struct stat file_stat;
    char line[MAX_CONFIGPARAM_LEN+10];

    if (cmdString!=NULL) cmdString[0]=0;  // init return string
    
    ESP_LOGI(TAG, "load configuration from SPIFFS");
    if (stat(CONFIG_FILE, &file_stat) == -1) {
        ESP_LOGE(TAG, "Failed to open config file: %s\n..create file with default values!", CONFIG_FILE);
        save_config();
        return(ESP_OK);
    } 

    fd = fopen(CONFIG_FILE, "r");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to read config file");
        return (ESP_FAIL);
    }

    while (fgets(line, sizeof(line), fd)) {
        if (cmdString!=NULL)
            strcat(cmdString,line);  // add the config command to the return string (for WebGui)
        // strip newline
        char* pos = strchr(line, '\n');
        if (pos) {
            *pos = '\0';
        }
        // ESP_LOGI(TAG, "Read from file: '%s'", line);
        parseConfigValue(line);
    }
    fclose(fd);

    ESP_LOGI(TAG, "Config read successfully.");
    return(ESP_OK);
}

/*
 * Saves all parameters to the CONFIG_FILE (ASCII file, SPI file system)
 * Parameter name and parameter value are separated by ' ', then a '\n' is added.
 * 
*/
esp_err_t save_config() {
    FILE *fd = NULL;
    
    ESP_LOGI(TAG, "save configuration to SPIFFS");
    fd = fopen(CONFIG_FILE, "w");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to create config file");
        return (0);
    }

   struct parameter_struct * p = config_parameters;
    while (p!=NULL) {
        if ( p->type == CONFIG_TYPE_INT) {
            fprintf(fd,"%s %d\n",p->cmd,*((int *)p->ref));
        } else if ( p->type == CONFIG_TYPE_STRING) {
            fprintf(fd,"%s %s\n",p->cmd,(char *)p->ref);
        }
        p=p->next;
    }

    fclose(fd);
    ESP_LOGI(TAG, "Config file written successfully.");
    return(ESP_OK);
}

/*
 * get an integer parameter value for a given command name 
 * line: input string e.g. "gain 10"
 * cmdString: the command name, e.g "gain"
 * value: pointer to integer, where result should be stored, e.g. 10
 * returns 1 if command name was found, else 0
*/
int parseIntValue(char * line, char * cmdString, int * value) {
    char * s;
    if ((s=strstr(line,cmdString))) { 
        if (strlen(s) > strlen(cmdString)+1) {
            *value = atoi(s+strlen(cmdString)+1);
            return(1);
        }
    }
    return(0);
}

/*
 * get a string parameter value for a given command name 
 * line: input string e.g. "name john"
 * cmdString: the command name, e.g "name"
 * value: string where result should be stored, e.g. "john"
 * returns 1 if command name was found, else 0
*/
int parseStringValue(char * line, char * cmdString, char * value) {
    char * s;
    if ((s=strstr(line,cmdString))) { 
        if (strlen(s) > strlen(cmdString)+1) {
            strcpy(value, s+strlen(cmdString)+1);
            return(1);
        }
    }
    return(0);
}

/*
 * Parse a configuration command (e.g. a line of the config file)
 * searches the list of all registered commands
 * if the current line (which is supposed to start with a command string)
 * starts with a known command string: get the parameter value and store into the dedicated variable
*/
void parseConfigValue(char * line) {

   struct parameter_struct * p = config_parameters;
    while (p!=NULL) {
        if (strstr(line,p->cmd)==line) {
            if ( p->type == CONFIG_TYPE_INT) {
                if (parseIntValue(line, p->cmd, (int *) p->ref)) {
                    ESP_LOGI(TAG, "config %s set to: %d", p->cmd, *((int *) p->ref)); 
                } 
            } else if ( p->type == CONFIG_TYPE_STRING) {
                if (parseStringValue(line, p->cmd, (char *) p->ref)) {
                    ESP_LOGI(TAG, "config %s set to: %s", p->cmd, (char *) p->ref); 
                } 
            }
            return;
        } 
        p=p->next;
    }
} 

/*
 * Parse commands from Webgui!
 * checks parameter values and function callbacks.
 * returns 1 (via callback function) if reply buffer should be sent back to the WebGui!
 */
int parseCommandFromWebGUI(char * cmdString, httpd_req_t *req) {

    // save request/reply buffer and request handle adresses in global varibales
    // (in case callback functions need to modify the reply - could be improved)
    replyBuf=cmdString;
    actReq=req;

    parseConfigValue(cmdString);

    struct parameter_struct * p = config_parameters;
    while (p!=NULL) {
        if (strstr(cmdString,p->cmd)==cmdString) {
            if ( p->fp ) {
                ESP_LOGI(TAG, "callback function for cmd %s found!",cmdString);
                return( p->fp() );
            }
        } 
        p=p->next;
    }
    return(0);
}