#include "app/rm200x_frame.h"

/******************************************************************************************/
// RM200x FRAME
/******************************************************************************************/

//FREERTOS CONTROL ITEMS
TaskHandle_t xHandle_Frame_Rx = NULL;

QueueHandle_t xqFrame_Rx = NULL;
QueueHandle_t xqFrame_Process = NULL;

//Task Tags
const char *FRAME_TASK_TAG = "FRAME_TASK";

// Globals

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

/******************************************************************************************/
// RM200x FRAME - Definitions
/******************************************************************************************/

/*****************************************************************************/
// RM200x FRAME BUILDING
/*****************************************************************************/
uint8_t CreateRollingCounter(void)
{
    static uint8_t counter;
    if (counter == UINT8_MAX)
    {
        counter = 0;
        return counter;
    }
    else
    {
        counter++;
        return counter;
    }
}

uint8_t Calculate_Checksum(const uint8_t *pt_frame_array)
{
    int _crc = 0x100;

    for (int i = 0; i < pt_frame_array[2]; i++)
    {
        _crc -= (int8_t)pt_frame_array[i + 3];
    }

    return (uint8_t)_crc;
}

uint8_t Get_Frame_Length (const uint8_t *pt_frame_array)
{
    uint8_t length_in = pt_frame_array[2];
    return length_in + 4;
}

uint8_t Get_Frame_Intent (const uint8_t *pt_frame_array)
{
    return pt_frame_array[4];
}

uint8_t Get_Frame_Payload (const uint8_t *pt_frame_array)
{
    return pt_frame_array[2];
}

uint8_t Get_Frame_CRC (const uint8_t *pt_frame_array)
{
    return pt_frame_array[2 + pt_frame_array[2] + 1];
}

uint8_t Get_Rolling_Counter (const uint8_t *pt_frame_array)
{
    return pt_frame_array[3];
}

/*
bit 0 == bad frame start
bit 1 == length error
bit 2 == bad crc
bit 3 == unknown intent
*/
uint8_t CheckFrame(const uint8_t *pt_frame_array)
{
    byte error;
    error.byte = 0x00;
    // Frame start is good ?
    if (pt_frame_array[0] != 0xFF) error.bits.bit0 = 1;
    if (pt_frame_array[1] != 0x55) error.bits.bit0 = 1;

    //Length and CRC are good
    // FF 55 --- LL --- xx yy zz --- crc 
    uint8_t length_in = pt_frame_array[2];
    if (length_in < 3) error.bits.bit1 = 1;

    uint8_t crc_idx = 2 + length_in + 1;
    uint8_t crc_in = pt_frame_array[crc_idx];
    uint8_t crc_calc = Calculate_Checksum(pt_frame_array);

    //ESP_LOGI(FRAME_TASK_TAG, "Pay. Len: 0x%02X, Msg. Len:: 0x%02X", length_in, crc_idx + 1);
    //ESP_LOGI(FRAME_TASK_TAG, "CRC in: 0x%02X, CRC calc: 0x%02X", crc_in, crc_calc);

    if (crc_in != crc_calc) error.bits.bit2 = 1;

    return error.byte;
}

uint8_t CreateFrame(uint8_t *pt_frame, const uint8_t pt_intent, const uint8_t data_size, const uint8_t *pt_data)
{    
    uint8_t index = 0;
    
    pt_frame[index++] = 0xFF;
    pt_frame[index++] = 0x55;
    pt_frame[index++] = data_size;
    pt_frame[index++] = CreateRollingCounter();
    pt_frame[index++] = pt_intent;
    //Add data here
    memcpy(&pt_frame[index], pt_data, data_size);
    //Add the checksum finally
    index += data_size;
    uint8_t crc = Calculate_Checksum(pt_frame);
    pt_frame[index++] = crc;

    // Check the frame
    byte check;
    check.byte = CheckFrame(pt_frame);

    ESP_LOG_BUFFER_HEXDUMP(FRAME_TASK_TAG, pt_frame, index, ESP_LOG_INFO);

    return check.byte;
   //Build OK returns the whole frame length else returns -1 == NOK 
}

