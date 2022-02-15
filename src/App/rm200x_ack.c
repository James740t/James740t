#include "app/rm200x_ack.h"

/******************************************************************************************/
// RM200x ACK
/******************************************************************************************/

//FREERTOS CONTROL ITEMS
TaskHandle_t xHandle_ACK_Reply = NULL;
TaskHandle_t xHandle_ACK_Process = NULL;

QueueHandle_t xqACK_Send_Reply = NULL;
QueueHandle_t xqACK_Response = NULL;
QueueHandle_t xqACK_Process = NULL;

//Task Tags
const char *ACK_REPLY_TASK_TAG = "ACK_REPLY_TASK";
const char *ACK_PROCESS_TASK_TAG = "ACK_PROCESS_TASK";

//ENUMs

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
// ACK - Definitions
/******************************************************************************************/

/******************************************************************************************/
// ACK - Helpers
/******************************************************************************************/

uint8_t Create_ACK_Reply(uint8_t *pt_ack, const uint8_t *pt_frame_in_array, uint8_t p_ack_state)
{
    uint8_t index = 0;
    pt_ack[index++] = 0xFF;                     // Frame start
    pt_ack[index++] = 0x55;                     // Frame start 2
    pt_ack[index++] = 0x04;                     // Frame length (payload)
    pt_ack[index++] = *(pt_frame_in_array + 3); // Frame rolling counter
    pt_ack[index++] = 0x02;                     // ACK Intent 0x02
    pt_ack[index++] = p_ack_state;              // ACK code 0x00 == SUCCESS
    pt_ack[index++] = *(pt_frame_in_array + 4); // Intent to be ACK'ed
    pt_ack[index] = Calculate_Checksum(pt_ack); // CRC

    // Check the frame
    byte check;
    check.byte = CheckFrame(pt_ack);
    return check.byte;
}

uint8_t Create_ACK_Reply_UART(const uint8_t *pt_frame_in_array, uint8_t p_ack_state)
{
    static uint8_t _ack[ACK_FRAME_LENGTH];
    static uint8_t *pt_ack = _ack;
    memset(pt_ack, 0x00, sizeof(_ack));

    static uart_message_t _tx_ack;
    static uart_message_t *tx_ack = &_tx_ack;
    memset(tx_ack, 0, sizeof(_tx_ack));

    uint8_t index = 0;
    pt_ack[index++] = 0xFF;                         // Frame start
    pt_ack[index++] = 0x55;                         // Frame start 2
    pt_ack[index++] = 0x04;                         // Frame length (payload)
    pt_ack[index++] = *(pt_frame_in_array + 3);     // Frame rolling counter
    pt_ack[index++] = 0x02;                         // ACK Intent 0x02
    pt_ack[index++] = p_ack_state;                  // ACK code 0x00 == SUCCESS
    pt_ack[index++] = *(pt_frame_in_array + 4);     // Intent to be ACK'ed
    pt_ack[index] = Calculate_Checksum(pt_ack);     // CRC

    // Build a UART message to queue
    tx_ack->IsFrame = true;
    tx_ack->IsASCII = false;
    tx_ack->IsHEX = true;
    tx_ack->port = ACK_PORT;
    tx_ack->length = ACK_FRAME_LENGTH;
    tx_ack->Message_ID = *(pt_frame_in_array + 3);
    memcpy(&(tx_ack->data), pt_ack, ACK_FRAME_LENGTH);

    // Check the frame
    byte check;
    check.byte = CheckFrame(pt_ack);

    ESP_LOGI(ACK_REPLY_TASK_TAG, "ACK Message Error : 0x%02X", check.byte);
    if (!check.byte)
    {
        // Send it to the TX Queue
        ESP_LOG_BUFFER_HEXDUMP(ACK_REPLY_TASK_TAG, &(tx_ack->data), tx_ack->length, ESP_LOG_WARN);
        xQueueSend(xqUART_tx, tx_ack, (TickType_t)0);
    }

    return check.byte;
}

/******************************************************************************************/
// ACK
/******************************************************************************************/

/******************************************************************************************/
// ACK TASK
/******************************************************************************************/

