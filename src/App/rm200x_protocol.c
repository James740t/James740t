#include "App/rm200x_protocol.h"

//Task Tags
const char *PROTOCOL_TAG = "PROTOCOL";

/******************************************************************************************/
// UART PROTOCOL - JSON
/******************************************************************************************/

//Frame store data
aupAudio_t              intent_0x10_data;
aupEQ_t                 intent_0x11_data;
aupPowerState_t         intent_0x21_data;

//LOCAL PROTOTYPES

/******************************************************************************************/
// HELPERS
/******************************************************************************************/

/******************************************************************************************/
// PUBLIC FUNCTIONS
/******************************************************************************************/
char* intent_0x10_json(char *pt_str_json, uint8_t *frame)
{
    //Check the frame intent is OK
    if (frame[4] == CMD_AUDIO)
    {
        //aupAudio_t
        memset(&intent_0x10_data, 0x00, sizeof(aupAudio_t));
        uint8_t data_0x10 = frame[5];
        intent_0x10_data.volume = data_0x10 & 0b01111111;
        intent_0x10_data.mute = (bool)(data_0x10 & 0b10000000);

        // JSON and get ready for MQTT
        size_t length = JSON_STRING_LENGTH;
        pt_str_json = json_objOpen(pt_str_json, "0x10", &length);                           // --> {\0
        pt_str_json = json_int(pt_str_json, "Volume", intent_0x10_data.volume, &length);    // --> {"Volume":22\0
        pt_str_json = json_bool(pt_str_json, "Mute", intent_0x10_data.mute, &length);       // --> {"Volume":22,"Mute":0\0
        pt_str_json = json_objClose(pt_str_json, &length);                                  // --> {"Volume":22,"Mute":0}\0
        pt_str_json = json_end(pt_str_json, &length);
    }
    
    return pt_str_json;
}

char* intent_0x11_json(char *pt_str_json, uint8_t *frame)
{
    // Check the frame intent is OK
    if (frame[4] == CMD_EQ)
    {
        //aupEQ_t
        memset(&intent_0x11_data, 0x00, sizeof(aupEQ_t));
        intent_0x11_data.bass = frame[7] - 10;
        intent_0x11_data.middle = frame[8] - 10;
        intent_0x11_data.treble = frame[9] - 10;
        intent_0x11_data.fade = frame[5] - 10;
        intent_0x11_data.balance = frame[6] - 10;
        intent_0x11_data.eq_mode = (frame[10] & 0b00111111);
        intent_0x11_data.loudness = (bool)(frame[10] & 0b10000000);
        intent_0x11_data.soft_mute = (bool)(frame[10] & 0b01000000);

        size_t length = JSON_STRING_LENGTH;
        pt_str_json = json_objOpen(pt_str_json, "0x11", &length);
        pt_str_json = json_int(pt_str_json, "Bass", intent_0x11_data.bass, &length);
        pt_str_json = json_int(pt_str_json, "Mid", intent_0x11_data.middle, &length);
        pt_str_json = json_int(pt_str_json, "Treble", intent_0x11_data.treble, &length);
        pt_str_json = json_int(pt_str_json, "Fader", intent_0x11_data.fade, &length);
        pt_str_json = json_int(pt_str_json, "Balance", intent_0x11_data.balance, &length);
        pt_str_json = json_int(pt_str_json, "EQ Mode", intent_0x11_data.eq_mode, &length);
        pt_str_json = json_bool(pt_str_json, "Loudness", intent_0x11_data.loudness, &length);
        pt_str_json = json_bool(pt_str_json, "SoftMute", intent_0x11_data.soft_mute, &length);
        pt_str_json = json_objClose(pt_str_json, &length);
        pt_str_json = json_end(pt_str_json, &length);
    }
    return pt_str_json;
}

