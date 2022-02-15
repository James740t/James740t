#include "base/rm200x_uart.h"

/******************************************************************************************/
// RM200x UART
/******************************************************************************************/

//FREERTOS CONTROL ITEMS
SemaphoreHandle_t bin_s_sync_uart_tx_task = NULL;
SemaphoreHandle_t bin_s_sync_uart_rx_process_task = NULL;

TaskHandle_t xHandle_uart_tx = NULL;
TaskHandle_t xHandle_uart_rx = NULL;
TaskHandle_t xHandle_uart_isr = NULL;

QueueHandle_t xqUART_events  = NULL;
QueueHandle_t xqUART_rx = NULL;
QueueHandle_t xqUART_tx = NULL;

//Task Tags
const char *TX_TASK_TAG = "TX_TASK";
const char *RX_TASK_TAG = "RX_TASK";
const char *UART_ISR_TASK_TAG = "UART_ISR_TASK";

#define TXD_PIN     (GPIO_NUM_17)
#define RXD_PIN     (GPIO_NUM_16)

//GLOBALS
// Variable dwell - initialise to 0 :
uint16_t UART_VARIABLE_TX_DELAY = 0;
bool ALL_ASCII = false;
bool HEX_DELIMITED = true;

bool PATTERN_0xFF_DETECTED = false;

//Local prototypes
void rx_uart_task(void *arg);
void tx_uart_task(void *arg);
void uart_event_task(void *pvParameters);

//#define STACK_MONITOR   1
#ifdef STACK_MONITOR
UBaseType_t uxHighWaterMark_RX;
UBaseType_t uxHighWaterMark_TX;
UBaseType_t uxHighWaterMark_EV;
#endif

        // #ifdef STACK_MONITOR
        //     /* Inspect our own high water mark on entering the task. */
        //     uxHighWaterMark_RX = uxTaskGetStackHighWaterMark( NULL );
        //     printf("MQTT RX STACK HW (START) = %d\r\n", uxHighWaterMark_RX);
        // #endif

        // #ifdef STACK_MONITOR
        //     uxHighWaterMark_RX = uxTaskGetStackHighWaterMark( NULL );
        //     printf("MQTT RX STACK HW (RUN): %d\r\n", uxHighWaterMark_RX);
        // #endif

/******************************************************************************************/
// Helpers
/******************************************************************************************/
void reg_delay (void) 
{
	__asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;");
}

void mqtt_send(mqtt_esp_message_t *mqtt_out)
{
    // send pointer to structure of the outgoing message
    xQueueSend(xqMQTT_tx_Messages, mqtt_out, (TickType_t)0);
    // Flash LED to show activity
    xSemaphoreGive(bin_s_sync_blink_task);

    ESP_LOGI(RX_TASK_TAG, "Message: %s/%s - data bytes = %d\r\n", mqtt_out->topic, mqtt_out->data, mqtt_out->data_len);
}

int HexStringToBytes(const char *hexStr, char *output, int *outputLen)
{
    size_t len = strlen(hexStr);
    if (len % 2 != 0)
    {
        return -1;
    }

    // force the string to uppercase:
    strupr((char *)hexStr);

    size_t finalLen = (len / 2);
    *outputLen = finalLen;
    for (size_t inIdx = 0, outIdx = 0; outIdx < finalLen; inIdx += 2, outIdx++)
    {
        if ((hexStr[inIdx] - 48) <= 9 && (hexStr[inIdx + 1] - 48) <= 9)
        {
            goto convert;
        }
        else
        {
            if ((hexStr[inIdx] - 65) <= 5 && (hexStr[inIdx + 1] - 65) <= 5)
            {
                goto convert;
            }
            else
            {
                *outputLen = 0;
                return -1;
            }
        }
    convert:
        output[outIdx] =
            (hexStr[inIdx] % 32 + 9) % 25 * 16 + (hexStr[inIdx + 1] % 32 + 9) % 25;
    }
    return (int)finalLen;
}

