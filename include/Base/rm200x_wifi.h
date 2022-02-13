#pragma once

#ifndef __RM200X_WIFI_H__
#define __RM200X_WIFI_H__

//FREERTOS CONTROL ITEMS
#define INCLUDE_vTaskSuspend    1

//STANDARD IO Headers
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"

//Base Headers


#ifdef __cplusplus
extern "C" {
#endif

//FREERTOS ITEMS
extern SemaphoreHandle_t bin_s_sync_wifi_connected;

//Task Tags
extern const char *WIFI_TAG;

#define WIFI_SSID   "ASUS"
#define WIFI_PASS   "62D57EBC35"
#define RETRY_LIMIT 20

//Variables
extern volatile esp_netif_ip_info_t RM_IP;

void wifi_init_station_mode(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*  __RM200X_WIFI_H__  */