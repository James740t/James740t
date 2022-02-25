#pragma once

#ifndef __RM200X_FRAME_DEFINITIONS_H__
#define __RM200X_FRAME_DEFINITIONS_H__

/******************************************************************************************/
// MODERN INTENT DEFINES (2022)
/******************************************************************************************/

enum AUDIO_0x90
{
    K_NONE = 0x00,
    K_POWER,        // Plus POWER enum
    K_MUTE,         // Plus ON_OFF enum
    K_EQ,           // Plus EQ enum
    K_FADE,         // Data ranges between -7 to +7 (use 0 to 14 and decode)
    K_BALANCE,      // Data ranges between -7 to +7 (use 0 to 14 and decode)
    K_BASS,         // Data ranges between -7 to +7 (use 0 to 14 and decode)
    K_MIDDLE,       // Data ranges between -7 to +7 (use 0 to 14 and decode)
    K_TREBLE,       // Data ranges between -7 to +7 (use 0 to 14 and decode)
    K_VOLUME,       // Data ranges between 0 and 100 (different radios have different limits)
    K_LOUDNESS,     // Plus ON_OFF enum
    K_CAN_SIDE_ACC  // Plus POWER enum
};

enum RADIO_0x90
{
    K_RADIO_BAND = 0x20, // Plus BAND enum
    K_RADIO_SOFT_MUTE,   // Plus ON_OFF enum
    K_RADIO_HFP,         // Data byte is cab number 0 to 255
    K_SEEK_UP,
    K_SEEK_DOWN,
    K_RADIO_RESERVED,     // Plus ON_OFF enum
    K_RADIO_PTY_RDS_TYPE, // Data follows US_RBDS standard 0-31
    K_RADIO_AF_CONTROL,   // Plus ON_OFF enum
    K_RADIO_TA_CONTROL,   // Plus ON_OFF enum
    K_RADIO_CT_CONTROL,   // Plus ON_OFF enum
    K_RADIO_LOC_CONTROL,  // Plus ON_OFF enum
    K_RADIO_REGION,       // Plus REGIONGET enum
    K_RADIO_BEEP,         // Plus ON_OFF enum
    K_RADIO_RESET,        // Plus ON_OFF enum
    K_RADIO_12_24_HR,     // Plus CLOCK_MODE enum
    K_RADIO_TONE,         // Play a preset tone # 0 to 255

    K_RADIO_RDS_CONTROL, // RDS 0: OFF;1:ON
    K_RADIO_TUNE_DOWN,
    K_RADIO_TUNE_UP,
    K_RADIO_ILLUMINATION,  // Illumination brightness. If not variable 0<49% Illumination OFF, >=50% Illumination ON
    K_BEEP_GENERATE = 0x36 // value 0 255 0 cancel (silent immediately) 1 beep 25mS 2 beep 50mS 80 beep 2 seconds(limit) Values > 80 generates 2 seconds
};

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
#define CMD_FACTORY_RESET_0x59       (0x59U) // 35
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

//--------------------------------- Radio Set Info Intent CMD_RADIO_SET 03 0x12
#define MAX_PTY (32U)

enum{   ENUM_SEARCH_OFF     = 0,
        ENUM_SEARCH_UP,
        ENUM_SEARCH_DOWN,
        ENUM_SEARCH_TA,
        ENUM_SEARCH_AF,
        ENUM_SEARCH_PI,
        ENUM_SEARCH_END //must be last entry
};

typedef enum
{   
    ENUM_EUROPE             = 0,
    ENUM_ASIA               = 1,
    ENUM_MIDDLE_EAST        = 2,
    ENUM_AUSTRALIA          = 3,
    ENUM_USA                = 4,
    ENUM_LATIN_AMERICA      = 5,
    ENUM_BRAZIL             = 6,
    ENUM_JAPAN              = 7,
    ENUM_RUSSIA             = 8,
    ENUM_REGION_END         = 9
} AAMP_Region_t;

typedef union aupRadioSetInfo {
    uint32_t entire32bit;
    struct {
        uint8_t af            : 1;
        uint8_t ta            : 1;
        uint8_t stereo        : 1;
        uint8_t loc_dx        : 1;
        uint8_t rds           : 1;
        uint8_t pty           : 1;
        uint8_t z             : 2;
        uint8_t search_type;
        uint8_t pty_set;      //---------- used value [0..32], unused [33..255]
        uint8_t region;
    } __attribute__((packed));
} __attribute__((packed))
aupRadioSetInfo_t;

//-------------- Radio Frequency State Intent CMD_RADIO_FREQUENCY_STATE 04 0x13
#define PTY_ID_MAX                (32U)
#define MAX_RADIO_SIGNAL_STRENGTH (100U)

