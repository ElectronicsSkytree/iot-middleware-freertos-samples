/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/* Copyright (c) Skytree B.V. All rights reserved. */

/* Local includes */
#include "main.h"
#include "errors.h"
// #include "controller_data.h"
#include "freertos_wrapper.h"
#include "time_wrapper.h"
#include "security_wrapper.h"
#include "uart_wrapper.h"
#include "log_utility.h"

// #include <stdint.h>
// #include <stdio.h>
// #include <stdarg.h>
// #include <string.h>
// #include <time.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "event_groups.h"
#include "lwip.h"
#include "lwip/apps/sntp.h"

/* Global Variables */
EventGroupHandle_t xConnectionEventGroup;

/* Prototypes. */
extern void vIoTBackendConnectionTask( void );
extern void vIoTTelemetryTask( void );
extern void vIoTCloudMessageTask( void );
extern void vControllerCommunicationTask( void );

/**
 * @brief Application hook.
 * 
 * This is where we start all our tasks to manage IoT
 * 
 * @note We start communication with controller before this
 *       hook getting triggered.
*/
void vApplicationDaemonTaskStartupHook( void );

static void prvGPIOInit( void )
{
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
}

/**
 * @brief Initializes the board.
 * 
 * Initializes all drivers and sets up memory etc.
 */
static void prvMiscInitialization( void )
{
    /* USER CODE END Boot_Mode_Sequence_0 */

    /* MPU Configuration */
    MPUConfig();

    /* Enable I-Cache */
    SCB_EnableICache();

    /* Enable D-Cache */
    SCB_EnableDCache();

    /* USER CODE BEGIN Boot_Mode_Sequence_1 */
    /* Add core if want to wait for CPU2 boots */

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock. */
    SystemClockConfig();

    prvGPIOInit();

    // @todo enable LED functionalities later
    // BSP_LED_Init( LED_GREEN );
    // BSP_LED_Init( LED1 );   // Indicates the process (main) is running
    // BSP_PB_Init( BUTTON_USER, BUTTON_MODE_EXTI );
    
    /* RNG init. */
    RNGInit();
    
    USART3_UARTInit();

    /* Heap_5 is being used because the RAM is not contiguous in memory, so the
     * heap must be initialized. */
    InitializeHeap();

    /* Task to start communicating with the controller, such as a controllino MEGA.
     * Doing before backend connection so that we can start receiving data immediately. */
    // vControllerCommunicationTask();
}
/*-----------------------------------------------------------*/

/**
 * @brief Application runtime entry point.
 */
int main( void )
{
    /* Perform any hardware initialization that does not require the RTOS to be
     * running.  */
    prvMiscInitialization();

    /* To indicate IoT main is up */
    // @todo enable led later
    // BSP_LED_On( LED1 );

    xConnectionEventGroup = xEventGroupCreate();

    /* Start the scheduler.  Initialization that requires the OS to be running,
     * including the WiFi initialization, is performed in the RTOS daemon task
     * startup hook. */
    vTaskStartScheduler();

    return 0;
}
/*-----------------------------------------------------------*/

/**
 * @brief Application hook.
 * 
 * This is where we start all our tasks to manage IoT
 * 
 * @note We start communication with controller before this
 *       hook getting triggered.
*/
void vApplicationDaemonTaskStartupHook( void )
{
    MX_LWIP_Init();
    
    RTCInit();
    
    if( InitializeSNTP() != 0 )
    {
        Error_Handler(__func__, __LINE__);
    }

    StartTimeSync();

    /* Starting IoT related tasks here as by now network is up. */
    configPRINTF( ( "---------STARTING IoT Tasks---------\r\n" ) );

    /* 1. Controller communication task */
    /* This is already started in main().*/

    /* 2. IoT backend connection task */
    vIoTBackendConnectionTask();

    /* 3. IoT subscription and direct method task */
    vIoTCloudMessageTask();

    /* 4. IoT telemetry task */
    vIoTTelemetryTask();
}
/*-----------------------------------------------------------*/

/**
 * @brief Period elapsed callback in non blocking mode
 *
 * @note This function is called  when TIM1 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 *
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback( TIM_HandleTypeDef * htim )
{
    if( htim->Instance == TIM6 )
    {
        HAL_IncTick();
    }
}
/*-----------------------------------------------------------*/
