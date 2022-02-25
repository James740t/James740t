#include "App/rm200x_protocol.h"

//Task Tags
const char *PROTOCOL_TAG = "PROTOCOL";
const char *RADIO_STATUS_TASK_TAG = "RADIO_STATUS";
const char *RADIO_COMMAND_TASK_TAG = "RADIO_COMMAND";

//FREERTOS CONTROL ITEMS
TaskHandle_t xHandle_Audio_in_task = NULL;

#define STACK_MONITOR   false
#if STACK_MONITOR
UBaseType_t uxHighWaterMark_RX;
UBaseType_t uxHighWaterMark_TX;
UBaseType_t uxHighWaterMark_EV;
#endif

        // #if STACK_MONITOR
        //     /* Inspect our own high water mark on entering the task. */
        //     uxHighWaterMark_RX = uxTaskGetStackHighWaterMark( NULL );
        //     printf("MQTT RX STACK HW (START) = %d\r\n", uxHighWaterMark_RX);
        // #endif

        // #if STACK_MONITOR
        //     uxHighWaterMark_RX = uxTaskGetStackHighWaterMark( NULL );
        //     printf("MQTT RX STACK HW (RUN): %d\r\n", uxHighWaterMark_RX);
        // #endif


//Local prototypes


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

void mqtt_send_json(const char *mqtt_text)
{
    static mqtt_esp_message_t _mqtt_msg;
    static mqtt_esp_message_t *mqtt_msg = &_mqtt_msg;
    memset(mqtt_msg, 0x00, sizeof(mqtt_esp_message_t));

    // TEXT VERSION:
    // start with a clean structure
    memset(mqtt_msg, 0x00, sizeof(mqtt_esp_message_t));
    // start building
    mqtt_msg->msg_id = 0; // set counter on send in Tx task, set to zero for now
    mqtt_msg->qos = 0;    // set required QoS level here
    // set the topic here
    mqtt_msg->topic_len = build_topic((mqtt_topic_t *)&mqtt_msg->topic_detail, MQTT_TELE_PREFIX, MQTT_BASE_TOPIC, MQTT_RADIO_SUFFIX);
    strcpy((char *)&mqtt_msg->topic, (char *)&mqtt_msg->topic_detail.topic);
    mqtt_msg->data_len = strlen(mqtt_text);
    mqtt_msg->total_data_len = mqtt_msg->data_len;
    memcpy(&mqtt_msg->data, (char *)mqtt_text, mqtt_msg->data_len);

    // send pointer to structure of the outgoing message
    xQueueSend(xqMQTT_tx_Messages, mqtt_msg, (TickType_t)0);
    // Flash LED to show activity
    xSemaphoreGive(bin_s_sync_blink_task);

    ESP_LOGW(RADIO_STATUS_TASK_TAG, "MQTT Message: %s/%s - data bytes = %d", mqtt_msg->topic, mqtt_msg->data, mqtt_msg->data_len);
}

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
char *intent_0x10_json(char *pt_str_json, uint8_t *frame)
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
        size_t length = JSON_TX_STRING_LENGTH;
        pt_str_json = json_objOpen(pt_str_json, "0x10", &length);                           // --> {\0
        pt_str_json = json_int(pt_str_json, "Volume", intent_0x10_data.volume, &length);    // --> {"Volume":22\0
        pt_str_json = json_bool(pt_str_json, "Mute", intent_0x10_data.mute, &length);       // --> {"Volume":22,"Mute":0\0
        pt_str_json = json_objClose(pt_str_json, &length);                                  // --> {"Volume":22,"Mute":0}\0
        pt_str_json = json_end(pt_str_json, &length);
    }
    
    return pt_str_json;
}

char *intent_0x11_json(char *pt_str_json, uint8_t *frame)
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

        size_t length = JSON_TX_STRING_LENGTH;
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
        size_t length = JSON_TX_STRING_LENGTH;
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
// PUBLIC FUNCTIONS - STATUS MESSAGES - INPUTS (from the Control Application)
/******************************************************************************************/