typedef union aupRadioFreqState {
    uint64_t full40bitValue : 40;
    struct {
        uint8_t ta         : 1;
        uint8_t tp         : 1;
        uint8_t eon        : 1;
        uint8_t z3         : 5;
        uint8_t pi_counntry;
        uint8_t pi_programId;
        uint8_t receive_pty;
        uint8_t signal_strength;
    } __attribute__((packed));
} __attribute__((packed))
aupRadioFreqState_t;

//----------------------------------- Radio PS Information CMD_RADIO_PS 05 0x14
#define FIXED_SIZE_PS_CHAR (8U)

typedef struct aupRadioPsInfo{
    uint8_t ps_info[FIXED_SIZE_PS_CHAR];
}__attribute__((packed))
aupRadioPsInfo_t;

//---------------------------- Radio RT Information Intent CMD_RADIO_RT 06 0x15
#define MAX_RT_CHAR (64U)
#define INVALID_RT   (0U)

typedef struct aupRadioRTInfo {
    uint8_t rt_length;
    uint8_t rt_info[MAX_RT_CHAR];
} __attribute__((packed))
aupRadioRTInfo_t;

//------------------------- Radio RDS Information Intent CMD_RADIO_RDS 07 0x16U
#define FIX_SIZE_RDS_INFO_FIELD 0x0DU
#define MAX_GROUP_NUMBER 0x1FU

typedef struct aupRadioRDSInfo {
    uint8_t rds_group; //-- 0x00~0x0F = group 0A~15A, 00x10~0x1F = group 0B~15B
    uint8_t rds_info[FIX_SIZE_RDS_INFO_FIELD]; //--- 108 bits per message group
} __attribute__((packed))
aupRadioRDSInfo_t;

//-------------------------------- Radio BT_Status Intent CMD_BT_STATUS 08 0x17
enum
{
    ENUM_NO_CONNECT = 0x00, //---- aupBtStaus : phone_connect, a2dp_connect
    ENUM_PAIRING = 0x01,    // pairing
    ENUM_LINKING = 0x02,    // 
    ENUM_CONNECT = 0x03     // 
};
enum
{   
    ENUM_NONE       = 0x00, //-------------------------- aupBtStaus.phone_state
    ENUM_CALL_IN    = 0x01,
    ENUM_CALL_OUT   = 0x02,
    ENUM_CALL_ACTIVE= 0x03
};
enum
{
    ENUM_TRANSMIT = 2,
    ENUM_RECEIVING,
};
enum{   ENUM_NO_TEXT_MESSAGE = 0,
        ENUM_TEXT_MESSAGE_RECEIVED
};

typedef struct aupBtStatus
{
    uint8_t phone_connect  : 2;
    uint8_t a2dp_connect   : 2;
    uint8_t phone_state    : 2;
    uint8_t spp            : 2;
    //-------------------------------------------------------------------------
    uint8_t btphonenameidx : 4;
    uint8_t hfpstate       : 1;
    uint8_t a2dpstate      : 1;
    uint8_t btpowerstate   : 1;
    uint8_t txtmsgin       : 1;
    //---------------------------------------------------- 0x20 == 32 bytes + 1
    uint8_t phone_name[33]; 
    //-------------------------------------------------------------------------
    uint8_t rssi           : 4;
    uint8_t battery_level  : 4;
    //-------------------------------------------------------------------------
    uint8_t mic_muted      : 1;
    uint8_t mic_channel    : 1;
    uint8_t phonebook_available  : 1;
    uint8_t call_on_hold   : 1;
    uint8_t line2_incomming : 1;
    uint8_t z0             : 3;
} __attribute__((packed))
aupBtStatus_t;

//------- BT Telephone Information Intent (is actually CMD_BT_TELEPHONE 09 0x18
//-----------------------------------------------------------------------------
// last telephone number with whom the BT talker had a telephone call, i.e. 
// tel. no. of the "most recent call".
//-----------------------------------------------------------------------------
#define MAX_BT_TEL_NO_CHAR  (0x18U)
#define FIXED_SOFTWARE_CHAR (0x40U)  //spec allows for upto 200 chars, limit it for now to real life amt to reduce mem usage

typedef struct aupBtTelepNum {
    uint8_t numberAscii[MAX_BT_TEL_NO_CHAR];
} __attribute__((packed))
aupBtTelepNum_t;

//------------------------------- BT SPP Transfer Intent CMD_BT_SPP_TFR 10 0x19

#define MAX_SPP_CHAR (128U)

enum {  ENUM_SPP_RECEIVE,
        ENUM_SPP_TRANSMIT
};

