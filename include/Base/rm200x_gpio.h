#pragma once

#ifndef __RM200X_GPIO_H__
#define __RM200X_GPIO_H__

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
#include "freertos/timers.h"

//ESP Headers
#include "esp_log.h"

//GPIO Headers
#include "driver/gpio.h"

//Base Headers
#include "base/rm200x_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

//FREERTOS CONTROL ITEMS
extern SemaphoreHandle_t bin_s_sync_blink_task;
extern TaskHandle_t xHandle_blink;
//Task Tags
extern const char *BLINK_TASK_TAG;

#define BLINK_GPIO              2       // GPIO_NUM_2
#define GPIO_ON                 1
#define GPIO_OFF                0
#define MIN_FLASH_TIME          10      // [mS]
#define MAX_FLASH_TIME          2000    // [mS]

void blink_init(uint16_t p_flash_time);

#ifdef __cplusplus
} // extern "C"
#endif

#endif  /*  __RM200X_GPIO_H__  */