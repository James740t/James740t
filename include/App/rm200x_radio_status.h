#pragma once

#ifndef __RM200X_RADIO_STATUS_H__
#define __RM200X_RADIO_STATUS_H__

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

//Application Headers
#include "rm200x_frame.h"
#include "rm200x_ack.h"

#ifdef __cplusplus
extern "C" {
#endif

//FREERTOS CONTROL ITEMS
#define INCLUDE_vTaskSuspend    1

extern TaskHandle_t xHandle_task1;
extern TaskHandle_t xHandle_task2;

extern QueueHandle_t xqACK_queue1;
extern QueueHandle_t xqACK_queue2;

//TYPE DEFINITIONS
typedef struct uart_message_t uart_message_t;
typedef struct ack_message_t ack_message_t;


//Task Tags
extern const char *RADIO_STATUS_TASK_TAG;

//Enumerations

// Public Prototypes
void init_radio_status(void);


#ifdef __cplusplus
} // extern "C"
#endif

#endif  /*  __RM200X_RADIO_STATUS_H__  */