typedef struct aupBtSppTransfer {
    union {
        uint8_t packetHeaderByte;
        struct {
            uint8_t packet_number : 7;
            uint8_t packet_type   : 1;
        } __attribute__((packed));
    } __attribute__((packed));
    uint8_t packet[MAX_SPP_CHAR];
} __attribute__((packed))
aupBtSppTransfer_t;

//------------------------ BT AVRCP Intent CMD_CURRENT_MEDIA_TITLE_INFO 11 0x1A
#define MAX_MEDIA_CHAR (40U)

enum {
    ENUM_SONG_TITLE    = 1,
    ENUM_ARTIST        = 2,
    ENUM_ALBUM_TITLE   = 3,
    ENUM_TRACK_NUMBER  = 4,
    ENUM_TOTAL_NUMBER_ALBUM_TRACKS = 5,
    ENUM_GENRE         = 6,
    ENUM_ATTRIBUTE_END = 7 //must be last entry
};

typedef struct aupMediaTitleInfo {
    uint8_t media_attribute;
    uint8_t media_attribute_data[MAX_MEDIA_CHAR];
} __attribute__((packed))
aupMediaTitleInfo_t;

//--------------------------------------------- BT PBAP Intent CMD_PBAP 12 0x1B
#define MAX_PBAP_CHAR (255-2-1)

enum { // changed from 0x0N to 0x4N Nov2019
    ENUM_SONG_TITLES              = 0x41,
    ENUM_MEDIA_ARTISTS            = 0x42,
    ENUM_MEDIA_ALBUM_TITLES       = 0x43,
    ENUM_MEDIA_GENRES             = 0x44,
    ENUM_MEDIA_PLAYLISTS          = 0x45,
    //- original values not changed Nov2019
    ENUM_PHONE_CONTACTS_BY_NAME   = 0x20,
    ENUM_PHONE_CONTACTS_BY_NUMBER = 0x21,
    ENUM_MISSED_CALLS_BY_NAME     = 0x22,
    ENUM_MISSED_CALLS_BY_NUMBER   = 0x23,
    ENUM_RECEIVED_CALLS_BY_NAME   = 0x24,
    ENUM_RECEIVED_CALLS_BY_NUMBER = 0x25,
    ENUM_MADE_CALLS_BY_NAME       = 0x26,
    ENUM_MADE_CALLS_BY_NUMBER     = 0x27,
    //------------------ introduced Nov2019
    ENUM_BT_RECEIVD_CALL          = 4,
    ENUM_BT_DIALLED_CALL          = 5,
    ENUM_BT_MISSESD_CALL          = 6
};

//------------------------------------------------------------------------------
// Phonebook record type
//------------------------------------------------------------------------------
typedef struct aupBtPbap0x1B_part{
    uint8_t pbap_storage;           // type of data
    uint16_t index;                 // position of current record
    uint8_t pbap_data[MAX_PBAP_CHAR-3];    // 249 characters
} __attribute__((packed))
aupBtPbap0x1B_part_t;

//----------------- Radio HeadUnit (HU) State Intent CMD_RADIO_HU_STATE 13 0x1C
#define MAX_SOURCES (0x08U)
#define MAX_STATE   (0x0AU)   //maximum number of possible menu states (see protocol spec for this intent)

#define RADIO_NO_SIGNAL_AMFMWB (0x00)
#define RADIO_FM1 (0x10U)
#define RADIO_FM2 (0x20U)
#define RADIO_FM3 (0x30U)
#define RADIO_AM1 (0x40U)
#define RADIO_AM2 (0x50U)
#define RADIO_LW  (0x60U)
#define RADIO_WB  (0x70U)
#define SD        (0x01U)
#define USB1      (0x02U)
#define AUX1      (0x03U)
#define AUX2      (0x04U)
#define BT_A2DP   (0x05U)
#define DAB1      (0x06U)
#define CD        (0x07U)
#define USB2      (0x09U)
#define DAB2      (0x16U)
#define XM1       (0x18U)
#define DAB3      (0x26U)
#define XM2       (0x28U)
#define XM3       (0x38U)

//-----------------------------------------------------------------------------
// Corresponding/related values used by intent 0x91 (Reverse direction, UART 
// to radio_MCU) use different enumeration. They are placed here for easy of 
// comparison.                                                        18Nov2019
//-----------------------------------------------------------------------------

