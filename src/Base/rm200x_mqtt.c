#include "base/rm200x_mqtt.h"

// Initialise the globals:
esp_mqtt_client_handle_t mqtt_client = NULL;

//FREERTOS CONTROL ITEMS
TaskHandle_t xHandle_rx_mqtt = NULL;
TaskHandle_t xHandle_tx_mqtt = NULL;

QueueHandle_t xqMQTT_rx_Messages = NULL;
QueueHandle_t xqMQTT_tx_Messages = NULL;

//Task Tags
const char *MQTT_RX_TASK_TAG    = "MQTT_RX_TASK";
const char *MQTT_TX_TASK_TAG    = "MQTT_TX_TASK";
const char *MQTT_TASK_TAG       = "MQTT_TASK";

//Local prototypes
void mqtt_rx_task(void *arg);
void mqtt_tx_task(void *arg);

//Module wide variables
//#define STACK_MONITOR   1
#ifdef STACK_MONITOR
UBaseType_t uxHighWaterMark_RX;
UBaseType_t uxHighWaterMark_TX;
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
// HELPERS
/******************************************************************************************/

int str_cut(char *str, int begin, int len)
{
/*
 *      Remove given section from string. Negative len means remove
 *      everything up to the end.
 */
    int l = strlen(str);

    if (len < 0) len = l - begin;
    if (begin + len > l) len = l - begin;
    memmove(str + begin, str + begin + len, l - len + 1);

    return len;
}

void copy_message_strut(mqtt_esp_message_t *pt_dest, mqtt_esp_message_t *pt_source)
{
    memcpy(pt_dest, pt_source, sizeof(mqtt_esp_message_t));
}

void clear_message_strut(mqtt_esp_message_t *pt_dest)
{
    memset(pt_dest, 0, sizeof(mqtt_esp_message_t));
}

void convert_mqtt_input_message(mqtt_esp_message_t *pt_dest, esp_mqtt_event_t *pt_source)
{
    //Clear the message container first
    memset (pt_dest, 0, sizeof(*pt_dest));
    //then copy in the new data
    memcpy(&pt_dest->data, pt_source->data, pt_source->data_len);
    pt_dest->data_len = pt_source->data_len;
    pt_dest->total_data_len = pt_source->total_data_len;
    pt_dest->current_data_offset =pt_source->current_data_offset;
    memcpy(&pt_dest->topic, pt_source->topic, pt_source->topic_len);
    pt_dest->topic_len = pt_source->topic_len;
    pt_dest->msg_id = pt_source->msg_id;
    pt_dest->qos = 0;                               //Modify before TX if necessary
    pt_dest->retain = pt_source->retain;
}

int build_topic(mqtt_topic_t *pt_topic, const char *prefix, const char *topic, const char *suffix)
{
    // Clear any data in pt_topic
    memset(pt_topic, 0, sizeof(*pt_topic));

    //Set up the topic elements
    strcpy((char *)&pt_topic->prefix, prefix);
    strcpy((char *)&pt_topic->base_topic, topic);
    strcpy((char *)&pt_topic->suffix, suffix);

    // Build / concatenate the topic items
    strcat((char *)&pt_topic->topic, "/");
    strcat((char *)&pt_topic->topic, prefix);
    if(strcmp(prefix, "") != 0) strcat((char *)&pt_topic->topic, "/");
    strcat((char *)&pt_topic->topic, topic);
    if(strcmp(topic, "") != 0) strcat((char *)&pt_topic->topic, "/");
    strcat((char *)&pt_topic->topic, suffix);
    if(strcmp(suffix, "") != 0) strcat((char *)&pt_topic->topic, "/");
    strcat((char *)&pt_topic->topic, "\0");

    int length = (int)strlen(pt_topic->topic);
    ESP_LOGI(MQTT_TASK_TAG, "COMPLETE TOPIC = %d, %s", length, (char *)pt_topic->topic);

    return length;
}