int HexStringToBytes_delim(const char *hexStr, char *output, int *outputLen)
{
    // force the string to uppercase:
    strupr((char *)hexStr);

    size_t len = strlen(hexStr);
    //Cleanup the string ending
    int i = len;
    uint8_t _tst = 0;
    do
    {
        _tst = (uint8_t)hexStr[i];
        //check if number 0 - 9
        if ((_tst >= 48) && (_tst <= 57))break;
        //check if letter A - F
        if ((_tst >= 65) && (_tst <= 70)) break;
        i--;
    } while (i >0);


    char delim_str [i+1];
    memcpy(&delim_str, (char *)&hexStr[0], i+1);
    delim_str[i+1] = '\0';
    strcat((char *)&delim_str, ":");
    //redo the length 
    len = strlen((char *)&delim_str);

    if (len % 3 != 0)
    {
        return -1;
    }

    size_t finalLen = (len / 3);
    *outputLen = finalLen;
    for (size_t inIdx = 0, outIdx = 0; outIdx < finalLen; inIdx += 3, outIdx++)
    {
        if ((delim_str[inIdx] - 48) <= 9 && (delim_str[inIdx + 1] - 48) <= 9)
        {
            goto convert;
        }
        else
        {
            if ((delim_str[inIdx] - 65) <= 5 && (delim_str[inIdx + 1] - 65) <= 5)
            {
                goto convert;
            }
            else
            {
                *outputLen = 0;
                return -1;
            }
        }
    convert:
        output[outIdx] =
            (delim_str[inIdx] % 32 + 9) % 25 * 16 + (delim_str[inIdx + 1] % 32 + 9) % 25;
    }
    return (int)finalLen;
}

int Hex2Bytes(const char *hexStr, char *output, int *outputLen)
{
    // force the string to uppercase:
    strupr((char *)hexStr);

    //remove any non hex characters
    int len = strlen(hexStr);
    char _temp[len];
    memset(&_temp, 0, len);
    int j = 0;
    for(int i = 0; i < len; i++)
    {
        char _tst = hexStr[i];
        if ((_tst >= 48) && (_tst <= 57))
        {
            _temp[j++] = _tst;
        }
        //check if letter A - F
        if ((_tst >= 65) && (_tst <= 70))
        {
            _temp[j++] = _tst;
        }
    }
    //Belt and braces - add a \0 at the last position
    _temp[j] = '\0';

    //Now send the array for processing...
    return HexStringToBytes((char *)&_temp, output, outputLen);
}

int BytesToHexString_hyp_delim(char *pt_out_str, const uint8_t *pt_in_bytes, const uint8_t input_length)
{
    char c[(input_length * 3) + 2];
    memset(&c, 0, sizeof(c));
    uint8_t b;

    for(int bx = 0, cx = 0; bx < input_length; ++bx, ++cx)
    {
        b = ((uint8_t)(pt_in_bytes[bx] >> 4));
        c[cx] = (char)(b > 9 ? b - 10 + 'A' : b + '0');

        b = ((uint8_t)(pt_in_bytes[bx] & 0x0F));
        c[++cx] = (char)(b > 9 ? b - 10 + 'A' : b + '0');

        //add a hyphen between the elements
        c[++cx] = '-';
    }
    //add 0x00 to the end to make it a c string
    c[(input_length * 3) - 1] = 0x0; // == \0
    //transfer it to the output string lostion
    memcpy(pt_out_str, c, strlen(c));

    return strlen(pt_out_str);
}

int BytesToHexString(char *pt_out_str, const uint8_t *pt_in_bytes, const uint8_t input_length)
{
    char c[(input_length * 2) + 1];
    memset(&c, 0, sizeof(c));
    uint8_t b;

    for(int bx = 0, cx = 0; bx < input_length; ++bx, ++cx)
    {
        b = ((uint8_t)(pt_in_bytes[bx] >> 4));
        c[cx] = (char)(b > 9 ? b - 10 + 'A' : b + '0');

        b = ((uint8_t)(pt_in_bytes[bx] & 0x0F));
        c[++cx] = (char)(b > 9 ? b - 10 + 'A' : b + '0');
    }
    //add 0x00 to the end to make it a c string
    c[input_length * 2] = 0x0; // == \0
    //transfer it to the output string lostion
    memcpy(pt_out_str, c, strlen(c));

    return strlen(pt_out_str);
}

/******************************************************************************************/
// UART
/******************************************************************************************/

void init_uart(void) 
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,          // 8
        .parity = UART_PARITY_DISABLE,          // N
        .stop_bits = UART_STOP_BITS_1,          // 1
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,  // none
        .source_clk = UART_SCLK_APB,
    };

    // UART event queue is initialised and enabled here xqUART_events ...
    uart_driver_install(EX_UART_NUM, UART_BUFFER_SIZE, UART_BUFFER_SIZE, UART_EVENT_QUEUE_DEPTH, &xqUART_events, 0);
    uart_param_config(EX_UART_NUM, &uart_config);
