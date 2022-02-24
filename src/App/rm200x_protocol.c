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
/** Print the value os a json object or array.
  * @param json The handler of the json object or array. */
static void json_dump( json_t const* json ) 
{
    jsonType_t const type = json_getType( json );
    if ( type != JSON_OBJ && type != JSON_ARRAY ) 
    {
        ESP_LOGE(PROTOCOL_TAG, "BAD JSON");
        return;
    }

    printf("%s", type == JSON_OBJ? " {": " [" );

    json_t const* child;
    for( child = json_getChild( json ); child != 0; child = json_getSibling( child ) ) 
    {

        jsonType_t propertyType = json_getType( child );
        char const* name = json_getName( child );
        if ( name ) printf(" \"%s\": ", name );

        if ( propertyType == JSON_OBJ || propertyType == JSON_ARRAY )
            json_dump( child );

        else 
        {
            char const* value = json_getValue( child );
            if ( value ) 
            {
                bool const text = JSON_TEXT == json_getType( child );
                char const* fmt = text? " \"%s\"": " %s";
                printf( fmt, value );
                bool const last = !json_getSibling( child );
                if ( !last ) printf(",");
            }
        }
    }

    printf("%s", type == JSON_OBJ? " }": " ]" );
    printf("\r\n");
}

/******************************************************************************************/
// PUBLIC FUNCTIONS - STATUS MESSAGES - OUTPUTS (from the radio)
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
    int vol = 0;
    int bass = 0;
    int middle = 0;
    int treble = 0;
    int balance = 0;
    int fader = 0;
    bool mute = false;
    bool loudness = false;
    bool smute = false;

    ESP_LOG_BUFFER_HEXDUMP(PROTOCOL_TAG, p_str, strlen((char *)p_str), ESP_LOG_INFO);

    // Setup a memory store for the parsed JSON string
    json_t mem[JSON_MAX_NO_TOKENS]; // Number of tokens

    json_t const* json = json_create( p_str, mem, sizeof mem / sizeof *mem );
    if ( !json ) 
    {
        return 0xFF;
    }

    json_dump(json);

     json_t const* Audio = json_getProperty( json, "0x10" );
 
    if ( Audio && json_getType(Audio) == JSON_OBJ ) 
    {
        // This an Audio message - decode it as such:

        //INTEGER VALUES
        json_t const *Volume = json_getProperty( Audio, "Volume" );
        if ( Volume && json_getType(Volume) == JSON_INTEGER )
        {
            vol = (int)json_getInteger( Volume );
            ESP_LOGI(PROTOCOL_TAG, "Volume \t: %d", vol);
        }
        
        //BOOLEAN VALUES
        json_t const *Mute = json_getProperty( Audio, "Mute" );
        if ( Mute && json_getType(Mute) == JSON_BOOLEAN )
        {
            mute = (int)json_getBoolean( Mute );
            ESP_LOGI(PROTOCOL_TAG, "Mute \t: %s", mute ? "true" : "false");
        }
    }

     json_t const* Tone = json_getProperty( json, "0x11" );
 
    if ( Tone && json_getType(Tone) == JSON_OBJ ) 
    {
        // This an Audio message - decode it as such:

        //INTEGER VALUES
        json_t const *Bass = json_getProperty( Tone, "Bass" );
        if ( Bass && json_getType(Bass) == JSON_INTEGER )
        {
            bass = (int)json_getInteger( Bass );
            ESP_LOGI(PROTOCOL_TAG, "Bass \t: %d", bass);
        }
        json_t const *Middle = json_getProperty( Tone, "Middle" );
        if ( Middle && json_getType(Middle) == JSON_INTEGER )
        {
            middle = (int)json_getInteger( Middle );
            ESP_LOGI(PROTOCOL_TAG, "Middle \t: %d", middle);
        }
        json_t const *Treble = json_getProperty( Tone, "Treble" );
        if ( Treble && json_getType(Treble) == JSON_INTEGER )
        {
            treble = (int)json_getInteger( Treble );
            ESP_LOGI(PROTOCOL_TAG, "Treble \t: %d", treble);
        }
        json_t const *Balance = json_getProperty( Tone, "Balance" );
        if ( Balance && json_getType(Balance) == JSON_INTEGER )
        {
            balance = (int)json_getInteger( Balance );
            ESP_LOGI(PROTOCOL_TAG, "Balance \t: %d", balance);
        }
        json_t const *Fader = json_getProperty( Tone, "Fader" );
        if ( Fader && json_getType(Fader) == JSON_INTEGER )
        {
            fader = (int)json_getInteger( Fader );
            ESP_LOGI(PROTOCOL_TAG, "Fader \t: %d", fader);
        }

        //BOOLEAN VALUES
        json_t const *SMute = json_getProperty( Tone, "SoftMute" );
        if ( SMute && json_getType(SMute) == JSON_BOOLEAN )
        {
            smute = (int)json_getBoolean( SMute );
            ESP_LOGI(PROTOCOL_TAG, "SoftMute \t: %s", smute ? "true" : "false");
        }
        json_t const *Loudness = json_getProperty( Tone, "Loudness" );
        if ( Loudness && json_getType(Loudness) == JSON_BOOLEAN )
        {
            loudness = (int)json_getBoolean( Loudness );
            ESP_LOGI(PROTOCOL_TAG, "Loudness \t: %s", loudness ? "true" : "false");
        } 
    }




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

    // build and send frame
    //static uint8_t _dat[2];
    //static uint8_t *pt_dat = _dat;
    _dat[0] = 0x02;
    _dat[1] = mute ? 1 : 0;
    
    //uint8_t err = 
    CreateSendFrame(0x90, _dat, 4);

    if(err)
    {
        ESP_LOGE(PROTOCOL_TAG, "BAD FRAME - Error code: 0x%02X", err);
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