/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/* Copyright (c) Skytree B.V. All rights reserved. */

/* Local includes */
#include "main.h"
#include "errors.h"
#include "controller_data.h"
#include "wifi_wrapper.h"
#include "freertos_wrapper.h"
#include "time_wrapper.h"
#include "security_wrapper.h"
#include "uart_wrapper.h"
#include "log_utility.h"

/* HAL includes */
#include "stm32l475e_iot01.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "semphr.h"
#include "event_groups.h"

/* Global Variables */
StaticSemaphore_t xSemaphoreBuffer;
xSemaphoreHandle xWifiSemaphoreHandle;
EventGroupHandle_t xConnectionEventGroup;

/* Prototypes. */
extern void vControllerCommunicationTask( void );
// extern void vIoTBackendConnectionTask( void );
// extern void vIoTTelemetryTask( void );
// extern void vIoTCloudMessageTask( void );

/**
 * @brief Application hook.
 * 
 * This is where we start all our tasks to manage IoT
 * 
 * @note We start communication with controller before this
 *       hook getting triggered.
*/
void vApplicationDaemonTaskStartupHook( void );

/**
 * @brief Initializes the board.
 * 
 * Initializes all drivers and sets up memory etc.
 */
static void prvMiscInitialization( void )
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock. */
    SystemClock_Config();

    /* freertos_heap2 is being used because the RAM is not contiguous in memory, so the
     * heap must be initialized. */
    InitializeHeap();

    BSP_LED_Init( LED_GREEN );
    BSP_LED_Init( LED1 );   // Indicates the process (main) is running
    BSP_PB_Init( BUTTON_USER, BUTTON_MODE_EXTI );

    /* RNG init. */
    RNGInit();

    /* RTC init. */
    RTCInit();

    /* UART console init. */
    ConsoleUARTInit();

    /* Task to start communicating with the controller, such as a controllino
     * MEGA. Doing before WiFi so that we can start receiving data immediately. */
    vControllerCommunicationTask();

    // if( InitializeWifi() != 0 )
    // {
    //     Error_Handler(__func__, __LINE__);
    // }

    // if( InitializeSNTP() != 0 )
    // {
    //     Error_Handler(__func__, __LINE__);
    // }
}
/*-----------------------------------------------------------*/

/**
 * @brief Application runtime entry point.
 */
int main( void )
{
    /* Perform any hardware initialization that does not
     * require the RTOS to be running. */
    prvMiscInitialization();

    /* To indicate IoT main is up */
    BSP_LED_On( LED1 );

    xConnectionEventGroup = xEventGroupCreate();
    
    /* Start the scheduler. Initialization that requires the OS to be running,
     * including the WiFi initialization, is performed in the RTOS daemon task
     * startup hook. */
    vTaskStartScheduler();

    return 0;
}
/*-----------------------------------------------------------*/

void vApplicationDaemonTaskStartupHook( void )
{
    /* Initialize wifi semaphore */
    xWifiSemaphoreHandle = xSemaphoreCreateMutexStatic( &( xSemaphoreBuffer ) );
    xSemaphoreGive( xWifiSemaphoreHandle );

    StartTimeSync();

    /* Starting IoT related tasks here as by now network is up. */
    configPRINTF( ( "---------STARTING IoT Tasks---------\r\n" ) );

    /* 1. Controller communication task */
    /* This is already started in main().*/

    /* 2. IoT backend connection task */
    // vIoTBackendConnectionTask();

    /* 3. IoT subscription and direct method task */
    // vIoTCloudMessageTask();

    /* 4. IoT telemetry task */
    // vIoTTelemetryTask();
}
/*-----------------------------------------------------------*/

/**
 * @brief  EXTI line detection callback.
 *
 * @param  GPIO_Pin: Specifies the port pin connected to corresponding EXTI line.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    switch( GPIO_Pin )
    {
        /* Pin number 1 is connected to Inventek Module Cmd-Data
         * ready pin. */
        case ( GPIO_PIN_1 ):
            SPI_WIFI_ISR();
            break;

        case (USER_BUTTON_PIN):
            BSP_LED_Toggle(LED_GREEN);
            // @todo was added by Kompa
            // gui_comm_send_data(data, 6, NULL, NULL, NULL);
            break;

        default:
            break;
    }
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
