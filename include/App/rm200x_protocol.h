#pragma once

#ifndef __RM200X_PROTOCOL_H__
#define __RM200X_PROTOCOL_H__

//STANDARD IO Headers
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

//Function Headers
#include "JSON/json-maker.h"
#include "JSON/tiny-json.h"



//ESP Headers

//Base Headers

//Application Headers
#include "App/rm200x_frame_definitions.h"
#include "App/rm200x_frame.h"


#ifdef __cplusplus
extern "C" {
#endif

//Task Tags
extern const char *PROTOCOL_TAG;

// Definitions
#define JSON_STRING_LENGTH      512
#define JSON_MAX_NO_TOKENS      32

//Frame store data
extern aupAudio_t           intent_0x10_data;
extern aupEQ_t              intent_0x11_data;
extern aupPowerState_t      intent_0x21_data;

//PROTOTYPES
char* intent_0x10_json(char *pt_str_json, uint8_t *frame);
char* intent_0x11_json(char *pt_str_json, uint8_t *frame);
char *intent_0x21_json(char *pt_str_json, uint8_t *frame);



//Command Prototypes
uint8_t json_volume_set(char *p_str);
uint8_t json_mute_set(char *p_str);


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __RM200X_PROTOCOL_H__ */