uint8_t CreateSendFrame(const uint8_t pt_intent, const uint8_t data_size, const uint8_t *pt_data)
{
    static uint8_t _frame[FRAME_BUFFER_SIZE];
    static uint8_t *frame  = (uint8_t *)&_frame;;
    memset(frame, 0x00, sizeof(_frame));

    static uart_message_t _frm_message;
    static uart_message_t *frm_message = &_frm_message;
    memset(frm_message, 0x00, sizeof(_frm_message));

    uint8_t test = CreateFrame(frame, pt_intent, data_size, pt_data);

    if (test == ACK_SUCCESS)
    {
        // Build a UART message to queue
        frm_message->IsFrame = true;
        frm_message->IsASCII = false;
        frm_message->IsHEX = true;
        frm_message->port = FRAME_PORT;
        frm_message->length = (uint16_t)Get_Frame_Length(frame);
        frm_message->Message_ID = (int)Get_Rolling_Counter(frame);
        memcpy(&(frm_message->data), frame, frm_message->length);

        // Check the frame
        byte check;
        check.byte = CheckFrame(frame);

        if (check.byte == 0)
        {
            // Send it to the UART TX Queue
            ESP_LOG_BUFFER_HEXDUMP(FRAME_TASK_TAG, &(frm_message->data), frm_message->length, ESP_LOG_WARN);
            xQueueSend(xqUART_tx, frm_message, (TickType_t)0);
        }
    }
    return test;
}

/*****************************************************************************/
// RM200x FRAME BUILDING  -- END --
/*****************************************************************************/

void init_rm200x_application(void)
{
    // SETUP REPORTING LEVELS
    // Frame Module
    esp_log_level_set(FRAME_TASK_TAG, ESP_LOG_NONE);
    // ACK Module
    esp_log_level_set(ACK_REPLY_TASK_TAG, ESP_LOG_NONE);
    esp_log_level_set(ACK_PROCESS_TASK_TAG, ESP_LOG_NONE);
    // Audio Module
    esp_log_level_set(AUDIO_TASK_TAG, ESP_LOG_INFO);

    // SETUP ALL QUEUES
    // Setup the frame Queue - to be used after ACK has been sent
    xqFrame_Rx = xQueueCreate(FRAME_QUEUE_DEPTH, sizeof(uint8_t) * FRAME_BUFFER_SIZE);
    // Frame processing queue - long
    xqFrame_Process = xQueueCreate(FRAME_PROCESSOR_DEPTH, sizeof(uint8_t) * FRAME_BUFFER_SIZE);
    // ACK receive buffer Queue
    xqACK_Send_Reply = xQueueCreate(ACK_QUEUE_DEPTH, sizeof(uint8_t) * ACK_BUFFER_SIZE);
    // ACK receive buffer Queue
    xqACK_Process = xQueueCreate(ACK_QUEUE_DEPTH, sizeof(uint8_t) * ACK_BUFFER_SIZE);
    // ACK Response output Queue
    xqACK_Response = xQueueCreate(ACK_QUEUE_DEPTH, sizeof(ack_message_t));


    //TASKS
    // Frame marshalling
    xTaskCreate(process_Frame_task, FRAME_TASK_TAG, 1024*3, NULL, configMAX_PRIORITIES - 10, &xHandle_Frame_Rx);
    configASSERT(xHandle_Frame_Rx);

    // ACK in Process Task - ACK reply pumped out onto a queue
    xTaskCreate(process_ACK_task, ACK_PROCESS_TASK_TAG, 1024*3, NULL, configMAX_PRIORITIES - 8, &xHandle_ACK_Process);
    configASSERT(xHandle_ACK_Process);

    // ACK Reply Task - held with ACK Queue until data is available
    xTaskCreate(reply_ACK_task, ACK_REPLY_TASK_TAG, 1024*3, NULL, configMAX_PRIORITIES - 9, &xHandle_ACK_Reply);
    configASSERT(xHandle_ACK_Reply);

    // Audio In Task - Process audio parameters
    xTaskCreate(audio_IN_task, AUDIO_TASK_TAG, 1024*3, NULL, configMAX_PRIORITIES - 7, &xHandle_Audio_in_task);
    configASSERT(xHandle_Audio_in_task);

    
}

