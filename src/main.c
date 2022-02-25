//STANDARD IO Headers
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

//FREERTOS Headers
#include "freertos/FreeRTOSConfig.h"

#define INCLUDE_vTaskSuspend    1 

//Base Headers
#include "base/rm200x_mqtt.h"
#include "base/rm200x_wifi.h"
#include "base/rm200x_gpio.h"
#include "base/rm200x_uart.h"
#include "base/rm200x_fs.h"

#include "base/rm200x_socket.h"

//Application Headers
#include "App/rm200x_ack.h"
#include "App/rm200x_frame.h"

/******************************************************************************************/
// MAIN
/******************************************************************************************/

void app_main() //Called from an "internal" main.c 
{
    // Start up notes
    ESP_LOGI("APPLICATION", "[APP] Startup..");
    ESP_LOGI("APPLICATION", "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI("APPLICATION", "[APP] IDF version: %s", esp_get_idf_version());

    // Set basic logging level to warnings -> errors
    esp_log_level_set("*", ESP_LOG_ERROR);
    // SET SPECIFIC MODULE LOGGING LEVELS
    // WIFI Module
	esp_log_level_set(WIFI_TAG, ESP_LOG_ERROR);
    // MQTT Module
	esp_log_level_set(MQTT_RX_TASK_TAG, ESP_LOG_ERROR);
    esp_log_level_set(MQTT_TX_TASK_TAG, ESP_LOG_ERROR);
    // UART Module
	esp_log_level_set(TX_TASK_TAG, ESP_LOG_ERROR);
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_ERROR);
    esp_log_level_set(UART_ISR_TASK_TAG, ESP_LOG_ERROR);
    // GPIO Module
    esp_log_level_set(BLINK_TASK_TAG, ESP_LOG_NONE);
    // FS - File System Module
    esp_log_level_set(FS_TASK_TAG, ESP_LOG_ERROR);

    //Initialise the config file system - MOUNT ONLY - this is done on each start up
    fs_initialise();


/******************************************************************************************/
// START TASKS HERE
/******************************************************************************************/
    // BLINKY
    blink_init(20);
    // UART
    init_uart();
    // Wifi
    wifi_init_station_mode();
    // MQTT
    mqtt_app_init();
    // SOCKET
    #ifdef SOCKET_SERVER
        // TCP SOCKET SERVER Module
        esp_log_level_set(TCP_SOCK_SERVER_TAG, ESP_LOG_INFO);
        socket_server_init();
    #elif SOCKET_CLIENT
        // TCP SOCKET CLIENT Module
        esp_log_level_set(TCP_SOCK_CLIENT_TAG, ESP_LOG_INFO);
        socket_client_init();
    #endif

    //APPLICATION TASKS    
    init_rm200x_application();

    //fs_test();
    //fs_finalise();

    // IS EVERYTHING NECESSARY RUNNING
    
}

/******************************************************************************************/
// MAIN   -- END --
/******************************************************************************************/