#if (PATTERN_DETECTION)
    //Set uart pattern detect function.
    uart_enable_pattern_det_baud_intr(EX_UART_NUM, PATTERN_CHR, PATTERN_CHR_NUM, 2, 0, 0);
    //Reset the pattern queue length to record at most 20 pattern positions.
    uart_pattern_queue_reset(EX_UART_NUM, PATTERN_QUEUE_SIZE);
#else
    uart_disable_pattern_det_intr(EX_UART_NUM);
#endif
    // Set UART pins
    uart_set_pin(EX_UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // // Setup the Rx and Tx Logging levels
    // // Do this here rather than at the task creation line...
    // esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    // esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    // esp_log_level_set(UART_ISR_TASK_TAG, ESP_LOG_INFO);
    

    // Start the tasks running here:
    // Tx Task - held with TX Queue data available
    xTaskCreate(tx_uart_task, TX_TASK_TAG, 1024*3, NULL, configMAX_PRIORITIES, &xHandle_uart_tx);
    // Rx Task - held with RX Queue - data ready for processing
    // Rx Task stack size larger because it will be responsible for frame assembly and marshalling
    // same applies to the event stack below...
    xTaskCreate(rx_uart_task, RX_TASK_TAG, 1024*5, NULL, configMAX_PRIORITIES-1, &xHandle_uart_rx);
    // Create a task to handler UART event from ISR - JIT managed - better !
    xTaskCreate(uart_event_task, UART_ISR_TASK_TAG, 1024*5, NULL, configMAX_PRIORITIES-2, &xHandle_uart_isr);
}

void tx_uart_task(void *arg)
{
#ifdef STACK_MONITOR
    /* Inspect our own high water mark on entering the task. */
    uxHighWaterMark_TX = uxTaskGetStackHighWaterMark(NULL);
    printf("UART TX STACK HW (START) = %d\r\n", uxHighWaterMark_TX);
#endif

    // Initialise things here
    static bool tx_error = false;
    static int bin_len = 0;

    static char byte_buffer[UART_BUFFER_SIZE];
    memset(&byte_buffer, 0, sizeof(byte_buffer));

    static uart_message_t _tx_message;
    static uart_message_t *tx_message = &_tx_message;
    memset(tx_message, 0, sizeof(_tx_message));

    int txBytes;

    // UART TX buffer Queue
    xqUART_tx = xQueueCreate(UART_TX_QUEUE_DEPTH, sizeof(uart_message_t));

    // //wait for all required tasks to come online
    // while (
    //            (xqUART_tx == NULL)
    //         || (xqUART_rx == NULL)
    //         || (xHandle_uart_tx == NULL) 
    //         || (xHandle_uart_rx == NULL) 
    //         || (xHandle_uart_isr == NULL)
    //         || (xHandle_blink == NULL)
    //       )
    // {
    //     // wait 1 second then check again
    //     vTaskDelay(100 / portTICK_PERIOD_MS);
    // }
    
    while (1) 
    {
        //Clear the byte send buffer and binary length ready for the next message
        memset(&byte_buffer, 0, sizeof(byte_buffer));
        bin_len = 0;

        // Block until a message is put on the queue
        if (xQueueReceive(xqUART_tx, tx_message, (TickType_t)portMAX_DELAY) == pdPASS)
        {
            //Message to be transmitted on the COMM port is... 
            if ((tx_message->IsASCII) && (tx_message->IsHEX))
            {
                //Parse message and send out in binary (hex) format
                Hex2Bytes((char *)&tx_message->data, byte_buffer, &bin_len);
                //Update tx length - it has been changed:
                tx_message->length = bin_len;
            }
            else
            {
                //Directly transmit the character string or hex (binary) data
                memcpy(&byte_buffer, tx_message->data, tx_message->length);
            }

            //Write the buffer out to the UART
            txBytes = uart_write_bytes(tx_message->port, byte_buffer, tx_message->length);

            tx_error = (txBytes != tx_message->length);     //NB: update the txmessage length after decoding to a byte array (binary data)
  
            // Flash LED to show activity
            xSemaphoreGive(bin_s_sync_blink_task);

            // Block / hold task for a fixed minimum time to prevent the receiver being overrun... (RM200x)
            int tx_delay_ms = UART_FIXED_MIN_TX_DELAY_MS + UART_VARIABLE_TX_DELAY;
                vTaskDelay(tx_delay_ms / portTICK_PERIOD_MS);    //5 ms for now
            // Log if necessary
            ESP_LOGI(TX_TASK_TAG, "UART TX - ID: %d, Port: %d, Data length: %d, Message dwell: %d mS\r\nData: %s\r\n", 
                tx_message->Message_ID, tx_message->port, tx_message->length, tx_delay_ms, tx_message->data);

            if (tx_error)
            {
                //ERROR - so do something here
                ESP_LOGE(TX_TASK_TAG, "ERROR - INCOMPLETE MESSAGE SENT: Bytes written = %d\r\nUART TX - ID: %d, Port: %d, Data length: %d, Message dwell: %d mS\r\nData: %s\r\n", 
                        txBytes, tx_message->Message_ID, tx_message->port, tx_message->length, tx_delay_ms, tx_message->data);
                printf("UART ERROR - INCOMPLETE MESSAGE SENT\r\n");

                //clear the error flag
                tx_error = false;
            }
        }
        #ifdef STACK_MONITOR
            uxHighWaterMark_TX = uxTaskGetStackHighWaterMark( NULL );
            printf("UART TX STACK HW (RUN): %d\r\n", uxHighWaterMark_TX);
        #endif
    }

    free(xqUART_tx);
    vTaskDelete(xHandle_uart_tx);
}