int complete_topic(mqtt_topic_t *pt_topic_data)
{
    // Clear any data in pt_topic
    memset(&pt_topic_data->topic, 0, sizeof(pt_topic_data->topic));
    // Build / concatenate the topic items
    strcat(pt_topic_data->topic, "/");
    strcat(pt_topic_data->topic, pt_topic_data->prefix);
    if(strcmp(pt_topic_data->prefix, "") == 0) strcat(pt_topic_data->topic, "/");
    strcat(pt_topic_data->topic, pt_topic_data->base_topic);
    if(strcmp(pt_topic_data->topic, "") == 0) strcat(pt_topic_data->topic, "/");
    strcat(pt_topic_data->topic, "/");
    strcat(pt_topic_data->topic, pt_topic_data->suffix);
    strcat(pt_topic_data->topic, "/\0");

    int length = (int)strlen(pt_topic_data->topic);

    ESP_LOGI(MQTT_TASK_TAG, "COMPLETE TOPIC = %d, %s", length, (char *)pt_topic_data->topic);

    return length;
}

void print_topic(mqtt_topic_t *pt_topic_data)
{
    printf("RX TOPIC: %s\r\n", pt_topic_data->topic); 
    printf("PREFIX: %s\r\n", pt_topic_data->prefix); 
    printf("BASE: %s\r\n", pt_topic_data->base_topic);
    printf("SUFFIX: %s\r\n", pt_topic_data->suffix); 
}

int parse_mqtt_topic_string(mqtt_topic_t *pt_topic_data)
{
    char *_temp_result;
    char delimiters[] = "/";

    // PREFIX
    _temp_result = strtok(pt_topic_data->topic, delimiters);
    if (_temp_result == NULL)
    {
        return 0; // No delimiter characters found
    }
    else
    {
        strcpy(pt_topic_data->prefix, _temp_result);
    }
    // BASE TOPIC
    _temp_result = strtok(NULL, delimiters);
    if (_temp_result == NULL)
    {
        return 1; // No more things
    }
    else
    {
        strcpy(pt_topic_data->base_topic, _temp_result);
    }
    // BASE SUFFIX
    _temp_result = strtok(NULL, delimiters);
    if (_temp_result == NULL)
    {
        return 2; // No more things
    }
    else
    {
        strcpy(pt_topic_data->suffix, _temp_result); 
    }
    return 3;
}

/******************************************************************************************/
// COMMAND PROCESSOR cmnd/%topic%/%suffix% ...
/******************************************************************************************/

void CommandStatusMessage(mqtt_esp_message_t *cmnd_message)
{
    memcpy(cmnd_message->topic_detail.topic, cmnd_message->topic, cmnd_message->topic_len);
    parse_mqtt_topic_string((mqtt_topic_t *)&cmnd_message->topic_detail);

    // start building
    cmnd_message->msg_id = 0; // set counter on send in Tx task, set to zero for now
    cmnd_message->qos = 0;    // set required QoS level here
    // set the topic here
    cmnd_message->topic_len = build_topic((mqtt_topic_t *)&cmnd_message->topic_detail, MQTT_STATUS_PREFIX, cmnd_message->topic_detail.base_topic, cmnd_message->topic_detail.suffix);
    strcpy((char *)&cmnd_message->topic, (char *)&cmnd_message->topic_detail.topic);
    strcat((char *)&cmnd_message->data, " - DONE");
    cmnd_message->data_len = strlen(cmnd_message->data);
    cmnd_message->total_data_len = cmnd_message->data_len;
    // SEND
    // send pointer to structure of the outgoing message
    xQueueSend(xqMQTT_tx_Messages, cmnd_message, (TickType_t)0);
    // Flash LED to show activity
    xSemaphoreGive(bin_s_sync_blink_task);
}

