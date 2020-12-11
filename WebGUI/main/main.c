/* WebGUI Example
    based upon Espressif IDF  HTTP File Server Example

   * a WebGUI is provided via Wifi Accesspoint (default) or Wifi Station
   * the default SSID/pwd can be found in config.c, the IP Adress is 192.168.4.1
   * a websocket communication is used to send live data (a signal waveform)

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <sys/param.h>

#include "esp_log.h"
#include "esp_event.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "captdns.h"
#include "demo_signal.h"
#include "Neopixeltest.h"
#include "led_strip.h"
#include "gpio.h"
#include "config.h"


static const char *TAG="main";

/* Function for SPI Filesystem initialisation, see spiffs. */
esp_err_t init_spiffs(void);

/* Functions for Wifi module initialisation, see wifi.c */
esp_err_t wifi_init_ap(char* ssid, char* password);
esp_err_t wifi_init_sta(char* ssid, char* password);

/* Function which starts the web server, see web_server.c */
esp_err_t start_web_server(const char *base_path);


void app_main(void)
{

    /* Print chip information at startup */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");


    /* Initialize NVS file system and partitions */
    ESP_ERROR_CHECK(nvs_flash_init());

    /* Initialize TCP/IP Stack */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Initialize FreeRTOS event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Initialize leds and buttons, see gpio.c */
    ESP_ERROR_CHECK(init_gpio());
    /* Initialize demo signal, see demo_signal.c */
    ESP_ERROR_CHECK(init_demoSignal());
    /*++++ init Neopixel driver ++++*/
    led_init();
    /* Initialize SPI filesystem, see spiffs.c  */
    ESP_ERROR_CHECK(init_spiffs());

    /* Initialize file storage and config data, see config.c  */
    ESP_ERROR_CHECK(init_config());

    /* Load configuration data from SPIFFS, see config.c */
    ESP_ERROR_CHECK(load_config(NULL)); 

    /* Initialise Wifi, according to stored communication mode */
    if (config.wifi_mode==CONFIG_WIFI_MODE_AP) {
        ESP_LOGI(TAG, "init Wifi in Access Point Mode.");
    	ESP_ERROR_CHECK(wifi_init_ap(config.wifiName,config.wifiPassword));	

        /* start DNS server for captive portal*/
        //captdnsInit();
    }
    else {
        ESP_LOGI(TAG, "init Wifi in Station Mode.");
    	ESP_ERROR_CHECK(wifi_init_sta(config.wifiName,config.wifiPassword));	
    }

    /* Start all file- and websocket services, see web_server.c */
    ESP_ERROR_CHECK(start_web_server(SPIFFS_BASE_PATH));
    //show new color
    //led_strip_show(&led_strip);
    //vTaskDelay(1000/portTICK_PERIOD_MS); //delay 1000ms
}
