#include "App/rm200x_radio_status.h"

/******************************************************************************************/
// RM200x ACK
/******************************************************************************************/

//FREERTOS CONTROL ITEMS
TaskHandle_t xHandle_Audio_in_task = NULL;
TaskHandle_t xHandle_Audio_out_task = NULL;

QueueHandle_t xqAUDIO_tx = NULL;

//Task Tags
const char *RADIO_STATUS_TASK_TAG = "RADIO_STATUS_TASK";
const char *AUDIO_TASK_TAG = "AUDIO_TASK";
const char *RADIO_COMMAND_TASK_TAG = "RADIO_COMMAND_TASK";

//GLOBALS

//Local prototypes

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
//   PARAMETERS 
/******************************************************************************************/

/******************************************************************************************/
// Definitions
/******************************************************************************************/

/******************************************************************************************/
// STATUS - Helpers
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

    ESP_LOGI(AUDIO_TASK_TAG, "Message: %s/%s - data bytes = %d\r\n", mqtt_msg->topic, mqtt_msg->data, mqtt_msg->data_len);
}

/******************************************************************************************/
// STATUS
/******************************************************************************************/

/******************************************************************************************/
// AUDIO TASK
/******************************************************************************************/

void audio_IN_task(void *arg)
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

    static char _json_buffer[JSON_STRING_LENGTH];
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
            //ESP_LOG_BUFFER_HEXDUMP(AUDIO_TASK_TAG, frame_in_buffer, Get_Frame_Length(frame_in_buffer), ESP_LOG_WARN);
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

void audio_OUT_task(void *arg)
{
#if STACK_MONITOR
    /* Inspect our own high water mark on entering the task. */
    uxHighWaterMark_TX = uxTaskGetStackHighWaterMark(NULL);
    printf("AUDIO OUT STACK HW (START) = %d\r\n", uxHighWaterMark_TX);
#endif

    // Initialise things here
    static uint8_t _frame_in_buffer[STATUS_BUFFER_SIZE];
    static uint8_t *frame_in_buffer = _frame_in_buffer;
    memset(frame_in_buffer, 0x00, sizeof(_frame_in_buffer));

    static uart_message_t _tx_cmd;
    static uart_message_t *tx_cmd = &_tx_cmd;
    memset(tx_cmd, 0, sizeof(uart_message_t));

    // Task loop
    while (1)
    {
        // Clear stuff here ready for a new input message
        memset(frame_in_buffer, 0x00, sizeof(_frame_in_buffer));
        memset(tx_cmd, 0, sizeof(uart_message_t));

        // Block until a message is put on the queue
        if (xQueueReceive(xqFrame_Process, frame_in_buffer, (TickType_t)portMAX_DELAY) == pdPASS)
        {
            ESP_LOG_BUFFER_HEXDUMP(AUDIO_TASK_TAG, frame_in_buffer, Get_Frame_Length(frame_in_buffer), ESP_LOG_INFO);



#if STACK_MONITOR
            uxHighWaterMark_TX = uxTaskGetStackHighWaterMark(NULL);
            printf("AUDIO OUT STACK HW (RUN): %d\r\n", uxHighWaterMark_TX);
#endif
        }
    }

    vTaskDelete(xHandle_Audio_out_task);
}

/******************************************************************************************/
// ACK   -- END --
/******************************************************************************************/