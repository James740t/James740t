#include "App/rm200x_radio_status.h"

/******************************************************************************************/
// RM200x ACK
/******************************************************************************************/

//FREERTOS CONTROL ITEMS
TaskHandle_t xHandle_task1 = NULL;
TaskHandle_t xHandle_task2 = NULL;

QueueHandle_t xqACK_queue1 = NULL;
QueueHandle_t xqACK_queue2 = NULL;

//Task Tags
const char *RADIO_STATUS_TASK_TAG = "RADIO_STATUS_TASK";

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
void reply_ACK_task(void *arg);
void process_ACK_task(void *arg);

/******************************************************************************************/
// STATUS - Helpers
/******************************************************************************************/


/******************************************************************************************/
// STATUS
/******************************************************************************************/
void init_radio_status(void) 
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
    //xTaskCreate(process_ACK_task, ACK_PROCESS_TASK_TAG, 1024*3, NULL, configMAX_PRIORITIES, &xHandle_ACK_Process);
    // Wait a shor time before starting the next task - they depend on each other...
    //vTaskDelay(250 / portTICK_PERIOD_MS);
    // ACK Reply Task - held with ACK Queue until data is available
    //xTaskCreate(reply_ACK_task, ACK_REPLY_TASK_TAG, 1024*3, NULL, configMAX_PRIORITIES, &xHandle_ACK_Reply);
    
}
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

    static uart_message_t _tx_ack;
    static uart_message_t *tx_ack = &_tx_ack;
    memset(tx_ack, 0, sizeof(_tx_ack));

    // ACK receive buffer Queue
    xqACK_Send_Reply = xQueueCreate(ACK_QUEUE_DEPTH, sizeof(_frame_in_buffer));

    // Task loop
    while (1)
    {
        // Clear stuff here ready for a new input message
        memset(frame_in_buffer, 0x00, sizeof(_frame_in_buffer));
        memset(ack_out_frame, 0x00, sizeof(_ack_out_frame));
        memset(tx_ack, 0, sizeof(_tx_ack));

        // Block until a message is put on the queue
        if (xQueueReceive(xqACK_Send_Reply, frame_in_buffer, (TickType_t)portMAX_DELAY) == pdPASS)
        {
            ESP_LOG_BUFFER_HEXDUMP(ACK_REPLY_TASK_TAG, frame_in_buffer, Get_Frame_Length(frame_in_buffer), ESP_LOG_INFO);

            if (Get_Frame_Intent(frame_in_buffer) == 0x02)
            {

            }
            else
            {

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

    // ACK Response output Queue
    xqACK_Response = xQueueCreate(ACK_QUEUE_DEPTH, sizeof(_rx_ack));

    // Task loop
    while (1)
    {
        // Clear stuff here ready for a new input message
        memset(frame_in_buffer, 0x00, sizeof(_frame_in_buffer));
        memset(rx_ack, 0, sizeof(_rx_ack));

        // Block until a message is put on the queue
        if (xQueueReceive(xqACK_Response, frame_in_buffer, (TickType_t)portMAX_DELAY) == pdPASS)
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

    free(xqACK_Response);
    vTaskDelete(xHandle_ACK_Process);
}

/******************************************************************************************/
// ACK   -- END --
/******************************************************************************************/