#pragma once

#ifndef __RM200X_SOCKET_SERVER_H__
#define __RM200X_SOCKET_SERVER_H__

//STANDARD IO Headers
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

//FREERTOS Headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

//ESP Headers
#include "esp_log.h"
#include "lwip/sockets.h"

//Base Headers
#include "base/rm200x_gpio.h"
#include "Base/rm200x_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_IPV4     1
//#define CONFIG_IPV6     1

//#define SOCKET_CLIENT   1
//#define SOCKET_SERVER   1

//FREERTOS ITEMS
extern TaskHandle_t xHandle_tcpip_tx;
extern TaskHandle_t xHandle_tcpip_rx;

extern QueueHandle_t xqTCPIP_rx;
extern QueueHandle_t xqTCPIP_tx;

//Task Tags
extern const char *TCP_SOCK_SERVER_TAG;
extern const char *TCP_SOCK_CLIENT_TAG;

//Client Definitions
#define HOST_IP_ADDR                "192.168.50.150"    //For client only - PC Fred
#define HOST_SERVER_PORT            3333
//Server Definitions
#define LOCAL_SERVER_PORT           3333
//Common
#define KEEPALIVE_IDLE              3   // [s]  - Keep alive time in seconds
#define KEEPALIVE_INTERVAL          1   // [s]  - time period to probe keep alive
#define KEEPALIVE_COUNT             8  // [x1] - tries before timing out

#define SOCKET_BUFFER_SIZE          1024
#define SOCKET_QUEUE_DEPTH          32

//TYPE DEFINITIONS
typedef struct TCPIPMessage tcpip_message_t;
struct TCPIPMessage
{
    int ID;
    char data[SOCKET_BUFFER_SIZE];
    uint16_t length;
};

int SOCKET;

void socket_server_init(void);
void socket_client_init(void);

void send_to_TCPIP(int p_socket, const char *pt_message, uint16_t p_length);
int socket_send(const char *tag, const int sock, const char * data, const size_t len);

int tcpip_send(tcpip_message_t *tcpip_out);


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __RM200X_SOCKET_SERVER_H__ */