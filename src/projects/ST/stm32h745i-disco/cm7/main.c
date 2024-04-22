/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/* Copyright (c) Skytree B.V. All rights reserved. */

/* Local includes */
#include "errors.h"
#include "memory_wrapper.h"
#include "security_wrapper.h"
#include "time_wrapper.h"
#include "uart_gpio_wrapper.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "lwip.h"
#include "event_groups.h"

/**
 * @brief IoT connection related event group.
*/
EventGroupHandle_t xConnectionEventGroup;

/**
 * @brief Task externs
*/
extern void vCreateIoTBackendConnectionTask( void );
extern void vCreateControllerCommunicationTask( void );
/*-----------------------------------------------------------*/

/**
 * @brief Initializes the board.
 */
static void prvMiscInitialization( void )
{
    /* MPU Configuration */
    MPUConfig();

    /* Enable I-Cache */
    SCB_EnableICache();

    /* Enable D-Cache */
    SCB_EnableDCache();

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock. */
    SystemClockConfig();

    MXGPIOInit();
    MXRNGInit();
    USART3Init();
    USART1Init();

    /* Heap_5 is being used because the RAM is not contiguous in memory, so the
     * heap must be initialized. */
    InitializeHeap();
}
/*-----------------------------------------------------------*/

/**
 * @brief Application runtime entry point.
 */
int main( void )
{
    /* Hardware initializations that does not require the RTOS to be running. */
    prvMiscInitialization();

    xConnectionEventGroup = xEventGroupCreate();

    /* Start the scheduler.  Initializations that requires the OS to be running
     * is performed in the RTOS daemon task startup hook.
     */
    vTaskStartScheduler();

    return 0;
}
/*-----------------------------------------------------------*/

void vApplicationDaemonTaskStartupHook( void )
{
    /* Initializers */
    MX_LWIP_Init();
    InitializeRTC();
    InitializeSNTP();

    StartTimeSync();

    /* Starting IoT related tasks here as by now network is up. */
    configPRINTF( ( "---------STARTING IoT Tasks---------\r\n" ) );

    /* 1. Controller communication task */
    vCreateControllerCommunicationTask();

    /* 2. IoT backend connection task */
    vCreateIoTBackendConnectionTask();
}
/*-----------------------------------------------------------*/