/******************************************************************************************/
// FRAME RX TASK
/******************************************************************************************/

void process_Frame_task(void *arg)
{
#if STACK_MONITOR
    /* Inspect our own high water mark on entering the task. */
    uxHighWaterMark_RX = uxTaskGetStackHighWaterMark(NULL);
    printf("FRAME PROCESS STACK HW (START) = %d\r\n", uxHighWaterMark_RX);
#endif

    // Initialise things here
    static uint8_t _frame_in_buffer[FRAME_BUFFER_SIZE];
    static uint8_t *frame_in_buffer = _frame_in_buffer;
    memset(frame_in_buffer, 0x00, sizeof(_frame_in_buffer));

    static uint8_t _frame_out_buffer[FRAME_BUFFER_SIZE];
    static uint8_t *frame_out_buffer = _frame_out_buffer;
    memset(frame_out_buffer, 0x00, sizeof(_frame_out_buffer));

    static uint8_t _frame_ack_buffer[FRAME_BUFFER_SIZE];
    static uint8_t *frame_ack_buffer = _frame_ack_buffer;
    memset(frame_ack_buffer, 0x00, sizeof(_frame_ack_buffer));

    // Task loop
    while (1)
    {
        // Clear stuff here ready for a new input message
        memset(frame_in_buffer, 0x00, sizeof(_frame_in_buffer));
        memset(frame_out_buffer, 0x00, sizeof(_frame_out_buffer));
        memset(frame_ack_buffer, 0x00, sizeof(_frame_ack_buffer));

        // Block until a message is put on the queue
        if (xQueueReceive(xqFrame_Rx, frame_in_buffer, (TickType_t)portMAX_DELAY) == pdPASS)
        {
            ESP_LOG_BUFFER_HEXDUMP(FRAME_TASK_TAG, frame_in_buffer, Get_Frame_Length(frame_in_buffer), ESP_LOG_WARN);

            // Copy the incomming buffer
            memcpy(frame_ack_buffer, frame_in_buffer, sizeof(_frame_in_buffer));
            memcpy(frame_out_buffer, frame_in_buffer, sizeof(_frame_in_buffer));

            // Get incomming intent:
            uint8_t intent = Get_Frame_Intent(frame_in_buffer);

            // It is a normal incomming message so we need to ACK it
            if (intent != CMD_ACK)
            {
                // ESP_LOG_BUFFER_HEXDUMP(FRAME_TASK_TAG, frame_ack_buffer, Get_Frame_Length(frame_ack_buffer), ESP_LOG_WARN);
                xQueueSend(xqACK_Send_Reply, frame_ack_buffer, (TickType_t)0);
            }

            if (CheckFrame(frame_in_buffer) == 0x00) // Frame is good
            {
                
/******************************************************************************************************************************************/
                // Process the incomming intents i.e. do stuff with the data received
/******************************************************************************************************************************************/
                switch (intent)
                {
                case CMD_ACK:
                {
                    // It is an ACK Message so we need to processed it to get the ACK status
                    xQueueSend(xqACK_Process, frame_ack_buffer, (TickType_t)0);
                    break;
                }
                case CMD_AUDIO:         // Volume and Mute status
                case CMD_EQ:            // Tone Controls
                case CMD_POWER_STATE:   // Power status
                // INSERT HERE
                {
                    xQueueSend(xqFrame_Process, frame_out_buffer, (TickType_t)0);
                    break;
                }

                default:
                {
                    // do nothing
                    break;
                }
                }
            }

#if STACK_MONITOR
            uxHighWaterMark_RX = uxTaskGetStackHighWaterMark(NULL);
            printf("FRAME PROCESS STACK HW (RUN): %d\r\n", uxHighWaterMark_RX);
#endif
        }
    }

    free(xqFrame_Rx);
    vTaskDelete(xHandle_Frame_Rx);
}

/******************************************************************************************/
// RM200x FRAME -- END --
/******************************************************************************************/