uint8_t intent_0x10_command(json_t const *json_to_decode)
{
    // Initialise
    uint8_t err = 0;
    static uint8_t _dat[2];

    int vol = 0;
    bool mute = false;

    // Decode
    // INTEGER VALUES
    json_t const *Volume = json_getProperty(json_to_decode, "Volume");
    if (Volume && json_getType(Volume) == JSON_INTEGER)
    {
        vol = (int)json_getInteger(Volume);
        ESP_LOGI(PROTOCOL_TAG, "Volume \t: %d", vol);

        // build and send VOLUME COMMAND frame
        _dat[0] = (uint8_t)K_VOLUME;
        _dat[1] = (uint8_t)vol;

        err += CreateSendFrame(0x90, _dat, 4);
    }

    // BOOLEAN VALUES
    json_t const *Mute = json_getProperty(json_to_decode, "Mute");
    if (Mute && json_getType(Mute) == JSON_BOOLEAN)
    {
        mute = (int)json_getBoolean(Mute);
        ESP_LOGI(PROTOCOL_TAG, "Mute \t: %s", mute ? "true" : "false");

        // build and send frame
        _dat[0] = K_MUTE;
        _dat[1] = (uint8_t)(mute ? 1 : 0);

        err += CreateSendFrame(0x90, _dat, 4);
    }

    if(err)
    {
        ESP_LOGE(PROTOCOL_TAG, "BAD FRAME - Error code: 0x%02X", err);
    }

    // Return

    return err;
}
uint8_t intent_0x11_command(json_t const *json_to_decode)
{
    // Initialise
    uint8_t err = 0;
    static uint8_t _dat[2];

    int bass = 0;
    int middle = 0;
    int treble = 0;
    int balance = 0;
    int fader = 0;
    bool loudness = false;
    bool smute = false;

    // Decode
    // INTEGER VALUES
    json_t const *Bass = json_getProperty(json_to_decode, "Bass");
    if (Bass && json_getType(Bass) == JSON_INTEGER)
    {
        bass = (int)json_getInteger(Bass);
        ESP_LOGI(PROTOCOL_TAG, "Bass \t: %d", bass);

        // build and send TONE COMMAND frame
        _dat[0] = (uint8_t)K_BASS;
        _dat[1] = (uint8_t)(bass + 10);
        err += CreateSendFrame(0x90, _dat, 4);
    }
    json_t const *Middle = json_getProperty(json_to_decode, "Middle");
    if (Middle && json_getType(Middle) == JSON_INTEGER)
    {
        middle = (int)json_getInteger(Middle);
        ESP_LOGI(PROTOCOL_TAG, "Middle \t: %d", middle);

        // build and send TONE COMMAND frame
        _dat[0] = (uint8_t)K_MIDDLE;
        _dat[1] = (uint8_t)(middle + 10);
        err += CreateSendFrame(0x90, _dat, 4);
    }
    json_t const *Treble = json_getProperty(json_to_decode, "Treble");
    if (Treble && json_getType(Treble) == JSON_INTEGER)
    {
        treble = (int)json_getInteger(Treble);
        ESP_LOGI(PROTOCOL_TAG, "Treble \t: %d", treble);

        // build and send TONE COMMAND frame
        _dat[0] = (uint8_t)K_TREBLE;
        _dat[1] = (uint8_t)(treble + 10);
        err += CreateSendFrame(0x90, _dat, 4);
    }
    json_t const *Balance = json_getProperty(json_to_decode, "Balance");
    if (Balance && json_getType(Balance) == JSON_INTEGER)
    {
        balance = (int)json_getInteger(Balance);
        ESP_LOGI(PROTOCOL_TAG, "Balance \t: %d", balance);

        // build and send TONE COMMAND frame
        _dat[0] = (uint8_t)K_BALANCE;
        _dat[1] = (uint8_t)(balance + 10);
        err += CreateSendFrame(0x90, _dat, 4);
    }
    json_t const *Fader = json_getProperty(json_to_decode, "Fader");
    if (Fader && json_getType(Fader) == JSON_INTEGER)
    {
        fader = (int)json_getInteger(Fader);
        ESP_LOGI(PROTOCOL_TAG, "Fader \t: %d", fader);

        // build and send TONE COMMAND frame
        _dat[0] = (uint8_t)K_FADE;
        _dat[1] = (uint8_t)(fader + 10);
        err += CreateSendFrame(0x90, _dat, 4);
    }

    // BOOLEAN VALUES
    json_t const *SMute = json_getProperty(json_to_decode, "SoftMute");
    if (SMute && json_getType(SMute) == JSON_BOOLEAN)
    {
        smute = (int)json_getBoolean(SMute);
        ESP_LOGI(PROTOCOL_TAG, "SoftMute \t: %s", smute ? "true" : "false");

        // build and send TONE COMMAND frame
        _dat[0] = (uint8_t)K_RADIO_SOFT_MUTE;
        _dat[1] = (uint8_t)(smute ? 1 : 0);
        err += CreateSendFrame(0x90, _dat, 4);

    }
    json_t const *Loudness = json_getProperty(json_to_decode, "Loudness");
    if (Loudness && json_getType(Loudness) == JSON_BOOLEAN)
    {
        loudness = (int)json_getBoolean(Loudness);
        ESP_LOGI(PROTOCOL_TAG, "Loudness \t: %s", loudness ? "true" : "false");

        // build and send TONE COMMAND frame
        _dat[0] = (uint8_t)K_LOUDNESS;
        _dat[1] = (uint8_t)(loudness ? 1 : 0);
        err += CreateSendFrame(0x90, _dat, 4);
    }

    if(err)
    {
        ESP_LOGE(PROTOCOL_TAG, "BAD FRAME/S - Error code: 0x%02X", err);
    }

    // Return

    return err;
}
uint8_t intent_0x21_command(json_t const *json_to_decode)
{
    // Initialise
    uint8_t err = 0;
    static uint8_t _dat[2];

    bool power = false;

    // Decode
    // BOOLEAN VALUES
    json_t const *Power = json_getProperty(json_to_decode, "Power");
    if (Power && json_getType(Power) == JSON_BOOLEAN)
    {
        power = (int)json_getBoolean(Power);
        ESP_LOGI(PROTOCOL_TAG, "Power \t: %s", power ? "true" : "false");

        // build and send frame
        _dat[0] = K_POWER;
        _dat[1] = (uint8_t)(power ? 2 : 1);

        err += CreateSendFrame(0x90, _dat, 4);
    }

    if(err)
    {
        ESP_LOGE(PROTOCOL_TAG, "BAD FRAME - Error code: 0x%02X", err);
    }

    // Return

    return err;
}