void CommandProcessor(mqtt_esp_message_t *mqtt_in)
{
    if (strcmp(mqtt_in->topic_detail.prefix, MQTT_CMND_PREFIX) == 0)
    {
        //COMMAND
        if (strcmp(mqtt_in->data, "ALL ASCII ON") == 0)
        {
            ALL_ASCII = true;
        }
        if (strcmp(mqtt_in->data, "ALL ASCII OFF") == 0)
        {
            ALL_ASCII = false;
        }
        if (strcmp(mqtt_in->data, "ALL ASCII TOGGLE") == 0)
        {
            ALL_ASCII = !ALL_ASCII;
        }
        if (strcmp(mqtt_in->data, "HEX DELIMIT OFF") == 0)
        {
            HEX_DELIMITED = false;
        }
        if (strcmp(mqtt_in->data, "HEX DELIMIT ON") == 0)
        {
            HEX_DELIMITED = true;
        }
        if (strcmp(mqtt_in->data, "HEX DELIMIT TOGGLE") == 0)
        {
            HEX_DELIMITED = !HEX_DELIMITED;
        }
        //Send an acknowledgement status message
        CommandStatusMessage(mqtt_in); 
    }
}

/******************************************************************************************/
// MQTT
/******************************************************************************************/

void log_error_if_nonzero(const char * message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(MQTT_RX_TASK_TAG, "Last error %s: 0x%x", message, error_code);
    }
}

esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    // Initialise things here
    static mqtt_esp_message_t _mqtt_in;
    mqtt_esp_message_t *mqtt_in = &_mqtt_in;
    memset(mqtt_in, 0, sizeof(struct st_MQTT_MESSAGE));

    //esp_mqtt_client_handle_t client = event->client;
    //int msg_id;

    // your_context_t *context = event->context;
    switch (event->event_id) 
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(MQTT_RX_TASK_TAG, "MQTT_EVENT_CONNECTED");
            // msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data = connected - QoS 1", 0, 1, 0);
            // ESP_LOGI(MQTT_RX_TASK_TAG, "sent publish successful, msg_id=%d", msg_id);

            // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
            // ESP_LOGI(MQTT_RX_TASK_TAG, "sent subscribe successful, msg_id=%d", msg_id);

            // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            // ESP_LOGI(MQTT_RX_TASK_TAG, "sent subscribe successful, msg_id=%d", msg_id);

            // msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            // ESP_LOGI(MQTT_RX_TASK_TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(MQTT_RX_TASK_TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(MQTT_RX_TASK_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            // msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            // ESP_LOGI(MQTT_RX_TASK_TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(MQTT_RX_TASK_TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(MQTT_RX_TASK_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(MQTT_RX_TASK_TAG, "MQTT_EVENT_DATA");
             if (xHandle_rx_mqtt != NULL)
             {
                // Copy the incomming message to the local
                convert_mqtt_input_message(mqtt_in, event);
                // Send out the incomming data for processing
                xQueueSend(xqMQTT_rx_Messages, mqtt_in, (TickType_t) 0);
             }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(MQTT_RX_TASK_TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
                log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(MQTT_RX_TASK_TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;
        default:
            ESP_LOGI(MQTT_RX_TASK_TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) 
{
    ESP_LOGD(MQTT_RX_TASK_TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

void mqtt_app_init(void)
{
    //Wait for a wifi connection before even starting:
    xSemaphoreTake(bin_s_sync_wifi_connected, portMAX_DELAY);
    // Set logging level
    //esp_log_level_set(MQTT_RX_TASK_TAG, ESP_LOG_WARN);

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = MQTT_SERVER,
        .username = MQTT_USER,
        .password = MQTT_PASSWORD,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
 
    //Register events
    esp_err_t esp_error_events = esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_client);
    while (esp_error_events != ESP_OK)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_error_events = esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_client);
    }
    //Start the MQTT client
    esp_err_t esp_error_start = esp_mqtt_client_start(mqtt_client);
    while (esp_error_start != ESP_OK)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_error_start = esp_mqtt_client_start(mqtt_client);
    }
    //Connected but let everything settle down
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  
    //Subcribe to the Command channel
    int stat = esp_mqtt_client_subscribe(mqtt_client, MQTT_SUBSCRIPTION_TOPIC, 0); 
    while (stat < 0)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        stat = esp_mqtt_client_subscribe(mqtt_client, MQTT_SUBSCRIPTION_TOPIC, 0);
    }

    // Start the MQTT tasks here :
    xTaskCreate(mqtt_rx_task, MQTT_RX_TASK_TAG, 1024*3, NULL, configMAX_PRIORITIES, &xHandle_rx_mqtt);
    xTaskCreate(mqtt_tx_task, MQTT_TX_TASK_TAG, 1024*3, NULL, configMAX_PRIORITIES, &xHandle_tx_mqtt);

    //All initialised - give the wifi semaphore back
    xSemaphoreGive(bin_s_sync_wifi_connected);
}

