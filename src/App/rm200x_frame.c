#include "app/rm200x_frame.h"

/******************************************************************************************/
// RM200x FRAME
/******************************************************************************************/

//FREERTOS CONTROL ITEMS
TaskHandle_t xHandle_Frame_Rx = NULL;

QueueHandle_t xqFrame_Rx = NULL;

//Task Tags
const char *FRAME_TASK_TAG = "FRAME_TASK";

// Globals

//Local prototypes
void process_Frame_task(void *arg);

#define STACK_MONITOR   true
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
    return length_in + 5;
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

    ESP_LOGI(FRAME_TASK_TAG, "Pay. Len: 0x%02X, Msg. Len:: 0x%02X", length_in, crc_idx + 1);
    ESP_LOGI(FRAME_TASK_TAG, "CRC in: 0x%02X, CRC calc: 0x%02X", crc_in, crc_calc);

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
    static uint8_t _frame[FRAME_FRAME_LENGTH];
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

void init_Frames(void)
{
    // Initialise things here - startup

    // wait for all required tasks to come online
    while (
        // UART is all running
        (xqUART_tx == NULL) || (xqUART_rx == NULL) || (xHandle_uart_tx == NULL) || (xHandle_uart_rx == NULL) || (xHandle_uart_isr == NULL))
    {
        // wait 100ms then check again
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    // Start the tasks running here:
    // ACK in Process Task - ACK reply pumped out onto a queue
    xTaskCreate(process_Frame_task, FRAME_TASK_TAG, 1024*3, NULL, configMAX_PRIORITIES, &xHandle_Frame_Rx);
    // Wait a shor time before starting the next task - they depend on each other...
    //vTaskDelay(250 / portTICK_PERIOD_MS);
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

    // Setup the frame Queue - to be used after ACK has been sent
    xqFrame_Rx = xQueueCreate(FRAME_QUEUE_DEPTH, sizeof(uint8_t) * FRAME_FRAME_LENGTH);

    //Wait for Queues to be online
    while (
        // Make sure all Queues are running
        (xqACK_Send_Reply == NULL) || (xqFrame_Rx == NULL) || (xqACK_Response == NULL))
    {
        // wait 100ms then check again
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    // Task loop
    while (1)
    {
        // Clear stuff here ready for a new input message
        memset(frame_in_buffer, 0x00, sizeof(_frame_in_buffer));

        // Block until a message is put on the queue
        if (xQueueReceive(xqFrame_Rx, frame_in_buffer, (TickType_t)portMAX_DELAY) == pdPASS)
        {
            // Send it to be ACK'ed
            xQueueSend(xqACK_Send_Reply, frame_in_buffer, (TickType_t)0);

            ESP_LOG_BUFFER_HEXDUMP(FRAME_TASK_TAG, frame_in_buffer, Get_Frame_Length(frame_in_buffer), ESP_LOG_WARN);

            //Get incomming intent:
            uint8_t intent = Get_Frame_Intent(frame_in_buffer);
            // Do stuff with the data received
            switch (intent)
            {
                case CMD_AUDIO:
                {
                    ESP_LOG_BUFFER_HEXDUMP(FRAME_TASK_TAG, frame_in_buffer, Get_Frame_Length(frame_in_buffer), ESP_LOG_INFO);
                    break;
                }



                default:
                {
                    // do nothing
                    break;
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