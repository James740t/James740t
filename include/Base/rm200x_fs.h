#pragma once

#ifndef __RM200X_FILE_SYSTEM_H__
#define __RM200X_FILE_SYSTEM_H__

//STANDARD IO Headers
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <sys/unistd.h>
#include <sys/stat.h>

//FREERTOS Headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

//ESP Headers
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_err.h"

//Base Headers
#include "base/rm200x_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

//FILE SYSTEM DEFINITIONS
// (very basic to hold operational)
// (configuration files only)
#define PARTITION_NAME      "spiffs"          //Name used in the sub-type of partition.csv
#define BASE_PATH           "/spiffs"
#define CONF_FILE_NAME      "/spiffs/rm200x.txt"
#define MAX_FILES           5

//Operational Filenames
#define CONFIG_FILENAME     "/spiffs/app_config.txt"
#define CONFIG_BIN_FILENAME "/spiffs/app_config.bin"

//Task Tags
extern const char *FS_TASK_TAG;

//Enumerations

// MOUNTING Prototypes
void fs_initialise(void);
void fs_finalise(void);
// TEXT Prototypes
bool fs_write(const char *pt_fliename, const char *pt_text);
bool fs_append(const char *pt_fliename, const char *pt_text);
bool fs_read(const char *pt_fliename, char *pt_text, size_t text_length);
bool fs_read_next_line(const char *pt_fliename, char *pt_text, size_t text_length);
// STRUCT Prototypes
bool fs_write_struct(const char *pt_fliename, const void *pt_data, size_t strut_size);
bool fs_append_struct(const char *pt_fliename, const void *pt_data, size_t strut_size);
bool fs_read_struct(const char *pt_fliename, void *pt_data, size_t strut_size, int r_item);
bool fs_read_next_struct(const char *pt_fliename, void *pt_data, size_t strut_size);
// Utility Prototypes
bool fs_rename(const char *pt_newname, const char *pt_oldname);
bool fs_delete(const char *pt_fliename);
unsigned long fs_size(const char *pt_filename);


void fs_test (void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif  /*  __RM200X_FILE_SYSTEM_H__  */