#define uRADIO_NO_SIGNAL_AMFMWB (0x00)
#define uRADIO_FM1 (0x01U)
#define uRADIO_FM2 (0x02U)
#define uRADIO_FM3 (0x03U)
#define uRADIO_AM1 (0x04U)
#define uRADIO_AM2 (0x05U)
#define uDAB1      (0x06U)
#define uDAB2      (0x07U)
#define uDAB3      (0x08U)
#define uUSB1      (0x09U)
#define uUSB2      (0x0AU)
#define uAUX1      (0x0BU)
#define uAUX2      (0x0CU)
#define uBT_A2DP   (0x0DU)
#define uRADIO_WB  (0x0EU)
#define uRADIO_LW  (0x0FU)
#define uCD        (0x10U)
#define uXM1       (0x11U)
#define uXM2       (0x12U)
#define uXM3       (0x13U)
#define uSD        (0x14U)
#define uLAST_SOURCE (0x15U)

#define uINVALID    (0xdd)

enum{   ENUM_SYSTEM_MODE_POSITION = 0,
        ENUM_CURRENT_PRESET_NUMBER_POSITION,
        ENUM_CURRENT_FREQUENCY_POSITION,
        ENUM_AUTO_SWITCHING_POSITION = 6,
        ENUM_RADIO_MENU_POSITION,
        ENUM_RADIO_MENU_STATE_NUMBER_POSITION,
        ENUM_RADIO_MEDIA_STATE_POSITION
};

enum{   ENUM_MANUAL = 0,
        ENUM_AUTO
};

enum{   ENUM_NOT_IN_MENU=0,
        ENUM_BEEP,
        ENUM_RE_CON,
        ENUM_DIS_CON,
        ENUM_REDIAL,
        ENUM_PHONEOOK,
        ENUM_MISSED_CALLS,
        ENUM_RECEIVED_CALLS,
        ENUM_RECENT_CALLS,
        ENUM_BASS,
        ENUM_TREBLE,
        ENUM_FADER,
        ENUM_BALANCE,
        ENUM_RDS,
        ENUM_PTY,
        ENUM_AF,
        ENUM_TA,
        ENUM_CT,
        ENUM_LOC,
        ENUM_SCAN,
        ENUM_12V,
        ENUM_PRUNE,
        ENUM_WELCOME,
        ENUM_SEE_YOU,
        ENUM_CLOCK_TIME,
        ENUM_CLOCK_1224,
        ENUM_RESET,
        ENUM_REGION,
        ENUM_EQ,
        ENUM_XBASS,
        ENUM_MUTE,
        ENUM_MENU_END   //must be last entry
};

typedef struct aupRadioHuState {
    uint8_t system_mode;
    uint8_t current_preset;
    union { //----------------- freq is unsigned integer, unit is KHz
        uint32_t freqReversedByteOrder; //MSB byte[0] LSB byte[3]
        struct {
            uint8_t freq_msb;
            uint8_t freq_mid2;
            uint8_t freq_mid1;
            uint8_t freq_lsb;
        } __attribute__((packed));
    } __attribute__((packed));
    struct {
        uint8_t dabFmOverride       : 1; //-- 1 = eg. DAB selected but listening to FM
        uint8_t seek                : 1; //- set 1 = "autoseek", clear 0 = manual seek
        uint8_t beepOnButtonPress   : 1;
        
        uint8_t ams_status          : 2;
        
        uint8_t autoseek            : 1;
        uint8_t manseek             : 1;
        
        uint8_t z                   : 1; //------- unused bits, patch to byte boundary
    } __attribute__((packed));
    uint8_t menu_position;
    uint8_t menu_state;
    union { //-------------- Update to v0.169 Protocol spec 26Sep2019
        uint16_t entire16bitValue;
        struct {
            uint8_t CDInserted      : 1;
            uint8_t USB1Inserted    : 1;
            uint8_t SDInserted      : 1;
            uint8_t AUX1Inserted    : 1;
            uint8_t AUX2Inserted    : 1;
            uint8_t RadioHasSignal  : 1;  // 
            uint8_t DABHasSignal    : 1;  // dab station tuned
            uint8_t XMHasSignal     : 1;
            //-------------------------------------------------------
            uint8_t CDHasHardware   : 1;
            uint8_t USB2Inserted    : 1;           
            uint8_t DABHasHardware  : 1;            
            uint8_t BTHasHardware   : 1;            
            uint8_t Reserved0       : 1;         
            uint8_t Reserved1       : 1;
            uint8_t Reserved2       : 1;
            uint8_t Reserved3       : 1;
        } __attribute__((packed));
    } __attribute__((packed));
} __attribute__((packed))
aupRadioHuState_t;

//------------------------------------ DAB Status Intent CMD_DAB_STATUS 14 0x1D
#define MAX_PROGRAM_TYPE_CHAR (29U)
#define MAX_DAB_SIGNAL_STRENGTH_CHAR (100U)

enum {  ENUM_NORMAL_STATE = 0,
        ENUM_SCAN_STATE   = 1,
        ENUM_SCAN_STATE_END //must be last entry
};

