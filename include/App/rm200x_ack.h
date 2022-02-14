#pragma once

#ifndef __RM200X_ACK_H__
#define __RM200X_ACK_H__

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

//Base Headers
#include "Base/rm200x_uart.h"

//Application Headers
#include "App/rm200x_frame_definitions.h"
#include "rm200x_frame.h"

#ifdef __cplusplus
extern "C" {
#endif

//FREERTOS CONTROL ITEMS
#define INCLUDE_vTaskSuspend    1

extern TaskHandle_t xHandle_ACK_Reply;
extern TaskHandle_t xHandle_ACK_Process;

extern QueueHandle_t xqACK_Send_Reply;
extern QueueHandle_t xqACK_Response;

//TYPE DEFINITIONS
typedef struct uart_message_t uart_message_t;
typedef struct ack_message_t ack_message_t;
struct ack_message_t
{
    uint8_t Intent;
    uint8_t Counter;
    uint8_t Status;
};

#define ACK_PORT                FRAME_PORT

#define ACK_BUFFER_SIZE         FRAME_BUFFER_SIZE
#define ACK_QUEUE_DEPTH         16
#define ACK_FRAME_LENGTH        8

//Task Tags
extern const char *ACK_REPLY_TASK_TAG;
extern const char *ACK_PROCESS_TASK_TAG;

//Enumerations

// Public Prototypes
uint8_t Create_ACK_Reply(uint8_t *pt_ack, const uint8_t *pt_frame_in_array, uint8_t p_ack_state);
uint8_t Create_ACK_Reply_UART(uart_message_t *pt_ack_uart, const uint8_t *pt_frame_in_array, uint8_t p_ack_state);

void init_ack(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif  /*  __RM200X_ACK_H__  */