#pragma once

#ifndef __RM200X_PROTOCOL_H__
#define __RM200X_PROTOCOL_H__

//STANDARD IO Headers
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

//FREERTOS Headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"


//Function Headers
#include "JSON/json-maker.h"
#include "JSON/tiny-json.h"

//ESP Headers
#include "esp_log.h"

//Base Headers
#include "Base/rm200x_mqtt.h"

//Application Headers
#include "App/rm200x_frame_definitions.h"
#include "App/rm200x_frame.h"
#include "App/rm200x_protocol.h"
#include "rm200x_ack.h"


#ifdef __cplusplus
extern "C" {
#endif

//Task Tags
extern const char *PROTOCOL_TAG;
extern const char *RADIO_STATUS_TASK_TAG;
extern const char *RADIO_COMMAND_TASK_TAG;

//FREERTOS CONTROL ITEMS
#define INCLUDE_vTaskSuspend    1
extern TaskHandle_t xHandle_Audio_in_task;
extern QueueHandle_t xqACK_queue2;

//TYPE DEFINITIONS
typedef struct uart_message_t uart_message_t;
typedef struct ack_message_t ack_message_t;


//Definitions


// Definitions
#define JSON_TX_STRING_LENGTH   128
#define JSON_MAX_NO_TOKENS      32
#define JSON_MAX_INTENTS        4
#define STATUS_PORT             FRAME_PORT
#define STATUS_BUFFER_SIZE      FRAME_BUFFER_SIZE
#define STATUS_QUEUE_DEPTH      8

//Definition data

//Frame store data
extern aupAudio_t           intent_0x10_data;
extern aupEQ_t              intent_0x11_data;
extern aupPowerState_t      intent_0x21_data;

//PROTOTYPES
char *intent_0x10_json(char *pt_str_json, uint8_t *frame);
char *intent_0x11_json(char *pt_str_json, uint8_t *frame);
char *intent_0x21_json(char *pt_str_json, uint8_t *frame);

//Command Prototypes
uint8_t protocol_command_json_process(char *p_str);

// TASKS
void protocol_rm200x_input_task(void *arg);


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __RM200X_PROTOCOL_H__ */