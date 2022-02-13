#pragma once

#ifndef __RM200X_MQTT_H__
#define __RM200X_MQTT_H__

//STANDARD IO Headers
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

//ESP Headers
#include "esp_log.h"

//MQTT Headers
#include "mqtt_client.h"
#include "mqtt_supported_features.h"

//FREERTOS Headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

//Base Headers
#include "base/rm200x_gpio.h"
#include "base/rm200x_uart.h"
#include "base/rm200x_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

//DEBUG
//#define MQTT_DEBUG

#ifdef MQTT_DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif

//FREERTOS CONTROL ITEMS
extern TaskHandle_t xHandle_rx_mqtt;
extern TaskHandle_t xHandle_tx_mqtt;

extern QueueHandle_t xqMQTT_rx_Messages;
extern QueueHandle_t xqMQTT_tx_Messages;

//Task Tags
extern const char *MQTT_RX_TASK_TAG;
extern const char *MQTT_TX_TASK_TAG;

#define MQTT_PREFIX_MAX_LEN     16
#define MQTT_SUFFIX_MAX_LEN     16
#define MQTT_TOPIC_MAX_LEN      64      // Will include both prefix and suffix eventually
#define MQTT_TOPIC_MAX_DEPTH    5       // The maximum number of / separated topic items
#define MQTT_DATA_MAX_LEN       1024    // Artificial limit - reduce to suit application
#define MQTT_MESSAGE_MAX_LEN    1088    // DATA length + TOPIC length - Artificial limit - reduce to suit application
#define MQTT_Q_DEPTH            32      // RX and Tx buffer queue depths

//TYPE DEFINITIONS
typedef struct st_MQTT_TOPIC mqtt_topic_t;
struct st_MQTT_TOPIC
{
    char prefix [MQTT_PREFIX_MAX_LEN];
    char base_topic  [MQTT_TOPIC_MAX_LEN - (MQTT_PREFIX_MAX_LEN + MQTT_SUFFIX_MAX_LEN)];
    char suffix [MQTT_SUFFIX_MAX_LEN];
    char topic [MQTT_TOPIC_MAX_LEN];
};

typedef struct st_MQTT_MESSAGE mqtt_esp_message_t;
struct st_MQTT_MESSAGE
{
    char data [MQTT_DATA_MAX_LEN];      /*!< Data associated with this event */
    int data_len;                       /*!< Length of the data for this event */
    int total_data_len;                 /*!< Total length of the data (longer data are supplied with multiple events) */
    int current_data_offset;            /*!< Actual offset for the data associated with this event */
    char topic [MQTT_TOPIC_MAX_LEN];    /*!< Topic associated with this event */
    int topic_len;                      /*!< Length of the topic for this event associated with this event */
    int msg_id;                         /*!< MQTT messaged id of message */
    int qos;                            /*!< MQTT QoS level set for TX of message */
    bool retain;                        /*!< Retained flag of the message associated with this event */

    mqtt_topic_t topic_detail;          /*!< Application topic detail prefix, base and suffix */
};

//Application topic forms
//message will be "/prefix/topic/payload"
//e.g. /tele/rm200x/"message from radio here"
#define MQTT_SUBSCRIPTION_TOPIC "/+/rm200x/"        //General topic for this application to subscribe too

#define MQTT_CMND_PREFIX        "cmnd"              //input command topic prefix
#define MQTT_COMM_PREFIX        "comm"              //Direct payload to the COM port 
#define MQTT_HEX_COMM_PREFIX    "comm_h"            //Direct payload to the COM port via a HEX parser
#define MQTT_HEX_SOCKET_PREFIX  "socket"            //Direct payload to the tcpip socket
#define MQTT_COMMAND_TOPIC      "/cmnd/rm200x/"     //Short-cut used for core subscription
#define MQTT_TELE_PREFIX        "tele"              //output telemetery topic prefix
#define MQTT_STATUS_PREFIX      "status"            //output status topic prefix
#define MQTT_TELE_SUFFIX        ""                  //topic suffix if needed
#define MQTT_BASE_TOPIC         "rm200x"            //basic topic

#define MQTT_DATA               "data"              //I/O data topic (see formal description of its payload - RM200x UART Protocol)

//Local Broker details
#define MQTT_SERVER             "mqtt://192.168.50.124"
#define MQTT_USER               "mqtt"
#define MQTT_PASSWORD           "mqttmqtt"

extern esp_mqtt_client_handle_t mqtt_client;

int str_cut(char *str, int begin, int len);
void copy_mqtt_message_strut(mqtt_esp_message_t *pt_dest, mqtt_esp_message_t *pt_source);
void clear_mqtt_message_strut(mqtt_esp_message_t *pt_dest);
void convert_mqtt_input_message(mqtt_esp_message_t *pt_dest, esp_mqtt_event_t *pt_source);

int build_topic(mqtt_topic_t *pt_topic, const char *prefix, const char *topic, const char *suffix);
void print_topic(mqtt_topic_t *pt_topic_data);
int complete_topic(mqtt_topic_t *pt_topic_data);

int parse_mqtt_topic_string(mqtt_topic_t *pt_topic_data);

void mqtt_app_init(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __RM200X_MQTT_H__ */