void reply_ACK_task(void *arg)
{
#if STACK_MONITOR
    /* Inspect our own high water mark on entering the task. */
    uxHighWaterMark_TX = uxTaskGetStackHighWaterMark(NULL);
    printf("ACK REPLY STACK HW (START) = %d\r\n", uxHighWaterMark_TX);
#endif

    // Initialise things here
    static uint8_t _frame_in_buffer[ACK_BUFFER_SIZE];
    static uint8_t *frame_in_buffer = _frame_in_buffer;
    memset(frame_in_buffer, 0x00, sizeof(_frame_in_buffer));

    static uint8_t _ack_out_frame[ACK_FRAME_LENGTH];
    static uint8_t *ack_out_frame = _ack_out_frame;
    memset(ack_out_frame, 0x00, sizeof(_ack_out_frame));

    // Task loop
    while (1)
    {
        // Clear stuff here ready for a new input message
        memset(frame_in_buffer, 0x00, sizeof(_frame_in_buffer));
        memset(ack_out_frame, 0x00, sizeof(_ack_out_frame));
        
        // Block until a message is put on the queue
        if (xQueueReceive(xqACK_Send_Reply, frame_in_buffer, (TickType_t)portMAX_DELAY) == pdPASS)
        {
            ESP_LOG_BUFFER_HEXDUMP(ACK_REPLY_TASK_TAG, frame_in_buffer, Get_Frame_Length(frame_in_buffer), ESP_LOG_INFO);

            // Check the incomming frame
            byte error;
            error.byte = CheckFrame(frame_in_buffer);
            uint8_t ack_status = 0x00;
            if (error.byte == 0x00)
                ack_status = ACK_SUCCESS;
            if (error.bits.bit0)
                ack_status = ACK_UNKNOWN_ERROR;
            if (error.bits.bit1)
                ack_status = ACK_BAD_LENGTH;
            if (error.bits.bit2)
                ack_status = ACK_CRC_ERROR;
            if (error.bits.bit3)
                ack_status = ACK_UNKNOWN_INTENT;

            ESP_LOGI(ACK_REPLY_TASK_TAG, "ACK Status : 0x%02X", ack_status);

            uint8_t err = Create_ACK_Reply_UART(frame_in_buffer, ack_status);

            if(err)
            {
                ESP_LOG_BUFFER_HEXDUMP(ACK_REPLY_TASK_TAG, frame_in_buffer, Get_Frame_Length(frame_in_buffer), ESP_LOG_ERROR);
            }
            

#if STACK_MONITOR
            uxHighWaterMark_TX = uxTaskGetStackHighWaterMark(NULL);
            printf("ACK REPLY STACK HW (RUN): %d\r\n", uxHighWaterMark_TX);
#endif
        }
    }

    free(xqACK_Send_Reply);
    vTaskDelete(xHandle_ACK_Reply);
}

void process_ACK_task(void *arg)
{
#if STACK_MONITOR
    /* Inspect our own high water mark on entering the task. */
    uxHighWaterMark_RX = uxTaskGetStackHighWaterMark(NULL);
    printf("ACK PROCESS STACK HW (START) = %d\r\n", uxHighWaterMark_RX);
#endif

    // Initialise things here
    static uint8_t _frame_in_buffer[ACK_BUFFER_SIZE];
    static uint8_t *frame_in_buffer = _frame_in_buffer;
    memset(frame_in_buffer, 0x00, sizeof(_frame_in_buffer));

    static ack_message_t _rx_ack;
    static ack_message_t *rx_ack = &_rx_ack;
    memset(rx_ack, 0, sizeof(_rx_ack));

    // Task loop
    while (1)
    {
        // Clear stuff here ready for a new input message
        memset(frame_in_buffer, 0x00, sizeof(_frame_in_buffer));
        memset(rx_ack, 0, sizeof(_rx_ack));

        // Block until a message is put on the queue
        if (xQueueReceive(xqACK_Process, frame_in_buffer, (TickType_t)portMAX_DELAY) == pdPASS)
        {
            ESP_LOG_BUFFER_HEXDUMP(ACK_PROCESS_TASK_TAG, frame_in_buffer, Get_Frame_Length(frame_in_buffer), ESP_LOG_INFO);

            //Check if ACK frame is good:
            byte error;
            error.byte = CheckFrame(frame_in_buffer);
            if (!error.byte)
            {
                // Pull out the ACK info we need
                rx_ack->Counter = frame_in_buffer[3];
                rx_ack->Status = frame_in_buffer[5];
                rx_ack->Intent = frame_in_buffer[6];
                
                // Send out to the response Queue
                xQueueSend(xqACK_Response, rx_ack, (TickType_t)0);
                ESP_LOGW(ACK_PROCESS_TASK_TAG, "Counter: 0x%02X, Intent: 0x%02X, Status: 0x%02X", rx_ack->Counter, rx_ack->Intent, rx_ack->Status);
            }

#if STACK_MONITOR
            uxHighWaterMark_RX = uxTaskGetStackHighWaterMark(NULL);
            printf("ACK PROCESS STACK HW (RUN): %d\r\n", uxHighWaterMark_RX);
#endif
        }
    }

    free(xqACK_Process);
    free(xqACK_Response);
    vTaskDelete(xHandle_ACK_Process);
}

/******************************************************************************************/
// ACK   -- END --
/******************************************************************************************/