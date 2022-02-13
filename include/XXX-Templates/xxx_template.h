#pragma once

#ifndef __XXX_TEMPLATE_H__
#define __XXX_TEMPLATE_H__

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
#include "App/rm200x_ack.h"

#ifdef __cplusplus
extern "C" {
#endif

//FREERTOS CONTROL ITEMS
TaskHandle_t xHandle_task;
const char *TASK_TASK_TAG;

//PROTOTYPES
void initiate_template(void);


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __XXX_TEMPLATE_H__ */