void mqtt_rx_task(void *arg)
{
    #ifdef STACK_MONITOR
        /* Inspect our own high water mark on entering the task. */
        uxHighWaterMark_RX = uxTaskGetStackHighWaterMark( NULL );
        printf("MQTT RX STACK HW (START) = %d\r\n", uxHighWaterMark_RX);
    #endif

    // Initialise things here
    static mqtt_esp_message_t _mqtt_rx;
    static mqtt_esp_message_t *mqtt_rx = &_mqtt_rx;
    memset(mqtt_rx, 0, sizeof(_mqtt_rx));

    // MQTT IO buffer Queue
    xqMQTT_rx_Messages = xQueueCreate(MQTT_Q_DEPTH, sizeof(_mqtt_rx));

    while (1)
    {
        //Clear the MQTT message holder ready for a new message
        memset(mqtt_rx, 0, sizeof(_mqtt_rx));
        //Wait for a new message
        if (xQueueReceive(xqMQTT_rx_Messages, mqtt_rx, (TickType_t)portMAX_DELAY) == pdPASS) // Hold until data received on queue
        {
            /**************************************************************************************************/
            /*  GET PREFIX, TOPIC AND SUFFIX FROM THE RECEIVED MESSAGE                                        */
            /**************************************************************************************************/
            //Complete the incomming message
            memcpy((char *)&mqtt_rx->topic_detail.topic, mqtt_rx->topic, mqtt_rx->topic_len);
            parse_mqtt_topic_string((mqtt_topic_t *)&mqtt_rx->topic_detail);

            // Flash LED to show activity
            xSemaphoreGive(bin_s_sync_blink_task);

            /**************************************************************************************************/
            /*  CHECK THE COMMAND PROCESSOR AND RUN IF REQUIRED                                               */
            /**************************************************************************************************/
            if (strstr((char *)&mqtt_rx->topic_detail.topic, MQTT_CMND_PREFIX) != NULL)
            {
                ESP_LOGI(MQTT_RX_TASK_TAG, "COMMAND");
                CommandProcessor(mqtt_rx);
                //Reply will be in the form of a status message
                continue;
            }
            /**************************************************************************************************/
            /*  DO SOMETHING WITH THE RECEIVED FRAME HEX MESSAGE                                              */
            /**************************************************************************************************/
            if (strcmp((char *)&mqtt_rx->topic_detail.prefix, MQTT_HEX_FRAME_PREFIX) == 0)
            {
                ESP_LOGI(MQTT_RX_TASK_TAG, "COMM PORT TX - FRAME");
                ESP_LOG_BUFFER_HEXDUMP(MQTT_RX_TASK_TAG, &mqtt_rx->data, strlen(mqtt_rx->data), ESP_LOG_WARN);
                // Send MQTT message to frame Tx
                xQueueSend(xqFrame_Tx, &mqtt_rx->data, (TickType_t)0);

                // // Send a copy of the MQTT message out on the UART

                // static uart_message_t uart_tx_msg;
                // memset(&uart_tx_msg, 0, sizeof(uart_message_t));

                // uart_tx_msg.Message_ID = mqtt_rx->msg_id;
                // uart_tx_msg.port = EX_UART_NUM;
                // uart_tx_msg.IsHEX = true;           // TREAT DATA AS HEX
                // uart_tx_msg.IsASCII = true;         // It IS an ASCII representation of HEX
                // uart_tx_msg.IsFrame = true;
                // int len = strlen(mqtt_rx->data);
                // if (len > UART_BUFFER_SIZE)
                // {
                //     uart_tx_msg.length = UART_BUFFER_SIZE; //(int)sizeof(uart_tx_msg.data);
                // }
                // else
                // {
                //     uart_tx_msg.length = len;
                // }
                // memcpy(&uart_tx_msg.data, mqtt_rx->data, uart_tx_msg.length); // Only copy what will fit...
                // // Place our data on the UART tx queue
                // xQueueSend(xqUART_tx, &uart_tx_msg, (TickType_t)0);
                continue;
            }
            /**************************************************************************************************/
            /*  DO SOMETHING WITH THE RECEIVED COMM HEX MESSAGE                                               */
            /**************************************************************************************************/
            if (strcmp((char *)&mqtt_rx->topic_detail.prefix, MQTT_HEX_COMM_PREFIX) == 0)
            {
                ESP_LOGI(MQTT_RX_TASK_TAG, "COMM PORT TX - HEX");
                // Send a copy of the MQTT message out on the UART
                static uart_message_t uart_tx_msg;
                memset(&uart_tx_msg, 0, sizeof(uart_message_t));

                uart_tx_msg.Message_ID = mqtt_rx->msg_id;
                uart_tx_msg.port = EX_UART_NUM;
                uart_tx_msg.IsHEX = true;           // TREAT DATA AS HEX
                uart_tx_msg.IsASCII = true;         // It IS an ASCII representation of HEX
                int len = strlen(mqtt_rx->data);
                if (len > UART_BUFFER_SIZE)
                {
                    uart_tx_msg.length = UART_BUFFER_SIZE; //(int)sizeof(uart_tx_msg.data);
                }
                else
                {
                    uart_tx_msg.length = len;
                }
                memcpy(&uart_tx_msg.data, mqtt_rx->data, uart_tx_msg.length); // Only copy what will fit...
                // Place our data on the UART tx queue
                xQueueSend(xqUART_tx, &uart_tx_msg, (TickType_t)0);
                continue;
            }
            /**************************************************************************************************/
            /*  DO SOMETHING WITH THE RECEIVED COMM MESSAGE                                                   */
            /**************************************************************************************************/
            if (strcmp((char *)&mqtt_rx->topic_detail.prefix, MQTT_COMM_PREFIX) == 0)
            {
                ESP_LOGI(MQTT_RX_TASK_TAG, "COM PORT TX - ASCII");
                // Send a copy of the MQTT message out on the UART
                static uart_message_t uart_tx_msg;
                memset(&uart_tx_msg, 0, sizeof(uart_message_t));

                uart_tx_msg.Message_ID = mqtt_rx->msg_id;
                uart_tx_msg.port = EX_UART_NUM;
                uart_tx_msg.IsHEX = false;          // TREAT DATA AS ASCII
                uart_tx_msg.IsASCII = true;         // It ASCII
                int len = strlen(mqtt_rx->data);
                if (len > UART_BUFFER_SIZE)
                {
                    uart_tx_msg.length = UART_BUFFER_SIZE; //(int)sizeof(uart_tx_msg.data);
                }
                else
                {
                    uart_tx_msg.length = len;
                }
                memcpy(&uart_tx_msg.data, mqtt_rx->data, uart_tx_msg.length); // Only copy what will fit...
                // Place our data on the UART tx queue
                xQueueSend(xqUART_tx, &uart_tx_msg, (TickType_t)0);
                continue;
            }
            /**************************************************************************************************/
            /*  DO SOMETHING WITH THE RECEIVED SOCKET MESSAGE                                                 */
            /**************************************************************************************************/
            if (strcmp((char *)&mqtt_rx->topic_detail.prefix, MQTT_HEX_SOCKET_PREFIX) == 0)
            {
                ESP_LOGI(MQTT_RX_TASK_TAG, "COMMAND SOCKET TX");
                // Send a copy of the MQTT message out on the SOCKET
                static uart_message_t uart_tx_msg;
                memset(&uart_tx_msg, 0, sizeof(uart_message_t));

                uart_tx_msg.Message_ID = mqtt_rx->msg_id;
                uart_tx_msg.port = EX_UART_NUM;
                uart_tx_msg.IsHEX = false;          // TREAT DATA AS ASCII
                uart_tx_msg.IsASCII = true;         // It is an ASCII string
                int len = strlen(mqtt_rx->data);
                if (len > UART_BUFFER_SIZE)
                {
                    uart_tx_msg.length = UART_BUFFER_SIZE; //(int)sizeof(uart_tx_msg.data);
                }
                else
                {
                    uart_tx_msg.length = len;
                }
                memcpy(&uart_tx_msg.data, mqtt_rx->data, uart_tx_msg.length); // Only copy what will fit...
                // Place our data on the UART tx queue

                // TBD
                continue;
            }
        }
#ifdef STACK_MONITOR
            uxHighWaterMark_RX = uxTaskGetStackHighWaterMark( NULL );
            printf("MQTT RX STACK HW (RUN): %d\r\n", uxHighWaterMark_RX);
        #endif
    }

    // Finalise task here
    free(xqMQTT_rx_Messages);
    vTaskDelete(xHandle_rx_mqtt);
}