typedef struct aupDabStatus {
    uint8_t scan_state;
    union { //----------------- freq is unsigned integer, unit is KHz
        uint32_t freqReversedByteOrder; //MSB byte[0] LSB byte[3]
        struct {
            uint8_t freq_msb;
            uint8_t freq_mid2;
            uint8_t freq_mid1;
            uint8_t freq_lsb;
        } __attribute__((packed));
    } __attribute__((packed));
    uint8_t program_type;     //-------------- value range [0 ..  29]
    uint8_t signal_strength;  //-------------- value range [0 .. 100]
} __attribute__((packed))
aupDabStatus_t;

//------------------------------------------------ CMD_DAB_STATION_NAME 15 0x1E
#define FIXED_NUMBER_OF_SHORT_SERVICE_NAME_CHAR (8U)
#define FIXED_NUMBER_OF_LONG_SERVICE_NAME_CHAR (16U)

typedef struct aupDabStationName {
    uint8_t     short_service[FIXED_NUMBER_OF_SHORT_SERVICE_NAME_CHAR];
    uint8_t     long_service [FIXED_NUMBER_OF_LONG_SERVICE_NAME_CHAR] ;
    uint16_t    service_index;
} __attribute__((packed))
aupDabStationName_t;

//-------------------------------- DAB Ensemble Intent CMD_DAB_ENSEMBLE 16 0x1F
#define FIXED_NUMBER_OF_ENSEMBLE_NAME_CHAR (16U)

typedef struct aupDabEnsembleName {
    uint8_t ensembleName[FIXED_NUMBER_OF_ENSEMBLE_NAME_CHAR];
} __attribute__((packed))
aupDabEnsembleName_t;

//------------------------------ DAB DLS Information Intent CMD_DAB_DLS 17 0x20
// DLS string length inferred by terminator '\0'
#define MAX_DAB_DLS_INFO_CHAR (128U)

typedef struct aupDabDlsInfo {
    uint8_t dabDlsInfo[MAX_DAB_DLS_INFO_CHAR];
} __attribute__((packed))
aupDabDlsInfo_t;

//---------------------------------- EEPROM Data Intent CMD_EEPROM_DATA 19 0x22
//-------------------------------------------------------------------
// pageSelect parameter can assume one of these values
//-------------------------------------------------------------------
enum {  ENUM_CAN_BOARD_HW_SW_VERSION = 0,
        ENUM_MCU_BOARD_HW_SW_VERSION,
        ENUM_RADIO_MODEL,
        ENUM_RADIO_SERIAL_NUMBER,
        ENUM_CAN_BOARD_SERIAL_NUMBER,
        ENUM_RESERVED,
        ENUM_VIN_1_15,
        ENUM_VIN_16_17
};

//-------------------------------------------------------------------
// aupEepromCacheChanged_t is not part of the protocol spec. It is 
// added as a tool to manage EEPROM. 
//-------------------------------------------------------------------
typedef union aupEepromCacheChanged {
    uint8_t flags;
    struct {
        uint8_t canHwSwVer     : 1; // 0
        uint8_t radioHwSwVer   : 1; // 1
        uint8_t radioModel     : 1; // 2
        uint8_t radioSerialNum : 1; // 3
        uint8_t canSerialNum   : 1; // 4
        uint8_t reserved       : 1; // 5
        uint8_t vin_1_15       : 1; // 6 - Index is 1-based. LS-digit is digit-1
        uint8_t vin_16_17      : 1; // 7  As of 2018 VIN is known to be 17-digit
    } __attribute__((packed));
} __attribute__((packed))
aupEepromCacheChanged_t;

//-------------------------------------------------------------------
// Different from CAN PROTOCOL v 0.164, was missing page selector. 
// Fields in the union are to be interpreted as strings (ASCII).
//-------------------------------------------------------------------
typedef struct aupEeprom {
    struct {
        uint8_t numOfBytes : 4;
        uint8_t pageSelect : 4;
    } __attribute__((packed));
    union {
        struct { //---------------------- pageSelect=0, numOfBytes=15 or 0
            uint8_t can_sw_ver[8]; // bytes [1..8]
            uint8_t can_hw_ver[7]; // bytes [9..15]
        } __attribute__((packed));
        struct { //---------------------- pageSelect=1, numOfBytes=15 or 0
            uint8_t mcu_sw_ver[8]; // bytes [1..8]
            uint8_t mcu_hw_ver[7]; // bytes [9..15]
        } __attribute__((packed));
        uint8_t radio_model[15];         // pageSelect=2,  numOfBytes=0~15
        uint8_t radio_serial_number[15]; // pageSelect=3,  numOfBytes=0~15
        uint8_t can_serial_number[15];   // pageSelect=4,  numOfBytes=0~15
        uint8_t reserved[15];            // pageSelect=5,  numOfBytes=0~15
        uint8_t vin1_15[15];             // pageSelect=6,  numOfBytes=0~15
        uint8_t vin16_17[15];            // pageSelect=7,  numOfBytes=0~15
    } __attribute__((packed));
} __attribute__((packed))
aupEeprom_t;

