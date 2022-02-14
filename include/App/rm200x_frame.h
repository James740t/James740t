#pragma once

#ifndef __RM200X_FRAME_H__
#define __RM200X_FRAME_H__

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
#include "base/rm200x_uart.h"

//Application Headers
#include "App/rm200x_frame_definitions.h"
#include "App/rm200x_ack.h"
#include "App/rm200x_radio_status.h"

//FREERTOS CONTROL ITEMS
#define INCLUDE_vTaskSuspend    1

extern TaskHandle_t xHandle_Frame_Rx;

extern QueueHandle_t xqFrame_Rx;
extern QueueHandle_t xqFrame_Process;

//Definitions
#define FRAME_PORT              UART_NUM_2

#define FRAME_BUFFER_SIZE       136
#define FRAME_QUEUE_DEPTH       8
#define FRAME_PROCESSOR_DEPTH   16

//Types
//typedef struct uart_message_t uart_message_t;
//typedef struct ack_message_t ack_message_t;

typedef union
{
  struct
  {
    unsigned char bit0 : 1;
    unsigned char bit1 : 1;
    unsigned char bit2 : 1;
    unsigned char bit3 : 1;
    unsigned char bit4 : 1;
    unsigned char bit5 : 1;
    unsigned char bit6 : 1;
    unsigned char bit7 : 1;
  }bits;
  unsigned char byte;
}byte;

#ifdef __cplusplus
extern "C" {
#endif

//FREERTOS CONTROL ITEMS

//Task Tags
extern const char *FRAME_TASK_TAG;

//defines and constants
//#define N_ELEMS(x)  (sizeof(x) / sizeof((x)[0]))

//Enumerations

//Prototypes
uint8_t CreateRollingCounter(void);
uint8_t Get_Frame_Length (const uint8_t *pt_frame_array);
uint8_t Get_Frame_CRC (const uint8_t *pt_frame_array);
uint8_t Get_Frame_Payload (const uint8_t *pt_frame_array);
uint8_t Get_Frame_Intent (const uint8_t *pt_frame_array);
uint8_t Get_Rolling_Counter (const uint8_t *pt_frame_array);

uint8_t Calculate_Checksum(const uint8_t *pt_frame_array);
uint8_t CheckFrame(const uint8_t *pt_frame_array);

uint8_t CreateFrame(uint8_t *pt_frame, const uint8_t pt_intent, const uint8_t data_size, const uint8_t *pt_data);
uint8_t CreateSendFrame(const uint8_t pt_intent, const uint8_t data_size, const uint8_t *pt_data);

void init_rm200x_application(void);

//TASKS
void process_Frame_task(void *arg);

#ifdef __cplusplus
} // extern "C"
#endif

#endif  /*  __RM200X_FRAME_H__  */