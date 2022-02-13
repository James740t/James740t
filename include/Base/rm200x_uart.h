#pragma once

#ifndef __RM200X_UART_H__
#define __RM200X_UART_H__

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

//GPIO Headers
#include "driver/gpio.h"

//UART Headers
#include "driver/uart.h"
#include "string.h"

//Base Headers
#include "base/rm200x_gpio.h"
#include "base/rm200x_mqtt.h"
#include "base/rm200x_socket.h"

//Application Headers
#include "app/rm200x_ack.h"

#ifdef __cplusplus
extern "C" {
#endif

//FREERTOS CONTROL ITEMS
#define INCLUDE_vTaskSuspend    1

//FREERTOS ITEMS
extern SemaphoreHandle_t bin_s_sync_uart_tx_task;
extern SemaphoreHandle_t bin_s_sync_uart_rx_process_task;

extern TaskHandle_t xHandle_uart_tx;
extern TaskHandle_t xHandle_uart_rx;
extern TaskHandle_t xHandle_uart_isr;

extern QueueHandle_t xqUART_events;
extern QueueHandle_t xqUART_rx;
extern QueueHandle_t xqUART_tx;


//Task Tags
extern const char *TX_TASK_TAG;
extern const char *RX_TASK_TAG;
extern const char *UART_ISR_TASK_TAG;

//UART
#define EX_UART_NUM                     UART_NUM_2
#define UART_BUFFER_SIZE                    1024
#define UART_RX_READ_TIMEOUT_MS             3
#define UART_RX_QUEUE_DEPTH                 32
#define UART_TX_QUEUE_DEPTH                 32
#define UART_EVENT_QUEUE_DEPTH              32
#define UART_FIXED_MIN_TX_DELAY_MS          5
extern uint16_t UART_VARIABLE_TX_DELAY;

// PATTERN DEFINITIONS
#define PATTERN_DETECTION   false
#define PATTERN_QUEUE_SIZE  32
#define PATTERN_CHR         0xFF
#define PATTERN_CHR_NUM     (1)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

//TYPE DEFINITIONS
typedef struct uart_message_t uart_message_t;
struct uart_message_t
{
    int Message_ID;
    uint8_t port;
    bool IsHEX;
    bool IsASCII;
    bool IsFrame;
    char data[UART_BUFFER_SIZE];
    uint16_t length;
};

//Globals
extern uint16_t UART_VARIABLE_TX_DELAY;
extern bool ALL_ASCII;
extern bool HEX_DELIMITED;

void init_uart(void);
void tx_uart_task(void *arg);
void uart_event_task(void *pvParameters);

//Helpers
int Hex2Bytes(const char *hexStr, char *output, int *outputLen);

int BytesToHexString(char *pt_out_str, const uint8_t *pt_in_bytes, const uint8_t input_length);
int BytesToHexString_hyp_delim(char *pt_out_str, const uint8_t *pt_in_bytes, const uint8_t input_length);

#ifdef __cplusplus
} // extern "C"
#endif

#endif  /*  __RM200X_UART_H__  */