/******************************************************************************************/
// COMMAND INPUTS - PUBLIC FUNCTIONS
/******************************************************************************************/

uint8_t protocol_command_json_process(char *p_str)
{
    uint8_t err = 0;

    ESP_LOG_BUFFER_HEXDUMP(PROTOCOL_TAG, p_str, strlen((char *)p_str), ESP_LOG_INFO);

    // Setup a memory store for the parsed JSON string
    json_t mem[JSON_MAX_NO_TOKENS]; // Number of tokens
 
    // Parse
    json_t const* json = json_create( p_str, mem, sizeof mem / sizeof *mem );
    if ( !json ) 
    {
        // return if parsing fails
        return 0xFF;
    }

    // Show recovered JSON (comment out for production code)
    json_dump(json);

    // Collect individual JSONs - CHILD (s) at the top level 
    // this translates into different protocol intents
    json_t const *child;
    for (child = json_getChild(json); child != 0; child = json_getSibling(child))
    {
        char const *name = json_getName(child);
  
        ESP_LOGW(PROTOCOL_TAG, "NAME \t: %s", name);

        if (strcmp(name, "0x10") == 0)
        {
            err += intent_0x10_command(child);
        }
        else if (strcmp(name, "0x11") == 0)
        {
            err += intent_0x11_command(child);
        }
        else if (strcmp(name, "0x21") == 0)
        {
            err += intent_0x21_command(child);
        }
    }

    return err;
}


/******************************************************************************************/
// PROTOCOL TASK
/******************************************************************************************/

void protocol_rm200x_input_task(void *arg)
{
#if STACK_MONITOR
    /* Inspect our own high water mark on entering the task. */
    uxHighWaterMark_TX = uxTaskGetStackHighWaterMark(NULL);
    printf("AUDIO IN STACK HW (START) = %d\r\n", uxHighWaterMark_TX);
#endif

    // Initialise things here
    static uint8_t _frame_in_buffer[FRAME_BUFFER_SIZE];
    static uint8_t *frame_in_buffer = _frame_in_buffer;
    memset(frame_in_buffer, 0x00, sizeof(_frame_in_buffer));

    static char _json_buffer[JSON_TX_STRING_LENGTH];
    static char *json_buffer = _json_buffer;
    memset(json_buffer, 0x00, sizeof(_json_buffer));

    // Task loop
    while (1)
    {
        // Clear stuff here ready for a new input message
        memset(frame_in_buffer, 0x00, sizeof(_frame_in_buffer));
        memset(json_buffer, 0x00, sizeof(_json_buffer));

        // Block until a message is put on the queue
        if (xQueueReceive(xqFrame_Process, frame_in_buffer, (TickType_t)portMAX_DELAY) == pdPASS)
        {
            ESP_LOG_BUFFER_HEXDUMP(RADIO_STATUS_TASK_TAG, frame_in_buffer, Get_Frame_Length(frame_in_buffer), ESP_LOG_INFO);
            uint8_t intent = Get_Frame_Intent(frame_in_buffer);

            switch (intent)
            {
                case CMD_AUDIO:
                {
                    // Process and send out to IoT via MQTT
                    intent_0x10_json(json_buffer, frame_in_buffer);
                    mqtt_send_json(json_buffer);
                    break;
                }
                case CMD_EQ:
                {
                    // Process and send out to IoT via MQTT
                    intent_0x11_json(json_buffer, frame_in_buffer);
                    mqtt_send_json(json_buffer);
                    break;
                }
                case CMD_POWER_STATE:
                {
                    // Process and send out to IoT via MQTT
                    intent_0x21_json(json_buffer, frame_in_buffer);
                    mqtt_send_json(json_buffer);
                    break;
                }

                default:
                {
                    break;
                }
            }


#if STACK_MONITOR
            uxHighWaterMark_TX = uxTaskGetStackHighWaterMark(NULL);
            printf("AUDIO IN STACK HW (RUN): %d\r\n", uxHighWaterMark_TX);
#endif
        }
    }

    vTaskDelete(xHandle_Audio_in_task);
}


/******************************************************************************************/
// UART PROTOCOL - JSON -- END --
/******************************************************************************************/