#include "xxx-templates/xxx_template.h"

/******************************************************************************************/
// XXX_TEMPLATE
/******************************************************************************************/

//FREERTOS CONTROL ITEMS
TaskHandle_t xHandle_task = NULL;
const char *TASK_TASK_TAG = "TASK_TASK";

//LOCAL PROTOTYPES
void task_template (void *arg);

//STACK MONITOR - USE FOR DEBUG AND TUNING
//#define STACK_MONITOR   1
#ifdef STACK_MONITOR
UBaseType_t uxHighWaterMark_template;
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

/******************************************************************************************/
// MAIN TASKS
/******************************************************************************************/

void initiate_template(void)
{
    //esp_log_level_set(TASK_TASK_TAG, ESP_LOG_NONE);

    //xTaskCreate(task_template, TASK_TASK_TAG, 1024*3, NULL, configMAX_PRIORITIES-10, &xHandle_task);
}

void task_template (void *arg)
{
    #ifdef STACK_MONITOR
        /* Inspect our own high water mark on entering the task. */
        uxHighWaterMark_template = uxTaskGetStackHighWaterMark( NULL );
        printf("TEMPLATE STACK HW (START) = %d\r\n", uxHighWaterMark_template);
    #endif



    /*  SETUP TASK STUFF HERE */



    while(1)
    {
        /* WAIT FOR SOMETHING HERE */


        /* DO TASK LOOP STUFF HERE */




        #ifdef STACK_MONITOR
            /* Inspect our own high water mark on entering the task. */
            uxHighWaterMark_template = uxTaskGetStackHighWaterMark( NULL );
            printf("TEMPLATE STACK HW (RUN) = %d\r\n", uxHighWaterMark_template);
        #endif
    }
    //Clean up and recover any memory
    //vTaskDelete(xHandle_task);
}

/******************************************************************************************/
// XXX_TEMPLATE -- END --
/******************************************************************************************/