//-------------------------- RTC Information Intent CMD_RTC_INFORMATION 20 0x23
#define MAX_HOUR_CHAR    (23U)
#define MAX_SEC_CHAR     (59U)
#define MAX_MINUTE       (59U)
#define MAX_DAY_CHAR     (31U)
#define MAX_MONTH_CHAR   (12U)
#define MAX_YEAR_CHAR    (99U) //---- should be able to accept 255
#define MIN_YEAR_CHAR    (17U) //---- should be able to accept   0
#define TIME_FORMAT_12HR (0U)
#define TIME_FORMAT_24HR (1U)

//-------------------------------------------------------------------
// simpleDate-t and simple_time are not directly specified in our 
// UART specification document. They are for clarity and convenience.
//-------------------------------------------------------------------
typedef union simpleDate {
    uint32_t entire24bitvalue : 24;
    struct{
        uint8_t day;
        uint8_t month;
        uint8_t year;
    } __attribute__((packed));
} __attribute__((packed))
simpleDate_t;

typedef union simpleTime {
    uint32_t entire24bitvalue : 24;
    struct{
        uint8_t hour       : 7;
        uint8_t timeFormat : 1;
        uint8_t minute     : 8;
        uint8_t second     : 8;
    } __attribute__((packed));
} __attribute__((packed))
simpletime_t;

typedef struct aupRtcInfo {
    union {
        struct { //--- this struct is for compatibility with old code
            uint8_t hour       : 7;
            uint8_t timeFormat : 1;
            uint8_t minute     : 8;
            uint8_t second     : 8;              
        } __attribute__((packed));
        simpletime_t time;
    } __attribute__((packed));
    union {
        struct { //--- this struct is for compatibility with old code
            uint8_t day;
            uint8_t month;
            uint8_t year;
        } __attribute__((packed));
        simpleDate_t date;
    } __attribute__((packed));
} __attribute__((packed))
aupRtcInfo_t;

//-------- Running Time Information Intent CMD_RUNNING_TIME_INFORMATION 21 0x24
typedef simpletime_t aupRunningTimeInfo_t;

//---------------------------- Media Information Intent CMD_MEDIA_STATE 22 0x25
#define MAX_MEDIA_TYPE_CHAR   (5U)   // was 4U, corrected
#define MAX_MEDIA_STATUS_CHAR (5U)   //na, play, pause, stop, ffw, rwd
#define MAX_MEDIA_VOLUME_CHAR (100U) //%

enum {  ENUM_NO_MEDIA_INFO    = 0,
        ENUM_SD_INFORMATION   = 1,
        ENUM_USB_INFORMATION  = 2,
        ENUM_CD_INFORMATION   = 3,
        ENUM_A2DP_INFORMATION = 4
};

enum {  ENUM_UNKNOWN     = 0,
        ENUM_MEDIA_PLAY  = 1,
        ENUM_MEDIA_PAUSE = 2,
        ENUM_MEDIA_STOP  = 3,
        ENUM_MEDIA_FFWD  = 4,
        ENUM_MEDIA_FRWD  = 5
};

typedef struct aupMediaInfo {
    uint8_t media_info_type;
    union {
        uint8_t total_file_number[2];  // compatibility with old code
        uint16_t numberOfFileLittleEndian;
    } __attribute__((packed));
    union {
        uint8_t current_file_number[2]; //compatibility with old code
        uint16_t currentFileLittleEndian;
    } __attribute__((packed));
    uint8_t play_status;
    union {
        struct { //--- this struct is for compatibility with old code
            uint8_t hour;
            uint8_t minute;
            uint8_t second;                
        } __attribute__((packed));
        aupRunningTimeInfo_t trackPlayingAt;
    } __attribute__((packed));
    simpletime_t trackLength;
    uint8_t volume_bt; //---- value range [0..100], unused [101..255]
} __attribute__((packed))
aupMediaInfo_t;

//------------------------------------------------------------------------------
// Radio station list enquiry - intent 0xd1
//------------------------------------------------------------------------------
typedef struct radio_station_enquiry
{
    uint8_t     type;   // 0x80 = DAB station list item
    uint16_t    n1;     // beginning position
    uint16_t    n2;     // number of entry / entries to send 
}__attribute__((packed))
radio_station_enquiry_0xD1_t;

