#include "base/rm200x_gpio.h"

//FREERTOS CONTROL ITEMS
SemaphoreHandle_t bin_s_sync_blink_task = NULL;
TaskHandle_t xHandle_blink = NULL;

static TimerHandle_t one_shot_timer = NULL;

const char *BLINK_TASK_TAG = "BLINK_TASK";

//Local prototypes
void blink_led_task(void *on_time);
void blink_led_off_callback(TimerHandle_t xTimer);

#define STACK_MONITOR   false
#if STACK_MONITOR
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
    //GPIO and BLINK
    static uint16_t task_parameters_flash_time;
    task_parameters_flash_time = p_flash_time;
    if (p_flash_time < MIN_FLASH_TIME) task_parameters_flash_time = MIN_FLASH_TIME;
    if (p_flash_time > MAX_FLASH_TIME) task_parameters_flash_time = MAX_FLASH_TIME;

    TickType_t flash_delay = task_parameters_flash_time / portTICK_PERIOD_MS;
    // Create a one-shot timer
    one_shot_timer = xTimerCreate(
                      "One-shot timer",         // Name of timer
                      flash_delay,              // Period of timer (in ticks)
                      pdFALSE,                  // Auto-reload
                      (void *)0,                // Timer ID
                      blink_led_off_callback);  // Callback function

    bin_s_sync_blink_task = xSemaphoreCreateBinary();

    esp_log_level_set(BLINK_TASK_TAG, ESP_LOG_NONE);

    xTaskCreate(blink_led_task, BLINK_TASK_TAG, 1024*2, NULL, configMAX_PRIORITIES, &xHandle_blink);
}

void blink_led_off_callback(TimerHandle_t xTimer)
{
    /* LED off (output low) */
    gpio_set_level(BLINK_GPIO, GPIO_OFF);
}

void blink_led_task(void *arg)
{
#if STACK_MONITOR
    /* Inspect our own high water mark on entering the task. */
    uxHighWaterMark_X = uxTaskGetStackHighWaterMark( NULL );
    printf("BLINK STACK HW (START) = %d\r\n", uxHighWaterMark_X);
#endif

    /* Setup the GPIO pin */
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    /* LED off (output low) */
    gpio_set_level(BLINK_GPIO, GPIO_OFF);

    while(1)
    {
        // Wait for the external trigger
        xSemaphoreTake(bin_s_sync_blink_task, portMAX_DELAY);
        /* LED on (output high) */
        gpio_set_level(BLINK_GPIO, GPIO_ON);

        // Start timer (if timer is already running, this will act as
        // xTimerReset() instead)
        xTimerStart(one_shot_timer, portMAX_DELAY);

#if STACK_MONITOR
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