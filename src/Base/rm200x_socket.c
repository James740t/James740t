#include "base/rm200x_socket.h"

//FREERTOS CONTROL ITEMS
TaskHandle_t xHandle_tcpip_tx = NULL;
TaskHandle_t xHandle_tcpip_rx = NULL;

QueueHandle_t xqTCPIP_rx = NULL;
QueueHandle_t xqTCPIP_tx = NULL;

const char *TCP_SOCK_SERVER_TAG = "TCP SOCKET SERVER";
const char *TCP_SOCK_CLIENT_TAG = "TCP SOCKET CLIENT";
const char *payload = "Message from ESP32 ";

int SOCKET = -1;


/******************************************************************************************/
// HELPERS - UART LINK 
/******************************************************************************************/

/**
 * @brief Indicates that the file descriptor represents an invalid (uninitialized or closed) socket
 *
 * Used in the TCP server structure `sock[]` which holds list of active clients we serve.
 */
#define INVALID_SOCK (-1)

/**
 * @brief Time in ms to yield to all tasks when a non-blocking socket would block
 *
 * Non-blocking socket operations are typically executed in a separate task validating
 * the socket status. Whenever the socket returns `EAGAIN` (idle status, i.e. would block)
 * we have to yield to all tasks to prevent lower priority tasks from starving.
 */
//#define YIELD_TO_ALL_MS 50

/**
 * @brief Utility to log socket errors
 *
 * @param[in] tag Logging tag
 * @param[in] sock Socket number
 * @param[in] err Socket errno
 * @param[in] message Message to print
 */
static void log_socket_error(const char *tag, const int sock, const int err, const char *message)
{
    ESP_LOGE(tag, "[sock=%d]: %s\n"
                  "error=%d: %s",
             sock, message, err, strerror(err));
}

/**
 * @brief Tries to receive data from specified sockets in a non-blocking way,
 *        i.e. returns immediately if no data.
 *
 * @param[in] tag Logging tag
 * @param[in] sock Socket for reception
 * @param[out] data Data pointer to write the received data
 * @param[in] max_len Maximum size of the allocated space for receiving data
 * @return
 *          >0 : Size of received data
 *          =0 : No data available
 *          -1 : Error occurred during socket read operation
 *          -2 : Socket is not connected, to distinguish between an actual socket error and active disconnection
 */
static int try_receive(const char *tag, const int sock, char *data, size_t max_len)
{
    char uart_data[256];
    memset (&uart_data, 0x00, 256);

    int len = recv(sock, data, max_len, 0);
    if (len < 0)
    {
        if (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return 0; // Not an error
        }
        if (errno == ENOTCONN)
        {
            ESP_LOGW(tag, "[sock=%d]: Connection closed", sock);
            return -2; // Socket has been disconnected
        }
        log_socket_error(tag, sock, errno, "Error occurred during receiving");
        return -1;
    }

    return len;
}

/**
 * @brief Sends the specified data to the socket. This function blocks until all bytes got sent.
 *
 * @param[in] tag Logging tag
 * @param[in] sock Socket to write data
 * @param[in] data Data to be written
 * @param[in] len Length of the data
 * @return
 *          >0 : Size the written data
 *          -1 : Error occurred during socket write operation
 */
int socket_send(const char *tag, const int sock, const char * data, const size_t len)
{
    int to_write = len;
    while (to_write > 0) {
        int written = send(sock, data + (len - to_write), to_write, 0);
        if (written < 0 && errno != EINPROGRESS && errno != EAGAIN && errno != EWOULDBLOCK) {
            log_socket_error(tag, sock, errno, "Error occurred during sending");
            return -1;
        }
        to_write -= written;
    }
    return len;
}

int tcpip_send(tcpip_message_t *tcpip_out)
{
    if (SOCKET >= 0)
    {
        int to_write = tcpip_out->length;
        int written = 0;
        while (to_write > 0)
        {
            written = send(SOCKET, (char *)&tcpip_out->data + (tcpip_out->length - to_write), to_write, 0);
            // Flash LED to show activity
            xSemaphoreGive(bin_s_sync_blink_task);

            if (written < 0 && errno != EINPROGRESS && errno != EAGAIN && errno != EWOULDBLOCK)
            {
                log_socket_error(TCP_SOCK_SERVER_TAG, SOCKET, errno, "Error occurred during sending");
                return -1;
            }
            to_write -= written;
        }

        ESP_LOGI(TCP_SOCK_SERVER_TAG, "Message: %s\r\nWritten %d of %d\r\n", tcpip_out->data, written, tcpip_out->length);

char out_string[1024];
memset (&out_string, 0, sizeof(out_string));
BytesToHexString_hyp_delim((char *)&out_string, (uint8_t *)tcpip_out->data, tcpip_out->length);
printf("Received from UART %d bytes: %s\r\n", (strlen(out_string) / 3), out_string);
    }

    return tcpip_out->length;
}

