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

//JSON Header
#include "JSON/json-maker.h"

//Base Headers
#include "Base/rm200x_mqtt.h"

//Application Headers
#include "rm200x_frame_definitions.h"
#include "rm200x_protocol.h"
#include "rm200x_frame.h"
#include "rm200x_ack.h"

#ifdef __cplusplus
extern "C" {
#endif

//FREERTOS CONTROL ITEMS
#define INCLUDE_vTaskSuspend    1

extern TaskHandle_t xHandle_Audio_in_task;
extern TaskHandle_t xHandle_Audio_out_task;

//extern QueueHandle_t xqAUDIO_tx;
extern QueueHandle_t xqACK_queue2;

//TYPE DEFINITIONS
typedef struct uart_message_t uart_message_t;
typedef struct ack_message_t ack_message_t;

//Task Tags
extern const char *RADIO_STATUS_TASK_TAG;
extern const char *AUDIO_TASK_TAG;
extern const char *RADIO_COMMAND_TASK_TAG;

//Definitions
#define STATUS_PORT             FRAME_PORT

#define STATUS_BUFFER_SIZE      FRAME_BUFFER_SIZE
#define STATUS_QUEUE_DEPTH      8



//Enumerations

// Public Prototypes


// TASKS
void audio_IN_task(void *arg);
void audio_OUT_task(void *arg);


#ifdef __cplusplus
} // extern "C"
#endif

#endif  /*  __RM200X_RADIO_STATUS_H__  */