//------------------------------------------------------------------------------
// Call history enquiry
//------------------------------------------------------------------------------
typedef struct call_history_enquiry
{
    uint8_t intent;
    uint8_t sub_intent;
    uint8_t storage;
    uint8_t navigation;
    // uint32_t extended_info   // for future use
    
}__attribute__((packed))
call_history_enquiry_0x94_0x1B_t;

//------------------------------- Radio_HU_MCU inititated CMD_HANDSHAKE 23 0xA0
#define HANDSHAKE_DATA_0 (0x55)
#define HANDSHAKE_DATA_1 (0xAA)
#define HANDSHAKE_VOIDED (0x00)

typedef struct aupHandshake1 { //---- aupHandshake1 is MCU initiated ()
    uint8_t byte0;
    uint8_t byte1;
} __attribute__((packed))
aupHandshake1_t;

//----------------- ACKnowledge reception of UART message CMD_REPLY_ACK 24 0x02
#define SUCCESS                     (0x00U) // Also used to resume from pause
#define ERROR_UNKNOWN_CMDID         (0x01U) // Unknown intent
#define ERROR_PARAMETER             (0x02U) // Inconsistent message length
#define ERROR_CRC                   (0x03U) // ** Undefined
#define ERROR_DATA_NOT_AVAILABLE    (0x04U) // Requested data not available
#define ERROR_FLASH_PAGE_EMPTY      (0x05U) // Flash page is empty
#define ERROR_FLASH_PAGE_NOT_EMPTY  (0x06U) // Flash page not empty
#define ERROR_CAN_CHANNEL_BUSY      (0x07U) // ** Undefined
#define ERROR_MESSAGE_LENGTH        (0x08U) // Browse navigation not available
#define ERROR_CAN_CHANNEL_CANCEL    (0x09U) // Value out of range
#define ERROR_SYSTEM_BUSY           (0x0AU) // system busy used by 0xD6 0x56
#define ERROR_DAB_STORE_FAIL        (0x0BU)

//---------------------------------------------------- CMD_REQUEST_DATA 25 0xB0
typedef struct aupBtPbabMediaAddress {
    uint8_t source;
    uint8_t type;
    uint8_t dataAddress[63];
} __attribute__((packed))
aupBtPbapMediaAddress_t;

//----------- Software versions used in the radio CMD_SOFTWARE_VERSIONS 27 0x27
typedef struct aupSoftwareVersions {
    uint8_t numberAscii[FIXED_SOFTWARE_CHAR];
} __attribute__((packed))
aupSoftwareVersions_t;

//----------------------------- BT Telephone Module Name CMD_HU_BT_NAME 28 0x28
#define MAX_HU_BT_NAME_CHAR (0x18)

typedef struct aupHuBtName {
    uint8_t huBtNameAscii[MAX_HU_BT_NAME_CHAR];
} __attribute__((packed))
aupHuBtName_t;