void send_to_uart(const char *pt_message, uint16_t p_length)
{
    static int counter;

    // Send a copy of the MQTT message out on the UART
    static uart_message_t uart_tx_msg;
    memset(&uart_tx_msg, 0, sizeof(uart_message_t));

    uart_tx_msg.Message_ID = counter++;
    uart_tx_msg.port = EX_UART_NUM;
    uart_tx_msg.IsHEX = true; // TREAT DATA AS HEX
    if (p_length > UART_BUFFER_SIZE)
    {
        uart_tx_msg.length = UART_BUFFER_SIZE; //(int)sizeof(uart_tx_msg.data);
    }
    else
    {
        uart_tx_msg.length = p_length;
    }
    memcpy(&uart_tx_msg.data, pt_message, uart_tx_msg.length); // Only copy what will fit...
    // Place our data on the UART tx queue
    xQueueSend(xqUART_tx, &uart_tx_msg, (TickType_t) 0);

    if(counter >= UINT8_MAX)
    {
        counter = 0;
    }
}
void send_to_TCPIP(int p_socket, const char *pt_message, uint16_t p_length)
{
    int to_write = p_length;
    while (to_write > 0)
    {
        int written = send(p_socket, pt_message + (p_length - to_write), to_write, 0);
        if (written < 0)
        {
            ESP_LOGE(TCP_SOCK_SERVER_TAG, "Error occurred during sending: errno %d", errno);
        }
        to_write -= written;
    }
}
int receive_from_tcpip(int p_socket, tcpip_message_t *pt_message)
{
    int len;
    // Clear the message
    memset(pt_message, 0, sizeof(tcpip_message_t));

    if (p_socket >= 0)
    {
        len = recv(p_socket, (char *)&pt_message->data, sizeof(pt_message->data) - 1, 0);

        if (len < 0)
        {
            ESP_LOGE(TCP_SOCK_SERVER_TAG, "Error occurred during receiving: errno %d", errno);
            pt_message->length = len;
            return len;
        }
        else if (len == 0)
        {
            ESP_LOGW(TCP_SOCK_SERVER_TAG, "Connection closed");
            pt_message->length = len;
            return len;
        }
        else
        {
            // complete message details and return
            pt_message->ID = 0;
            pt_message->length = len;

            ESP_LOGI(TCP_SOCK_SERVER_TAG, "Received %d bytes: %s", pt_message->length, pt_message->data);

            // Diagnostic ACK
            //send_to_TCPIP(p_socket, "ACK'ed\r\n", 8);

            // Flash LED to show activity
            xSemaphoreGive(bin_s_sync_blink_task);

            send_to_uart((char *)&pt_message->data, pt_message->length);

            return len;
        }
    }
    else
    {
        ESP_LOGW(TCP_SOCK_SERVER_TAG, "Socket closed");
    }
    return -1;
}

    /******************************************************************************************/
    // TCP SOCKET SERVER - USE SERVER_PORT 3333
    /******************************************************************************************/

    static void socket_receiver(const int sock)
    {
        int len;
        char rx_buffer[SOCKET_BUFFER_SIZE];
        memset(&rx_buffer, 0x00, SOCKET_BUFFER_SIZE);

        do
        {
            len = try_receive(TCP_SOCK_SERVER_TAG, sock, (char *)&rx_buffer, SOCKET_BUFFER_SIZE);
            if (len > 0)
            {
                send_to_uart(rx_buffer, len);
                ESP_LOGI(TCP_SOCK_SERVER_TAG, "Received %d bytes: %s", len, rx_buffer);
            }

        } while (len > 0);
    }

    static void tcp_server_task(void *pvParameters)
    {
        // Initialise things here

        char addr_str[128];
        int addr_family = (int)pvParameters;
        int ip_protocol = 0;
        int keepAlive = 1;
        int keepIdle = KEEPALIVE_IDLE;
        int keepInterval = KEEPALIVE_INTERVAL;
        int keepCount = KEEPALIVE_COUNT;
        struct sockaddr_storage dest_addr;

        if (addr_family == AF_INET)
        {
            struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
            dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
            dest_addr_ip4->sin_family = AF_INET;
            dest_addr_ip4->sin_port = htons(LOCAL_SERVER_PORT);
            ip_protocol = IPPROTO_IP;
        }
#ifdef CONFIG_EXAMPLE_IPV6
    else if (addr_family == AF_INET6) {
        struct sockaddr_in6 *dest_addr_ip6 = (struct sockaddr_in6 *)&dest_addr;
        bzero(&dest_addr_ip6->sin6_addr.un, sizeof(dest_addr_ip6->sin6_addr.un));
        dest_addr_ip6->sin6_family = AF_INET6;
        dest_addr_ip6->sin6_port = htons(LOCAL_SERVER_PORT);
        ip_protocol = IPPROTO_IPV6;
    }
#endif

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TCP_SOCK_SERVER_TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#if defined(CONFIG_EXAMPLE_IPV4) && defined(CONFIG_EXAMPLE_IPV6)
    // Note that by default IPV6 binds to both protocols, it is must be disabled
    // if both protocols used at the same time (used in CI)
    setsockopt(listen_sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
#endif

    ESP_LOGI(TCP_SOCK_SERVER_TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TCP_SOCK_SERVER_TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TCP_SOCK_SERVER_TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(TCP_SOCK_SERVER_TAG, "Socket bound, port %d", LOCAL_SERVER_PORT);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TCP_SOCK_SERVER_TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1)
    {
        ESP_LOGI(TCP_SOCK_SERVER_TAG, "Socket listening");

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        SOCKET = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (SOCKET < 0)
        {
            ESP_LOGE(TCP_SOCK_SERVER_TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Set tcp keepalive option
        setsockopt(SOCKET, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(SOCKET, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(SOCKET, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(SOCKET, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
        // Convert ip address to string
        if (source_addr.ss_family == PF_INET)
        {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
#ifdef CONFIG_EXAMPLE_IPV6
        else if (source_addr.ss_family == PF_INET6)
        {
            inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
        }
#endif
        ESP_LOGI(TCP_SOCK_SERVER_TAG, "Socket accepted ip address: %s", addr_str);

        socket_receiver(SOCKET);

        shutdown(SOCKET, 0);
        close(SOCKET);
    }

CLEAN_UP:

    

    close(listen_sock);
    vTaskDelete(NULL);
}

void socket_server_init(void)
{
#ifdef CONFIG_IPV4
    xTaskCreate(tcp_server_task, "tcp_server", 1024*4, (void*)AF_INET, 5, NULL);
#endif
#ifdef CONFIG_IPV6
    xTaskCreate(tcp_server_task, "tcp_server", 1024*4, (void*)AF_INET6, 5, NULL);
#endif
}

/******************************************************************************************/
// TCP SOCKET CLIENT - USE HOST_SERVER_PORT 3333 
/******************************************************************************************/

static void tcp_client_task(void *pvParameters)
{
    char rx_buffer[128];
    char host_ip[] = HOST_IP_ADDR;
    int addr_family = 0;
    int ip_protocol = 0;

    while (1)
    {
#if defined CONFIG_IPV4
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(host_ip);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(HOST_SERVER_PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_pton(AF_INET, host_ip, &dest_addr.sin_addr);
#elif defined CONFIG_IPV6
        struct sockaddr_in6 dest_addr = {0};
        inet6_aton(host_ip, &dest_addr.sin6_addr);
        dest_addr.sin6_family = AF_INET6;
        dest_addr.sin6_port = htons(HOST_SERVER_PORT);
        dest_addr.sin6_scope_id = esp_netif_get_netif_impl_index(EXAMPLE_INTERFACE);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
#endif
        int sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0)
        {
            ESP_LOGE(TCP_SOCK_CLIENT_TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TCP_SOCK_CLIENT_TAG, "Socket created, connecting to %s:%d", host_ip, HOST_SERVER_PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
        if (err != 0)
        {
            ESP_LOGE(TCP_SOCK_CLIENT_TAG, "Socket unable to connect: errno %d", errno);
            break;
        }
        ESP_LOGI(TCP_SOCK_CLIENT_TAG, "Successfully connected");

        while (1)
        {
            int err = send(sock, payload, strlen(payload), 0);

            // Flash LED to show activity
            xSemaphoreGive(bin_s_sync_blink_task);

            if (err < 0)
            {
                ESP_LOGE(TCP_SOCK_CLIENT_TAG, "Error occurred during sending: errno %d", errno);
                break;
            }

            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occurred during receiving
            if (len < 0)
            {
                ESP_LOGE(TCP_SOCK_CLIENT_TAG, "recv failed: errno %d", errno);
                break;
            }
            // Data received
            else
            {
                ESP_LOGI(TCP_SOCK_CLIENT_TAG, "Received %d bytes from %s:", len, host_ip);
                ESP_LOGI(TCP_SOCK_CLIENT_TAG, "%s", rx_buffer);

                // Flash LED to show activity
                xSemaphoreGive(bin_s_sync_blink_task);
            }

            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }

        if (sock != -1)
        {
            ESP_LOGE(TCP_SOCK_CLIENT_TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

void socket_client_init(void)
{
    xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);
}

/******************************************************************************************/
// TCP SOCKET SERVER -- END --
/******************************************************************************************/