void rx_uart_task(void *arg)
{
#ifdef STACK_MONITOR
    /* Inspect our own high water mark on entering the task. */
    uxHighWaterMark_RX = uxTaskGetStackHighWaterMark(NULL);
    printf("UART RX STACK HW (START) = %d\r\n", uxHighWaterMark_RX);
#endif

    // Initialise things here
    static uart_message_t _rx_message;
    static uart_message_t *rx_message = &_rx_message;
    memset(rx_message, 0, sizeof(_rx_message));

    static tcpip_message_t _tcpip_msg;
    static tcpip_message_t *tcpip_msg = &_tcpip_msg;
    memset(tcpip_msg, 0, sizeof(tcpip_message_t));

    // Local variable to allow sample ACK message to be sent
    static mqtt_esp_message_t _mqtt_tx;
    static mqtt_esp_message_t *mqtt_tx = &_mqtt_tx;
    memset(mqtt_tx, 0, sizeof(_mqtt_tx));

    char rx_string[UART_BUFFER_SIZE];

    // UART IO buffer Queues
    xqUART_rx = xQueueCreate(UART_RX_QUEUE_DEPTH, sizeof(uart_message_t));

    while (1)
    {
        // Clear the input container ready for a new message
        memset(rx_message, 0, sizeof(_rx_message));
        // Will block until a message is put on the queue
        if (xQueueReceive(xqUART_rx, rx_message, (TickType_t)portMAX_DELAY) == pdPASS)
        {
            // Choose if ASCII or binary (HEX)
            // If first char is 0xFF - it will be a hex string coming in (radio application)
            if (rx_message->data[0] == 0xFF)
            {
                if (ALL_ASCII)
                {
                    rx_message->IsASCII = true;
                    rx_message->IsHEX = false;
                    rx_message->IsFrame = false;
                }
                else
                {
                    rx_message->IsASCII = false;
                    rx_message->IsHEX = true;
                    rx_message->IsFrame = false;
                }

                if (rx_message->data[1] == 0x55)
                {
                    rx_message->IsASCII = false;
                    rx_message->IsHEX = true;
                    rx_message->IsFrame = true;
                }
            }

            if (rx_message->IsFrame)
            {
                // Deal with the incomming frame
                xQueueSend(xqFrame_Rx, &(rx_message->data), (TickType_t)0);
                ESP_LOG_BUFFER_HEXDUMP(TX_TASK_TAG, &(rx_message->data), 8, ESP_LOG_WARN);

                // Flash LED to show activity
                xSemaphoreGive(bin_s_sync_blink_task);
            }

            int len_str = 0;
            if (!rx_message->IsFrame)
            {
                if (rx_message->IsHEX)
                {
                    if (HEX_DELIMITED)
                    {
                        // PROCESS INCOMMING UART BYTE MESSAGE STRING:
                        // clear string buffer
                        memset(&rx_string, 0, sizeof(rx_string));
                        len_str = BytesToHexString_hyp_delim((char *)&rx_string, (uint8_t *)&rx_message->data, rx_message->length);
                        // send this out on the MQTT tele channel

                        // start with a clean structure
                        memset(mqtt_tx, 0, sizeof(mqtt_esp_message_t));
                        // start building
                        mqtt_tx->msg_id = 0; // set counter on send in Tx task, set to zero for now
                        mqtt_tx->qos = 0;    // set required QoS level here
                        // set the topic here
                        mqtt_tx->topic_len = build_topic((mqtt_topic_t *)&mqtt_tx->topic_detail, MQTT_TELE_PREFIX, MQTT_BASE_TOPIC, "hex_delimited");
                        strcpy((char *)&mqtt_tx->topic, (char *)&mqtt_tx->topic_detail.topic);
                        mqtt_tx->data_len = len_str; // strlen(input_string);
                        mqtt_tx->total_data_len = mqtt_tx->data_len;
                        memcpy(&mqtt_tx->data, &rx_string, mqtt_tx->data_len);
                        // SEND
                        mqtt_send(mqtt_tx);
                    }
                    else
                    {
                        // NON-DELIMITED VERSION
                        // clear string buffer
                        memset(&rx_string, 0, sizeof(rx_string));
                        len_str = BytesToHexString((char *)&rx_string, (uint8_t *)&rx_message->data, rx_message->length);
                        // send this out on the MQTT tele channel
                        // start with a clean structure
                        memset(mqtt_tx, 0, sizeof(mqtt_esp_message_t));
                        // start building
                        mqtt_tx->msg_id = 0; // set counter on send in Tx task, set to zero for now
                        mqtt_tx->qos = 0;    // set required QoS level here
                        // set the topic here
                        mqtt_tx->topic_len = build_topic((mqtt_topic_t *)&mqtt_tx->topic_detail, MQTT_TELE_PREFIX, MQTT_BASE_TOPIC, "hex");
                        strcpy((char *)&mqtt_tx->topic, (char *)&mqtt_tx->topic_detail.topic);
                        mqtt_tx->data_len = len_str; // strlen(input_string);
                        mqtt_tx->total_data_len = mqtt_tx->data_len;
                        memcpy(&mqtt_tx->data, &rx_string, mqtt_tx->data_len);

                        // SEND
                        mqtt_send(mqtt_tx);
                    }
                }
                else
                {
                    // TEXT VERSION:
                    // start with a clean structure
                    memset(mqtt_tx, 0, sizeof(mqtt_esp_message_t));
                    // start building
                    mqtt_tx->msg_id = 0; // set counter on send in Tx task, set to zero for now
                    mqtt_tx->qos = 0;    // set required QoS level here
                    // set the topic here
                    mqtt_tx->topic_len = build_topic((mqtt_topic_t *)&mqtt_tx->topic_detail, MQTT_TELE_PREFIX, MQTT_BASE_TOPIC, "xxx ascii");
                    strcpy((char *)&mqtt_tx->topic, (char *)&mqtt_tx->topic_detail.topic);
                    mqtt_tx->data_len = rx_message->length; // strlen(input_string);
                    mqtt_tx->total_data_len = mqtt_tx->data_len;
                    memcpy(&mqtt_tx->data, (char *)&rx_message->data, mqtt_tx->data_len);
                    // SEND
                    mqtt_send(mqtt_tx);
                }
            }
        }
#ifdef STACK_MONITOR
        uxHighWaterMark_RX = uxTaskGetStackHighWaterMark(NULL);
        printf("UART RX STACK HW (RUN): %d\r\n", uxHighWaterMark_RX);
#endif
    }

    free(xqUART_rx);
    vTaskDelete(xHandle_uart_rx);
}