char *intent_0x21_json(char *pt_str_json, uint8_t *frame)
{
    // Check the frame intent is OK
    if (frame[4] == CMD_POWER_STATE)
    {
        //aupPowerState_t 
        intent_0x21_data.accOn = (bool)(frame[5] & 0b00000001);
        intent_0x21_data.powerOn = (bool)(frame[5] & 0b00000010);
        intent_0x21_data.sleep = (bool)(frame[5] & 0b00000100);

        // JSON and get ready for MQTT
        size_t length = JSON_STRING_LENGTH;
        pt_str_json = json_objOpen(pt_str_json, "0x21", &length);
        pt_str_json = json_bool(pt_str_json, "Power", intent_0x21_data.powerOn, &length);
        pt_str_json = json_bool(pt_str_json, "Ignition", intent_0x21_data.accOn, &length);
        pt_str_json = json_bool(pt_str_json, "Sleep", intent_0x21_data.sleep, &length);
        pt_str_json = json_objClose(pt_str_json, &length);
        pt_str_json = json_end(pt_str_json, &length);
    }

    return pt_str_json;
}


/******************************************************************************************/
// COMMAND INPUTS - PUBLIC FUNCTIONS
/******************************************************************************************/

uint8_t json_volume_set(char *p_str)
{
    json_t mem[32]; // Number of tokens
    json_t const* json = json_create( p_str, mem, sizeof mem / sizeof *mem );
    if ( !json ) 
    {
        return 0xFF;
    }

    json_t const* Volume = json_getProperty( json, "Volume" );
    if ( !Volume || JSON_INTEGER != json_getType( Volume ) ) 
    {
        return 0xFF;
    }
    int const vol = (int)json_getInteger( Volume );

    // build and send frame
    static uint8_t _dat[2];
    //static uint8_t *pt_dat = _dat;
    _dat[0] = (uint8_t)0x09;
    _dat[1] = (uint8_t)vol;
    
    uint8_t err = CreateSendFrame(0x90, _dat, 4);

    if(err)
    {
        ESP_LOGE(PROTOCOL_TAG, "BAD FRAME - Error code: 0x%02X", err);
        ESP_LOG_BUFFER_HEXDUMP(PROTOCOL_TAG, p_str, strlen((char *)p_str), ESP_LOG_ERROR);
    }

///////////////////////////////////////////////////////////////////////////////////////////////////
    json_t const* Mute = json_getProperty( json, "Mute" );
    if ( !Mute || JSON_BOOLEAN != json_getType( Mute ) ) 
    {
        return 0xFF;
    }
    bool const mut = (bool)json_getBoolean( Mute );

    // build and send frame
    //static uint8_t _dat[2];
    //static uint8_t *pt_dat = _dat;
    _dat[0] = 0x02;
    _dat[1] = mut ? 1 : 0;
    
    //uint8_t err = 
    CreateSendFrame(0x90, _dat, 4);

    if(err)
    {
        ESP_LOG_BUFFER_HEXDUMP(PROTOCOL_TAG, p_str, strlen((char *)p_str), ESP_LOG_ERROR);
    }
//////////////////////////////////////////////////////////////////////////////////////////////////////


    // json_t const* age = json_getProperty( json, "age" );
    // if ( !age || JSON_INTEGER != json_getType( age ) ) 
    // {
    //     return -1;
    // }
    // int const ageVal = (int)json_getInteger( age );
    // printf( "Age: %d.\n", ageVal );


    return err;
}

uint8_t json_mute_set(char *p_str)
{
    json_t mem[32]; // Number of tokens
    json_t const* json = json_create( p_str, mem, sizeof mem / sizeof *mem );
    if ( !json ) 
    {
        return 0xFF;
    }

    json_t const* Mute = json_getProperty( json, "Mute" );
    if ( !Mute || JSON_BOOLEAN != json_getType( Mute ) ) 
    {
        return 0xFF;
    }
    bool const mut = (bool)json_getBoolean( Mute );

    // build and send frame
    static uint8_t _dat[2];
    //static uint8_t *pt_dat = _dat;
    _dat[0] = 0x02;
    _dat[1] = mut ? 1 : 0;
    
    uint8_t err = CreateSendFrame(0x90, _dat, 4);

    if(err)
    {
        ESP_LOG_BUFFER_HEXDUMP(PROTOCOL_TAG, p_str, strlen((char *)p_str), ESP_LOG_ERROR);
    }

    return err;
}
/******************************************************************************************/
// UART PROTOCOL - JSON -- END --
/******************************************************************************************/