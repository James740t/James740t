#pragma once

#ifndef __RM200X_FRAME_DEFINITIONS_H__
#define __RM200X_FRAME_DEFINITIONS_H__

/******************************************************************************************/
// INTENT DEFINES
/******************************************************************************************/
//----- Intent name                intent/hex   Index(dec)
#define CMD_ACK                      (0x02U) // 00
#define CMD_AUDIO                    (0x10U) // 01
#define CMD_EQ                       (0x11U) // 02
#define CMD_RADIO_SET                (0x12U) // 03
#define CMD_RADIO_FREQUENCY_STATE    (0x13U) // 04
#define CMD_RADIO_PS                 (0x14U) // 05
#define CMD_RADIO_RT                 (0x15U) // 06
#define CMD_RADIO_RDS                (0x16U) // 07
#define CMD_BT_STATUS                (0x17U) // 08
#define CMD_BT_TELEPHONE             (0x18U) // 09
#define CMD_BT_SPP_TFR               (0x19U) // 10
#define CMD_CURRENT_MEDIA_TITLE_INFO (0x1AU) // 11
#define CMD_PBAP                     (0x1BU) // 12
#define CMD_RADIO_HU_STATE           (0x1CU) // 13
#define CMD_DAB_STATUS               (0x1DU) // 14
#define CMD_DAB_STATION_NAME         (0x1EU) // 15
#define CMD_DAB_ENSEMBLE             (0x1FU) // 16
#define CMD_DAB_DLS                  (0x20U) // 17
#define CMD_POWER_STATE              (0x21U) // 18
#define CMD_EEPROM_DATA              (0x22U) // 19
#define CMD_RTC_INFORMATION          (0x23U) // 20
#define CMD_RUNNING_TIME_INFORMATION (0x24U) // 21
#define CMD_MEDIA_STATE              (0x25U) // 22
#define CMD_HANDSHAKE                (0xA0U) // 23
#define CMD_REPLY_ACK                (0x02U) // 24
#define CMD_REQUEST_DATA             (0xB0U) // 25
#define CMD_BT_PBAP_MEDIA_ADDRESSES  (0x26U) // 26
#define CMD_SOFTWARE_VERSIONS        (0x27U) // 27
#define CMD_HU_BT_NAME               (0x28U) // 28
#define CMD_HU_HARDWARE_DEFINITION   (0x29U) // 29
#define CMD_RADIO_STATION_0x51       (0x51U) // 30
#define CMD_TOTAL_COUNT_0x56         (0x56U) // 31
#define CMD_BT_DEVICES_0x54          (0x54U) // 32
#define CMD_BT_EXTENDED_INFO_0x2B    (0x2BU) // 33
#define CMD_FILES_ON_MEDIA_0x2A      (0x2AU) // 34
#define CMD_LIST_INVALIDATION_0x59   (0x59U) // 35
#define CMD_TEXT_0x5A                (0x5AU) // 36

/******************************************************************************************/
// ENUMERATIONS AND LIST DEFINES
/******************************************************************************************/

//----------------- ACKnowledge reception of UART message CMD_REPLY_ACK 24 0x02
#define ACK_SUCCESS                 0x00     // Success
#define ACK_UNKNOWN_INTENT          0x01     // Error: Unknown Intent
#define ACK_BAD_LENGTH              0x02     // Error: Inconsistent message length
#define ACK_CRC_ERROR               0x03     // Error: CRC wrong - depreciated
#define ACK_DATA_NOT_AVAILABLE      0x04     // Requested data is not available (or request not granted, or requested operation failed)
#define ACK_FLASH_PAGE_EMPTY        0x05     // Flash Page is Empty
#define ACK_FLASH_PAGE_NOT_EMPTY    0x06     // Flash Page is not Empty
#define ACK_DEPRECIATED             0x07     //(Undefined, need checking host firmware source code) - Depreciated
#define ACK_BROWSING_NG             0x08     // Browse navigation is not available(used by intent 0x92 “Browse navigation”)
#define ACK_VALUE_OUT_OF_RANGE      0x09     // Value out of range / value(s) in data field inconsistent with or not supported by intent
#define ACK_SYSTEM_BUSY             0x0A     // System busy(used by intent pair 0xD6 0x56)
#define ACK_SAVE_DAB_PRESET_FAILED  0x0B     // DAB stations save Preset Failed
#define ACK_UNKNOWN_ERROR           0xFF

/******************************************************************************************/
// TYPE DEFINITIONS
/******************************************************************************************/
//---------------------------------- Power State Intent CMD_POWER_STATE 18 0x21
typedef union aupPowerState{
    uint8_t entire8bit;
    struct {
        uint8_t accOn    : 1;
        uint8_t powerOn  : 1;
        uint8_t sleep    : 1; // 0 == no sleep request, 1 == sleep request
        uint8_t z        : 5; //----------------- patch unused bits [3..7]
    } __attribute__((packed));
} __attribute__((packed))
aupPowerState_t;

//--------------------------------- Radio Audio Status Intent CMD_AUDIO 01 0x10
typedef union aupAudio {
    uint8_t entire8bitValue;
    struct {
        uint8_t volume : 7;
        uint8_t mute   : 1;
    } __attribute__((packed));
} __attribute__((packed))
aupAudio_t;

//--------------------------------------------- Equaliser Intent CMD_EQ 02 0x11
#define MAX_LEVEL         (14U)   //displayed as +/-7 on Radio
#define MAX_LEVEL_BAL_FAD (20U)   //displays as +/-10 on Radio
    
enum{   ENUM_EQ_MODE_OFF     = 0,
        ENUM_EQ_MODE_POP     = 1,
        ENUM_EQ_MODE_USER    = 2,
        ENUM_EQ_MODE_TECHNO  = 3,
        ENUM_EQ_MODE_ROCK    = 4,
        ENUM_EQ_MODE_CLASSIC = 5,
        ENUM_EQ_MODE_JAZZ    = 6,
        ENUM_EQ_MODE_VOCAL   = 7,
        ENUM_EQ_MODE_END
};
typedef union aupEQ {
    uint64_t full48bitValue : 48;
    struct {
        uint8_t fade;
        uint8_t balance;
        uint8_t bass;
        uint8_t middle;
        uint8_t treble;
        uint8_t eq_mode   : 6; // value range [0..7] need 3 bits + patch 3 bits
        uint8_t soft_mute : 1; //------------------------------ bit 6
        uint8_t loudness  : 1; //------------------------------ bit 7
    } __attribute__((packed));
} __attribute__((packed))
aupEQ_t;



#ifdef __cplusplus
extern "C" {
#endif

// Place variables and etc here

#ifdef __cplusplus
} // extern "C"
#endif

#endif  /*  __RM200X_FRAME_DEFINITIONS_H__  */