//------------------------------------------------------------------------------
// Hardware flags for Intent 0x29
//------------------------------------------------------------------------------
typedef struct aupHuHwCapabilities 
{
    union
    {
        struct 
        {
            uint8_t fm        : 1; // bit 0
            uint8_t am        : 1; // bit 1
            uint8_t sw        : 1; // bit 2
            uint8_t lw        : 1; // bit 3
            uint8_t wb        : 1; // bit 4
            uint8_t dab       : 1; // bit 5
            uint8_t sxm       : 1; // bit 6       
            uint8_t unused    : 1; // bit 7
        } __attribute__((packed));
        uint8_t byte; 
    } __attribute__((packed))   
    built_in_A;
    
    union 
    {        
        struct {
            uint8_t usb_front : 1; // bit 0
            uint8_t usb_rear  : 1; // bit 1
            uint8_t aux_front : 1; // bit 2
            uint8_t aux_rear  : 1; // bit 3
            uint8_t bt        : 1; // bit 4
            uint8_t cd        : 1; // bit 5
            uint8_t mfi       : 1; // bit 6
            uint8_t can       : 1; // bit 7
        } __attribute__((packed));
        uint8_t byte; 
    }__attribute__((packed))   
    built_in_B;
    
    union
    {
        struct 
        {
            uint8_t fm        : 1; // bit 0
            uint8_t am        : 1; // bit 1
            uint8_t sw        : 1; // bit 2
            uint8_t lw        : 1; // bit 3
            uint8_t wb        : 1; // bit 4
            uint8_t dab       : 1; // bit 5
            uint8_t sxm       : 1; // bit 6       
            uint8_t hfp_en    : 1; // bit 7
        } __attribute__((packed));
        uint8_t byte; 
    }__attribute__((packed))   
    enabled_A;

    union 
    {
        struct 
        {
            uint8_t usb_front : 1; // bit 0
            uint8_t usb_rear  : 1; // bit 1
            uint8_t aux_front : 1; // bit 2
            uint8_t aux_rear  : 1; // bit 3
            uint8_t bt_power  : 1; // bit 4
            uint8_t cd        : 1; // bit 5
            uint8_t mfi       : 1; // bit 6
            uint8_t can       : 1; // bit 7
        } __attribute__((packed));
        uint8_t byte; 
    }__attribute__((packed))   
    enabled_B;
    
    union 
    {
        struct 
        {
            uint8_t fm        : 1; // bit 0
            uint8_t am        : 1; // bit 1
            uint8_t sw        : 1; // bit 2
            uint8_t lw        : 1; // bit 3
            uint8_t wb        : 1; // bit 4
            uint8_t dab       : 1; // bit 5
            uint8_t sxm       : 1; // bit 6       
            uint8_t unused    : 1; // bit 7
        } __attribute__((packed));
        uint8_t byte; 
    }__attribute__((packed))   
    runtime_A;

    union
    {
        struct 
        {
            uint8_t usb_front : 1; // bit 0
            uint8_t usb_rear  : 1; // bit 1
            uint8_t cd        : 1; // bit 2
            uint8_t mfi       : 1; // bit 3
            uint8_t bt_spp    : 1; // bit 4
            uint8_t bt_avrcp  : 1; // bit 5
            uint8_t bt_a2dp   : 1; // bit 6
            uint8_t bt_hfp    : 1; // bit 7
        } __attribute__((packed));
        uint8_t byte; 
    }__attribute__((packed))   
    runtime_B;
    
} __attribute__((packed))
aupHuHwCapabilities_t;

typedef struct Full_
        {                                           // size
            uint8_t     type;                       // 1
            uint16_t    n1;                         // 2
            uint16_t    n0;                         // 2    
            uint32_t    id;                         // 4
            uint8_t     short_service_name[8];      // 8
            uint8_t     extended_information[256];  // 256
        } __attribute__((packed))           // total = 273
        Full_t;
        
typedef struct
        {
            uint8_t     type;                       // 1
            uint16_t    n1;                         // 2
            uint16_t    n0;                         // 2
            uint32_t    service_id;                 // 4
            uint8_t     short_service_name[8];      // 8
            uint16_t    service_index;              // 2
            uint8_t     extended_information[254];  // 254 
            
            
        } __attribute__((packed))
        DAB_t;   
      
typedef struct radioStation 
{
    union 
    {
        Full_t Full;
        DAB_t  DAB;
    };
} __attribute__((packed)) radioStation_0x51_t;

//------------------------------------------------------------------------------
// Intent 0x56 - reply to a 0xD6 intent enquiry
//------------------------------------------------------------------------------
typedef struct total_count_0x56
{
    uint8_t     source;
    uint16_t    folder_count;
    uint16_t    file_count;
    
} __attribute__((packed))
total_count_0x56_t;

//------------------------------------------------------------------------------
// Intent 0x54 - reply to a D4 enquiry (BT device info)
//------------------------------------------------------------------------------
typedef struct BT_device_0x54
{
    uint8_t     index;          // of current item
    uint8_t     total;          // 0 means empty list
    uint8_t     paired_profile; // reserved
    uint8_t     connection_profile;
    uint8_t     extended_info[250];
    
} __attribute__((packed)) 
BT_device_t;

//------------------------------------------------------------------------------
// Intent 0x5A - Text (DLS, RT etc)
//------------------------------------------------------------------------------
typedef struct text_0x5A
{
    uint8_t     type;
    uint8_t     n1;     // current segment number
    uint8_t     n2;     // total segment count
    uint8_t     s[128]; // max text size
} 
text_0x5A_t;

//------------------------------------------------------------------------------
// Intent 0x59 LIST_INVALIDATION
//------------------------------------------------------------------------------
typedef struct List_Invalidation
{
    uint8_t     lists;    
} __attribute__((packed))
LIST_INVALIDATION_t;

//------------------------------------------------------------------------------
// Intent 0x2B  BT Extended Information - callers name
//------------------------------------------------------------------------------
typedef struct BT_Extended_info_0x2B
{
    uint8_t     attribute;  //0x0a = name of contact. Others for future use
    uint8_t     data[200];
}
BT_Extended_info_0x2B_t;








#ifdef __cplusplus
extern "C" {
#endif

// Place variables and etc here

#ifdef __cplusplus
} // extern "C"
#endif

#endif  /*  __RM200X_FRAME_DEFINITIONS_H__  */
