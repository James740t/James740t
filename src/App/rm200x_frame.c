#include "app/rm200x_frame.h"

/******************************************************************************************/
// RM200x FRAME
/******************************************************************************************/

//FREERTOS CONTROL ITEMS
TaskHandle_t xHandle_Frame_Rx = NULL;
TaskHandle_t xHandle_Frame_Tx = NULL;

QueueHandle_t xqFrame_Rx = NULL;
QueueHandle_t xqFrame_Tx = NULL;
QueueHandle_t xqFrame_Process = NULL;


//Task Tags
const char *FRAME_RX_TAG = "FRAME_RX";
const char *FRAME_TX_TAG = "FRAME_TX";

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

uint8_t Calculate_Checksum(uint8_t *pt_frame_array)
{
    uint8_t len = pt_frame_array[2];
    uint8_t *p_payload = (uint8_t *)&pt_frame_array[3]; // Start of CRC calc data - payload
    uint8_t _sum = 0;

    for (int i = 1; i <= len; i++)  // Length so count from 1 to length
    {
        _sum += *p_payload++;
    }

    uint8_t crc = (uint8_t)(0x100 - _sum);
    //pt_frame_array[len + 3] = crc;
    return crc;
}

uint8_t Get_Frame_Length (uint8_t *pt_frame_array)
{
    uint8_t length_in = pt_frame_array[2];
    return length_in + 4;
}

uint8_t Get_Frame_Intent (uint8_t *pt_frame_array)
{
    return pt_frame_array[4];
}

uint8_t Get_Frame_Payload (uint8_t *pt_frame_array)
{
    return pt_frame_array[2];
}

uint8_t Get_Frame_CRC ( uint8_t *pt_frame_array)
{
    return pt_frame_array[2 + pt_frame_array[2] + 1];
}

uint8_t Get_Rolling_Counter (uint8_t *pt_frame_array)
{
    return pt_frame_array[3];
}

/*
bit 0 == bad frame start
bit 1 == length error
bit 2 == bad crc
bit 3 == unknown intent
*/
uint8_t CheckFrame(uint8_t *pt_frame_array)
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

    if (crc_in != crc_calc) error.bits.bit2 = 1;

    if(error.byte)
    {
        ESP_LOGE(FRAME_RX_TAG, "ERROR BYTE: 0x%02X", error.byte);
        ESP_LOGE(FRAME_RX_TAG, "Pay. Len: 0x%02X, Msg. Len:: 0x%02X", length_in, crc_idx + 1);
        ESP_LOGE(FRAME_RX_TAG, "CRC in: 0x%02X, CRC calc: 0x%02X", crc_in, crc_calc);
    }

    return error.byte;
}

uint8_t CreateFrame(uint8_t *pt_frame, uint8_t pt_intent, uint8_t *pt_data, uint8_t data_size)
{    
    uint8_t index = 0;
    
    pt_frame[index++] = 0xFF;                   // 0
    pt_frame[index++] = 0x55;                   // 1
    pt_frame[index++] = data_size;              // 2
    pt_frame[index++] = CreateRollingCounter(); // 3
    pt_frame[index++] = pt_intent;              // 4
    //Add data here

    memcpy(&pt_frame[index], pt_data, data_size);   // 5 start
    //Add the checksum finally
    //index += data_size;
    uint8_t crc = Calculate_Checksum(pt_frame);
    pt_frame[2 + data_size + 1] = crc;

    // Check the frame
    byte check;
    check.byte = CheckFrame(pt_frame);

    //ESP_LOG_BUFFER_HEXDUMP("DEBUG MESS:", pt_frame, Get_Frame_Length(pt_frame), ESP_LOG_ERROR);

    if (check.byte)
    {
        ESP_LOGE(FRAME_TX_TAG, "Bad frame Build : 0x%02X", check.byte);
    }

    return check.byte;
   //Build OK returns the whole frame length else returns 0x00 else > 0 == NOK
}

uint8_t CreateSendFrame(uint8_t pt_intent, uint8_t *pt_data, uint8_t data_size)
{
    static uint8_t _frame[FRAME_BUFFER_SIZE];
    static uint8_t *frame  = (uint8_t *)&_frame;;
    memset(frame, 0x00, sizeof(_frame));

    uint8_t err = CreateFrame(frame, pt_intent, pt_data, data_size);

    if (!err)
    {
        // Send the Frame out onto the UART
        err = SendFrame(frame, FRAME_PORT);
    }
    else
    {
        ESP_LOGE(FRAME_TX_TAG, "Frame Send Error : 0x%02X", err);
    }
    return err;
}

/*****************************************************************************/
// RM200x FRAME SENDER
/*****************************************************************************/

