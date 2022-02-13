#include "base/rm200x_wifi.h"

/******************************************************************************************/
// STARTUP WIFI
/******************************************************************************************/

//FREERTOS CONTROL ITEMS
SemaphoreHandle_t bin_s_sync_wifi_connected = NULL;
//Task Tags
const char *WIFI_TAG = "WIFI_TASK";

//Variables
volatile esp_netif_ip_info_t RM_IP;

static EventGroupHandle_t s_wifi_event_group;
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
static int s_retry_num = 0;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        if (s_retry_num < RETRY_LIMIT) 
        {
            esp_wifi_connect();
            s_retry_num++;
            printf("Retry connection to the AP\r\n");
        } 
        else 
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        printf("Connect to the AP failed\r\n");
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_TAG, "IP Address: \t" IPSTR, IP2STR(&event->ip_info.ip));
	    ESP_LOGI(WIFI_TAG, "Subnet mask: \t" IPSTR, IP2STR(&event->ip_info.netmask));
	    ESP_LOGI(WIFI_TAG, "Gateway: \t" IPSTR, IP2STR(&event->ip_info.gw));

        //Copy individual elements to keep the volitile attribute
        RM_IP.ip.addr = event->ip_info.ip.addr;
        RM_IP.netmask.addr = event->ip_info.netmask.addr;
        RM_IP.gw.addr = event->ip_info.gw.addr;

        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_station_mode(void)
{
    // Initialise the semaphore
	bin_s_sync_wifi_connected = xSemaphoreCreateBinary();

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    printf("START WIFI CONNECTION : Station Mode\r\n");

    s_wifi_event_group = xEventGroupCreate();

    // initialize the tcp stack
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. In case your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(WIFI_TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) 
    {
        printf("CONNECTED to %s\r\n", WIFI_SSID);
        ESP_LOGI(WIFI_TAG, "connected to ap SSID:%s password:%s", WIFI_SSID, WIFI_PASS);
        //Connection details
        printf("IP Address: \t" IPSTR "\r\n", IP2STR(&RM_IP.ip));
	    printf("Subnet mask: \t" IPSTR "\r\n", IP2STR(&RM_IP.netmask));
	    printf("Gateway: \t" IPSTR "\r\n", IP2STR(&RM_IP.gw));
        // Wait to fire the MQTT start semaphore
        vTaskDelay(1000 / portTICK_RATE_MS);
	    // Release the semaphore to allow MQTT task to start
	    xSemaphoreGive(bin_s_sync_wifi_connected);
    } 
    else if (bits & WIFI_FAIL_BIT) 
    {
        printf("CONNECTION to %s FAILED\r\n", WIFI_SSID);
        ESP_LOGI(WIFI_TAG, "Failed to connect to SSID:%s, password:%s", WIFI_SSID, WIFI_PASS);
    } 
    else 
    {
        printf("CONNECTION to %s UNEXPECTED EVENT\r\n", WIFI_SSID);
        ESP_LOGE(WIFI_TAG, "UNEXPECTED EVENT");
    }

    // Run to end and clean up - all is running now (or failed :))
    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

/******************************************************************************************/
// STARTUP WIFI -- END --
/******************************************************************************************/