void uart_event_task(void *pvParameters)
{
#ifdef STACK_MONITOR
    /* Inspect our own high water mark on entering the task. */
    uxHighWaterMark_EV = uxTaskGetStackHighWaterMark(NULL);
    printf("UART EV STACK HW (START) = %d\r\n", uxHighWaterMark_EV);
#endif

    uart_event_t event;
    uart_message_t tmp_rx;
    uint8_t *dtmp = (uint8_t *)malloc(UART_BUFFER_SIZE + 1);

    while (1)
    {
        // bzero(dtmp, UART_BUFFER_SIZE);
        // Clear all local buffers / containers ready for the next event
        memset(&tmp_rx, 0x00, sizeof(uart_message_t));
        memset(dtmp, 0x00, sizeof(*dtmp));

        // Waiting for UART event.
        if (xQueueReceive(xqUART_events, (void *)&event, (portTickType)portMAX_DELAY))
        {
            ESP_LOGI(UART_ISR_TASK_TAG, "uart[%d] event:", EX_UART_NUM);
            switch (event.type)
            {
            // Event of UART receving data
            /*We'd better handle the data event fast, there would be much more data events than
            other types of events. If we take too much time on data event, the queue might
            be full.*/
            case UART_DATA:
                ESP_LOGI(UART_ISR_TASK_TAG, "[UART DATA]: %d", event.size);
                
                if (xqUART_rx != NULL)
                {
                    // Clear buffer
                    memset(&tmp_rx, 0x00, sizeof(uart_message_t));
                    // Read in the available data
                    tmp_rx.length = uart_read_bytes(EX_UART_NUM, &tmp_rx.data, event.size, UART_RX_READ_TIMEOUT_MS / portTICK_PERIOD_MS);   // (TickType_t)portMAX_DELAY);
                    tmp_rx.port = EX_UART_NUM;
                    // Send out on to the RX queue - for further processing (don't wait for the buffer to be free)
                    ESP_LOG_BUFFER_HEXDUMP(UART_ISR_TASK_TAG, &tmp_rx.data, tmp_rx.length, ESP_LOG_INFO);
                    xQueueSend(xqUART_rx, &tmp_rx, (TickType_t)0);
                }
                else
                {
                    //Flush the RX buffer to keep it clean
                    uart_flush_input(EX_UART_NUM);
                }
                break;
            // Event of HW FIFO overflow detected
            case UART_FIFO_OVF:
                ESP_LOGI(UART_ISR_TASK_TAG, "hw fifo overflow");
                // If fifo overflow happened, you should consider adding flow control for your application.
                // The ISR has already reset the rx FIFO,
                // As an example, we directly flush the rx buffer here in order to read more data.
                reg_delay();
                uart_flush_input(EX_UART_NUM);
                xQueueReset(xqUART_events);
                break;
            // Event of UART ring buffer full
            case UART_BUFFER_FULL:
                ESP_LOGI(UART_ISR_TASK_TAG, "ring buffer full");
                // If buffer full happened, you should consider encreasing your buffer size
                // As an example, we directly flush the rx buffer here in order to read more data.
                reg_delay();
                uart_flush_input(EX_UART_NUM);
                xQueueReset(xqUART_events);
                break;
            // Event of UART RX break detected
            case UART_BREAK:
                ESP_LOGI(UART_ISR_TASK_TAG, "uart rx break");
                break;
            // Event of UART parity check error
            case UART_PARITY_ERR:
                ESP_LOGI(UART_ISR_TASK_TAG, "uart parity error");
                break;
            // Event of UART frame error
            case UART_FRAME_ERR:
                ESP_LOGI(UART_ISR_TASK_TAG, "uart frame error");
                break;
            // UART_PATTERN_DET
            #if (PATTERN_DETECTION)
            case UART_PATTERN_DET:
                size_t buffered_size;
                uart_get_buffered_data_len(EX_UART_NUM, &buffered_size);
                int pos = uart_pattern_pop_pos(EX_UART_NUM);
                ESP_LOGI(UART_ISR_TASK_TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                if (pos == -1)
                {
                    // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
                    // record the position. We should set a larger queue size.
                    // As an example, we directly flush the rx buffer here.
                    PATTERN_0xFF_DETECTED = false;
                    reg_delay();
                    uart_flush_input(EX_UART_NUM);
                }
                else
                {
                    //Pop th position off the queue
                    int pos = uart_pattern_pop_pos(EX_UART_NUM);
                    //Reset i.e. clear the position queue so it does not break
                    uart_pattern_queue_reset(EX_UART_NUM, PATTERN_QUEUE_SIZE);
                    PATTERN_0xFF_DETECTED = true; 
                }
                break;
            #endif
            // Others
            default:
                ESP_LOGI(UART_ISR_TASK_TAG, "uart event type: %d", event.type);
                break;
            }
        }
#ifdef STACK_MONITOR
        uxHighWaterMark_EV = uxTaskGetStackHighWaterMark(NULL);
        printf("UART EV STACK HW (RUN): %d\r\n", uxHighWaterMark_EV);
#endif
    }
    free(dtmp);
    dtmp = NULL;
    free(xqUART_events);
    vTaskDelete(xHandle_uart_isr);
}

/******************************************************************************************/
// RM200x UART  -- END --
/******************************************************************************************/