uint8_t SendFrame(uint8_t *p_frame_array, uint8_t p_port)
{
    static uart_message_t _tx_uart;
    static uart_message_t *tx_uart = &_tx_uart;
    memset(tx_uart, 0, sizeof(uart_message_t));

    // Build a UART message to queue
    tx_uart->IsFrame = true;
    tx_uart->IsASCII = false;
    tx_uart->IsHEX = true;
    tx_uart->port = p_port;
    tx_uart->length = Get_Frame_Length(p_frame_array);
    tx_uart->Message_ID = *(p_frame_array + 3);
    memcpy(&(tx_uart->data), p_frame_array, tx_uart->length);

    // Check the frame
    byte check;
    check.byte = CheckFrame((uint8_t *)&(tx_uart->data));

    if (!check.byte)
    {
        // Send it to the TX Queue
        ESP_LOG_BUFFER_HEXDUMP(FRAME_TX_TAG, &(tx_uart->data), tx_uart->length, ESP_LOG_WARN);
        xQueueSend(xqUART_tx, tx_uart, (TickType_t)0);
    }
    else
    {
        ESP_LOGE(FRAME_TX_TAG, "Frame TX Error : 0x%02X", check.byte);
    }

    return check.byte;
}

/*****************************************************************************/
// RM200x FRAME BUILDING  -- END --
/*****************************************************************************/