void mqtt_tx_task(void *arg)
{
    #ifdef STACK_MONITOR
        /* Inspect our own high water mark on entering the task. */
        uxHighWaterMark_TX = uxTaskGetStackHighWaterMark( NULL );
        printf("MQTT TX STACK HW (START) = %d\r\n", uxHighWaterMark_TX);
    #endif

    // Initialise things here
    static mqtt_esp_message_t _mqtt_tx;
    static mqtt_esp_message_t *mqtt_tx = &_mqtt_tx;
    memset(mqtt_tx, 0, sizeof(_mqtt_tx));

    static uint16_t tx_counter = 0;

    // MQTT IO buffer Queue
    xqMQTT_tx_Messages = xQueueCreate(MQTT_Q_DEPTH, sizeof(_mqtt_tx));

    char ip_addr[32];
    sprintf((char *)&ip_addr, "IP Address: " IPSTR, IP2STR(&RM_IP.ip));
    //Send a running message out (hard coded) now: 
    strcpy((char *)&mqtt_tx->data, "ESP32 APPLICATION RUNNING on ");
    strcat((char *)&mqtt_tx->data, ip_addr);
    esp_mqtt_client_publish(mqtt_client, "/status/rm200x/", mqtt_tx->data, 0, 0, 0);

    while (1)
    {
        //Clear the input holder for a new message
        memset(mqtt_tx, 0, sizeof(_mqtt_tx));
        //Wait for a new message to transmit
        if (xQueueReceive(xqMQTT_tx_Messages, mqtt_tx, (TickType_t)portMAX_DELAY) == pdPASS) // Hold until data received on queue
        {
            mqtt_tx->msg_id = tx_counter++;     // add the counter on send
            //Now ready to publish
            ESP_LOGI(MQTT_TX_TASK_TAG, "Message: %s/%s - topic bytes = %d data bytes = %d", mqtt_tx->topic, mqtt_tx->data, mqtt_tx->topic_len, mqtt_tx->data_len);

            // Immediately publish the message from the queue
            mqtt_tx->msg_id = esp_mqtt_client_publish(mqtt_client, mqtt_tx->topic, mqtt_tx->data, 0, mqtt_tx->qos, 0);
            // or Enqueue the message on the "Publish" stream - BEWARE the queue has a limited size and will "pace" the outgoing messages!
            //mqtt_tx->msg_id = esp_mqtt_client_enqueue(mqtt_client, mqtt_tx->topic, mqtt_tx->data, 0, mqtt_tx->qos, 0, true);

            // Loop counter around if necessary
            if (tx_counter > (UINT16_MAX-1))
            {
                tx_counter = 0;
            }

            // Flash LED to show activity
            xSemaphoreGive(bin_s_sync_blink_task);

        }

        #ifdef STACK_MONITOR
            uxHighWaterMark_TX = uxTaskGetStackHighWaterMark( NULL );
            printf("MQTT TX STACK HW (RUN):: %d\r\n", uxHighWaterMark_TX);
        #endif
    }

    // Finalise task here
    free(xqMQTT_tx_Messages);
    vTaskDelete(xHandle_rx_mqtt);
}

/******************************************************************************************/
// END MQTT
/******************************************************************************************/