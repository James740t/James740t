#include "base/rm200x_gpio.h"

//FREERTOS CONTROL ITEMS
SemaphoreHandle_t bin_s_sync_blink_task = NULL;
TaskHandle_t xHandle_blink = NULL;
const char *BLINK_TASK_TAG = "BLINK_TASK";

//Local prototypes
void blink_led_task(void *on_time);

//#define STACK_MONITOR   1
#ifdef STACK_MONITOR
UBaseType_t uxHighWaterMark_X;
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
// GPIO and HELPERS
/******************************************************************************************/
void blink_init(uint16_t p_flash_time)
{
    bin_s_sync_blink_task = xSemaphoreCreateBinary();

    //GPIO and BLINK
    static uint16_t task_parameters_flash_time;
    task_parameters_flash_time = p_flash_time;
    if (p_flash_time < MIN_FLASH_TIME) task_parameters_flash_time = MIN_FLASH_TIME;
    if (p_flash_time > MAX_FLASH_TIME) task_parameters_flash_time = MAX_FLASH_TIME;
    
    esp_log_level_set(BLINK_TASK_TAG, ESP_LOG_NONE);

    xTaskCreate(blink_led_task, BLINK_TASK_TAG, 1024*3, (void *)&task_parameters_flash_time, configMAX_PRIORITIES, &xHandle_blink);
}

void blink_led_task(void *on_time)
{
#ifdef STACK_MONITOR
    /* Inspect our own high water mark on entering the task. */
    uxHighWaterMark_X = uxTaskGetStackHighWaterMark( NULL );
    printf("BLINK STACK HW (START) = %d\r\n", uxHighWaterMark_X);
#endif
    /* Keep a local value of the  blink time */
    int time_on = (int)*((uint16_t *)on_time);

    /* Setup the GPIO pin */
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    /* LED off (output low) */
    gpio_set_level(BLINK_GPIO, GPIO_OFF);

    while(1)
    {
        xSemaphoreTake(bin_s_sync_blink_task, portMAX_DELAY);
        /* LED on (output high) */
        gpio_set_level(BLINK_GPIO, GPIO_ON);
        vTaskDelay(time_on / portTICK_PERIOD_MS);
        /* LED off (output low) */
        gpio_set_level(BLINK_GPIO, GPIO_OFF);

        #ifdef STACK_MONITOR
            /* Inspect our own high water mark on entering the task. */
            uxHighWaterMark_X = uxTaskGetStackHighWaterMark( NULL );
            printf("BLINK STACK HW (RUN) = %d\r\n", uxHighWaterMark_X);
        #endif

    }
    vTaskDelete(xHandle_blink);
}

/******************************************************************************************/
// GPIO and HELPERS -- END --
/******************************************************************************************/