void init_rm200x_application(void)
{
    // SETUP REPORTING LEVELS
    // Frame Module
    esp_log_level_set(FRAME_RX_TAG, ESP_LOG_ERROR);
    esp_log_level_set(FRAME_TX_TAG, ESP_LOG_ERROR);
    esp_log_level_set(PROTOCOL_TAG, ESP_LOG_INFO);
    // ACK Module
    esp_log_level_set(ACK_REPLY_TASK_TAG, ESP_LOG_NONE);
    esp_log_level_set(ACK_PROCESS_TASK_TAG, ESP_LOG_NONE);
    // Audio Module
    esp_log_level_set(AUDIO_TASK_TAG, ESP_LOG_WARN);

    // SETUP ALL QUEUES
    // Setup the frame Rx Queue
    xqFrame_Rx = xQueueCreate(FRAME_QUEUE_DEPTH, sizeof(uint8_t) * FRAME_BUFFER_SIZE);
    // Setup the frame Tx Queue - Used from external sources e.g. MQTT
    xqFrame_Tx = xQueueCreate(FRAME_TX_QUEUE_DEPTH, sizeof(uint8_t) * FRAME_TX_BUFFER);
    // Frame processing queue - long
    xqFrame_Process = xQueueCreate(FRAME_PROCESSOR_DEPTH, sizeof(uint8_t) * FRAME_BUFFER_SIZE);
    // ACK receive buffer Queue
    xqACK_Send_Reply = xQueueCreate(ACK_QUEUE_DEPTH, sizeof(uint8_t) * ACK_BUFFER_SIZE);
    // ACK receive buffer Queue
    xqACK_Process = xQueueCreate(ACK_QUEUE_DEPTH, sizeof(uint8_t) * ACK_BUFFER_SIZE);
    // ACK Response output Queue
    xqACK_Response = xQueueCreate(ACK_QUEUE_DEPTH, sizeof(ack_message_t));


    //SETUP AND START ALL TASKS
    // Frame marshalling
    xTaskCreate(process_Frame_task, FRAME_RX_TAG, 1024*3, NULL, configMAX_PRIORITIES, &xHandle_Frame_Rx);
    configASSERT(xHandle_Frame_Rx);

    // Frame Out Task - Process all frame transmissions
    xTaskCreate(transmit_Frame_task, FRAME_TX_TAG, 1024*3, NULL, configMAX_PRIORITIES, &xHandle_Frame_Tx);
    configASSERT(xHandle_Frame_Tx);

    // ACK in Process Task - ACK reply pumped out onto a queue
    xTaskCreate(process_ACK_task, ACK_PROCESS_TASK_TAG, 1024*3, NULL, configMAX_PRIORITIES, &xHandle_ACK_Process);
    configASSERT(xHandle_ACK_Process);

    // ACK Reply Task - held with ACK Queue until data is available
    xTaskCreate(reply_ACK_task, ACK_REPLY_TASK_TAG, 1024*3, NULL, configMAX_PRIORITIES, &xHandle_ACK_Reply);
    configASSERT(xHandle_ACK_Reply);

    // Audio In Task - Process audio parameters
    xTaskCreate(audio_IN_task, AUDIO_TASK_TAG, 1024*3, NULL, configMAX_PRIORITIES, &xHandle_Audio_in_task);
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

    uint8_t err = 0;

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
            ESP_LOG_BUFFER_HEXDUMP(FRAME_RX_TAG, frame_in_buffer, Get_Frame_Length(frame_in_buffer), ESP_LOG_INFO);

            // Copy the incomming buffer
            memcpy(frame_ack_buffer, frame_in_buffer, sizeof(_frame_in_buffer));
            memcpy(frame_out_buffer, frame_in_buffer, sizeof(_frame_in_buffer));

            err = CheckFrame(frame_in_buffer);
            if (!err) // Frame is good
            {  
                // Get incomming intent:
                uint8_t intent = Get_Frame_Intent(frame_in_buffer);

                // It is a normal incomming message so we need to ACK it
                if (intent != CMD_ACK)
                {
                    //ESP_LOG_BUFFER_HEXDUMP(FRAME_RX_TAG, frame_ack_buffer, Get_Frame_Length(frame_ack_buffer), ESP_LOG_WARN);
                    xQueueSend(xqACK_Send_Reply, frame_ack_buffer, (TickType_t)0);
                }

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
                // INSERT INCOMMING INTENTS TO BE PROCESSED HERE
                case CMD_AUDIO:       // Volume and Mute status
                case CMD_EQ:          // Tone Controls
                case CMD_POWER_STATE: // Power status
                {
                    // Send for processing - in our case Json and MQTT
                    //ESP_LOG_BUFFER_HEXDUMP("FRAME RX DEBUG TAG", frame_out_buffer, Get_Frame_Length(frame_out_buffer), ESP_LOG_WARN);
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
            else
            {
                ESP_LOGE(FRAME_RX_TAG, "BAD FRAME - Error code: 0x%02X", err);
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
// FRAME RX TASK
/******************************************************************************************/

void transmit_Frame_task(void *arg)
{
#if STACK_MONITOR
    /* Inspect our own high water mark on entering the task. */
    uxHighWaterMark_TX = uxTaskGetStackHighWaterMark(NULL);
    printf("FRAME PROCESS STACK HW (START) = %d\r\n", uxHighWaterMark_TX);
#endif

    // Initialise things here
    int err = 0;
    int _len = 0;

    static uint8_t _incomming[FRAME_TX_BUFFER];
    static uint8_t *incomming = _incomming;
    memset(incomming, 0x00, sizeof(_incomming));

    static uint8_t _frame[FRAME_TX_BUFFER];
    static uint8_t *frame = _frame;
    memset(frame, 0x00, sizeof(_frame));

    while (1)
    {
        // Clear stuff here ready for a new input message
        memset(incomming, 0x00, sizeof(_incomming));

        // Block until a message is put on the queue
        if (xQueueReceive(xqFrame_Tx, incomming, (TickType_t)portMAX_DELAY) == pdPASS)
        {
            // Data can be anything - likely to be JSON or Frame (assume string for diag.)
            ESP_LOG_BUFFER_HEXDUMP(FRAME_TX_TAG, incomming, strlen((char *)incomming), ESP_LOG_INFO);
            //Check if frame [0] == FF and [1] == 55
            if ((incomming[0] == 0xFF) && (incomming[1] == 0x55))
            {
                // treat it like a frame
                err = SendFrame(incomming, FRAME_PORT);
                if (err)
                {
                    ESP_LOG_BUFFER_HEXDUMP(FRAME_TX_TAG, incomming, strlen((char *)incomming), ESP_LOG_ERROR);
                }
            }
            else
            {
                // Treat it like a string representation of a frame

                // Is it a string representation of a hex frame
                if (
                    (incomming[0] == 0x46) 
                    && (incomming[1] == 0x46) 
                    && (incomming[2] == 0x20) 
                    && (incomming[3] == 0x35) 
                    && (incomming[4] == 0x35)
                    )
                {
                    //Clean frame buffer
                    memset(frame, 0x00, sizeof(_frame));
                    //Parse message and convert it to binary (hex) format
                    Hex2Bytes((char *)incomming, (char *)frame, &_len);
                    //Send it out as a frame
                    err = SendFrame(frame, FRAME_PORT);
                    if (err)
                    {
                        ESP_LOG_BUFFER_HEXDUMP(FRAME_TX_TAG, frame, _len, ESP_LOG_ERROR);
                    }
                }
                // Is it a JSON special request that needs translation and further - it will start with " or {
                if (
                       (incomming[0] == 0x22) 
                    || (incomming[0] == 0x7B) 
                    )
                {
                    // Extract the intent structure and the process it.
                    err = json_volume_set((char *)incomming);
                    if (err)
                    {
                        ESP_LOG_BUFFER_HEXDUMP(FRAME_TX_TAG, incomming, strlen((char *)incomming), ESP_LOG_ERROR);
                    }
                    // err = json_mute_set((char *)incomming);
                    // if (err)
                    // {
                    //     ESP_LOG_BUFFER_HEXDUMP(FRAME_TX_TAG, incomming, strlen((char *)incomming), ESP_LOG_ERROR);
                    // }

                }
            }

        }


#if STACK_MONITOR
        uxHighWaterMark_TX = uxTaskGetStackHighWaterMark(NULL);
        printf("FRAME PROCESS STACK HW (RUN): %d\r\n", uxHighWaterMark_TX);
#endif
    }

    free(xqFrame_Tx);
    vTaskDelete(xHandle_Frame_Tx);
}

/******************************************************************************************/
// RM200x FRAME -- END --
/******************************************************************************************/