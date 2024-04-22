/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "main.h"
#include "errors.h"
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "config.h"
#include "task.h"
#include "lwip.h"
#include "lwip/apps/sntp.h"
#include "event_groups.h"

/*-----------------------------------------------------------*/

void vApplicationDaemonTaskStartupHook( void );

/* Private variables ---------------------------------------------------------*/
RNG_HandleTypeDef xHrng;
RTC_HandleTypeDef xHrtc;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart1;
static const char * pTimeServers[] = { "pool.ntp.org", "time.nist.gov" };
const size_t numTimeServers = sizeof( pTimeServers ) / sizeof( char * );

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config( void );
static void MX_GPIO_Init( void );
static void MPU_Config( void );
static void MX_RNG_Init( void );
static void MX_USART3_UART_Init( void );
static void MX_USART1_UART_Init( void );

EventGroupHandle_t xConnectionEventGroup;

/**
 * @brief Task externs
*/
extern void vCreateIoTBackendConnectionTask( void );
extern void vControllerCommunicationTask( void );

/**
 * @brief Initializes LWIP SNTP.
 */
static void prvInitializeSNTP( void );

/**
 * @brief Initialize Real Time Clock
 */
static void prvInitializeRTC( void );

/**
 * @brief Sets Real Time Clock
 *
 * @param[in] sec Unsigned integer to set to
 */
void setTimeRTC( uint32_t sec );

/**
 * @brief Gets unix time from Real Time Clock
 *
 * @param[out] pTime Pointer variable to store unix time in.
 */
static void getTimeRTC( uint32_t * pTime );

/**
 * @brief Initializes the STM32L475 IoT node board.
 *
 * Initialization of clock, LEDs, RNG, RTC, and WIFI module.
 */
static void prvMiscInitialization( void );

/**
 * @brief Initializes the FreeRTOS heap.
 *
 * Heap_5 is being used because the RAM is not contiguous, therefore the heap
 * needs to be initialized.  See http://www.freertos.org/a00111.html
 */
static void prvInitializeHeap( void );

/*-----------------------------------------------------------*/

/**
 * @brief Application runtime entry point.
 */
int main( void )
{
    /* Perform any hardware initialization that does not require the RTOS to be
     * running.  */
    prvMiscInitialization();

    xConnectionEventGroup = xEventGroupCreate();

    /* Start the scheduler.  Initialization that requires the OS to be running,
     * including the WiFi initialization, is performed in the RTOS daemon task
     * startup hook. */
    vTaskStartScheduler();

    return 0;
}
/*-----------------------------------------------------------*/


int uxRand( void )
{
    HAL_StatusTypeDef status = HAL_OK;
    uint32_t random_number = 0;

    status = HAL_RNG_GenerateRandomNumber( &xHrng, &random_number );

    if( HAL_OK != status )
    {
        return 0;
    }

    return ( int ) random_number;
}

void vApplicationDaemonTaskStartupHook( void )
{
    /* Initializers */
    MX_LWIP_Init();
    prvInitializeRTC();
    prvInitializeSNTP();

    /* Starting IoT related tasks here as by now network is up. */
    configPRINTF( ( "---------STARTING IoT Tasks---------\r\n" ) );

    /* 1. Controller communication task */
    vControllerCommunicationTask();

    /* 2. IoT backend connection task */
    vCreateIoTBackendConnectionTask();
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
 * implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
 * used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    uint32_t * pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
 * function then they must be declared static - otherwise they will be allocated on
 * the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle
     * task's state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
 * implementation of vApplicationGetTimerTaskMemory() to provide the memory that is
 * used by the RTOS daemon/time task. */
void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                     StackType_t ** ppxTimerTaskStackBuffer,
                                     uint32_t * pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
 * function then they must be declared static - otherwise they will be allocated on
 * the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle
     * task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
/*-----------------------------------------------------------*/

/**
 * @brief Publishes a character to the STM32L475 UART
 *
 * This is used to implement the tinyprintf created by Spare Time Labs
 * http://www.sparetimelabs.com/tinyprintf/tinyprintf.php
 *
 * @param pv    unused void pointer for compliance with tinyprintf
 * @param ch    character to be printed
 */
void vSTM32h745putc( void * pv,
                     char ch )
{
    while( HAL_OK != HAL_UART_Transmit( &huart3, ( uint8_t * ) &ch, 1, 30000 ) )
    {
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief Publishes a character to the STM32L475 UART
 *
 */
int __io_putchar( int ch )
{
    vSTM32h745putc( NULL, ch );

    return 0;
}

/*-----------------------------------------------------------*/

/**
 * @brief Initializes the board.
 */
static void prvMiscInitialization( void )
{
    /* USER CODE END Boot_Mode_Sequence_0 */

    /* MPU Configuration--------------------------------------------------------*/
    MPU_Config();

    /* Enable I-Cache---------------------------------------------------------*/
    SCB_EnableICache();

    /* Enable D-Cache---------------------------------------------------------*/
    SCB_EnableDCache();

    /* USER CODE BEGIN Boot_Mode_Sequence_1 */
    /* Add core if want to wait for CPU2 boots */

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock. */
    SystemClock_Config();

    MX_GPIO_Init();
    MX_RNG_Init();
    MX_USART3_UART_Init();
    MX_USART1_UART_Init();

    /* Heap_5 is being used because the RAM is not contiguous in memory, so the
     * heap must be initialized. */
    prvInitializeHeap();
}
/*-----------------------------------------------------------*/

static void MX_GPIO_Init( void )
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
 * @brief Initializes the system clock.
 */
static void SystemClock_Config( void )
{
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };

    /** Supply configuration update enable
     */
    /*HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY); */

    /** Configure the main internal regulator output voltage
     */
    /*__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1); */

    /*while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {} */

    /** Initializes the CPU, AHB and APB busses clocks
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48 | RCC_OSCILLATORTYPE_LSI
                                       | RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 2;
    RCC_OscInitStruct.PLL.PLLN = 64;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;

    if( HAL_RCC_OscConfig( &RCC_OscInitStruct ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }

    /** Initializes the CPU, AHB and APB busses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                                  | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

    if( HAL_RCC_ClockConfig( &RCC_ClkInitStruct, FLASH_LATENCY_2 ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC | RCC_PERIPHCLK_USART3
                                               | RCC_PERIPHCLK_RNG;
    PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
    PeriphClkInitStruct.RngClockSelection = RCC_RNGCLKSOURCE_HSI48;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;

    if( HAL_RCCEx_PeriphCLKConfig( &PeriphClkInitStruct ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }
}
/*-----------------------------------------------------------*/

static void MPU_Config( void )
{
    MPU_Region_InitTypeDef MPU_InitStruct = { 0 };

    /* Disables the MPU */
    HAL_MPU_Disable();

    /** Initializes and configures the Region and the memory to be protected
     */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU_InitStruct.BaseAddress = 0x30040000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_32KB;
    MPU_InitStruct.SubRegionDisable = 0x0;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

    HAL_MPU_ConfigRegion( &MPU_InitStruct );

    /** Initializes and configures the Region and the memory to be protected
     */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER1;
    MPU_InitStruct.BaseAddress = 0x30040000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_256B;
    MPU_InitStruct.SubRegionDisable = 0x0;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

    HAL_MPU_ConfigRegion( &MPU_InitStruct );
    /* Enables the MPU */
    HAL_MPU_Enable( MPU_PRIVILEGED_DEFAULT );
}
/*-----------------------------------------------------------*/

/**
 * @brief USART3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART3_UART_Init( void )
{
    /* USER CODE BEGIN USART3_Init 0 */

    /* USER CODE END USART3_Init 0 */

    /* USER CODE BEGIN USART3_Init 1 */

    /* USER CODE END USART3_Init 1 */
    huart3.Instance = USART3;
    huart3.Init.BaudRate = 115200;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if( HAL_UART_Init( &huart3 ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }

    if( HAL_UARTEx_SetTxFifoThreshold( &huart3, UART_TXFIFO_THRESHOLD_1_8 ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }

    if( HAL_UARTEx_SetRxFifoThreshold( &huart3, UART_RXFIFO_THRESHOLD_1_8 ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }

    if( HAL_UARTEx_DisableFifoMode( &huart3 ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }

    /* USER CODE BEGIN USART3_Init 2 */

    /* USER CODE END USART3_Init 2 */
}
/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init( void )
{
    /* USER CODE BEGIN USART1_Init 0 */

    /* USER CODE END USART1_Init 0 */

    /* USER CODE BEGIN USART1_Init 1 */

    /* USER CODE END USART1_Init 1 */
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if( HAL_UART_Init( &huart1 ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }

    if( HAL_UARTEx_SetTxFifoThreshold( &huart1, UART_TXFIFO_THRESHOLD_1_8 ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }

    if( HAL_UARTEx_SetRxFifoThreshold( &huart1, UART_RXFIFO_THRESHOLD_1_8 ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }

    if( HAL_UARTEx_DisableFifoMode( &huart1 ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }

    /* USER CODE BEGIN USART1_Init 2 */

    /* USER CODE END USART1_Init 2 */
}
/*-----------------------------------------------------------*/

/**
 * @brief RNG Initialization Function
 * @param None
 * @retval None
 */
static void MX_RNG_Init( void )
{
    /* USER CODE BEGIN RNG_Init 0 */

    /* USER CODE END RNG_Init 0 */

    /* USER CODE BEGIN RNG_Init 1 */

    /* USER CODE END RNG_Init 1 */
    xHrng.Instance = RNG;
    xHrng.Init.ClockErrorDetection = RNG_CED_ENABLE;

    if( HAL_RNG_Init( &xHrng ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }

    /* USER CODE BEGIN RNG_Init 2 */

    /* USER CODE END RNG_Init 2 */
}
/*-----------------------------------------------------------*/

/**
 * @brief Warn user if pvPortMalloc fails.
 *
 * Called if a call to pvPortMalloc() fails because there is insufficient
 * free memory available in the FreeRTOS heap.  pvPortMalloc() is called
 * internally by FreeRTOS API functions that create tasks, queues, software
 * timers, and semaphores.  The size of the FreeRTOS heap is set by the
 * configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h.
 *
 */
void vApplicationMallocFailedHook()
{
    taskDISABLE_INTERRUPTS();

    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Loop forever if stack overflow is detected.
 *
 * If configCHECK_FOR_STACK_OVERFLOW is set to 1,
 * this hook provides a location for applications to
 * define a response to a stack overflow.
 *
 * Use this hook to help identify that a stack overflow
 * has occurred.
 *
 */
void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    char * pcTaskName )
{
    portDISABLE_INTERRUPTS();

    /* Loop forever */
    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
}

/*-----------------------------------------------------------*/

void prvGetRegistersFromStack( uint32_t * pulFaultStackAddress )
{
/* These are volatile to try and prevent the compiler/linker optimising them
 * away as the variables never actually get used.  If the debugger won't show the
 * values of the variables, make them global my moving their declaration outside
 * of this function. */
    volatile uint32_t r0;
    volatile uint32_t r1;
    volatile uint32_t r2;
    volatile uint32_t r3;
    volatile uint32_t r12;
    volatile uint32_t lr;  /* Link register. */
    volatile uint32_t pc;  /* Program counter. */
    volatile uint32_t psr; /* Program status register. */

    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    /* Remove compiler warnings about the variables not being used. */
    ( void ) r0;
    ( void ) r1;
    ( void ) r2;
    ( void ) r3;
    ( void ) r12;
    ( void ) lr;  /* Link register. */
    ( void ) pc;  /* Program counter. */
    ( void ) psr; /* Program status register. */

    /* When the following line is hit, the variables contain the register values. */
    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

/* Psuedo random number generator.  Just used by demos so does not need to be
 * secure.  Do not use the standard C library rand() function as it can cause
 * unexpected behaviour, such as calls to malloc(). */
int MainRand32( void )
{
    static UBaseType_t uxlNextRand; /*_RB_ Not seeded. */
    const uint32_t ulMultiplier = 0x015a4e35UL, ulIncrement = 1UL;

    /* Utility function to generate a pseudo random number. */

    uxlNextRand = ( ulMultiplier * uxlNextRand ) + ulIncrement;

    return( ( int ) ( uxlNextRand >> 16UL ) & 0x7fffUL );
}
/*-----------------------------------------------------------*/

static void prvInitializeHeap( void )
{
    static uint8_t ucHeap1[ configTOTAL_HEAP_SIZE + 25 * 1024 ];

    HeapRegion_t xHeapRegions[] =
    {
        { ( unsigned char * ) ucHeap1, sizeof( ucHeap1 ) },
        { NULL,                        0                 }
    };

    vPortDefineHeapRegions( xHeapRegions );
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

int mbedtls_hardware_poll( void * data,
                           unsigned char * output,
                           size_t len,
                           size_t * olen )
{
    HAL_StatusTypeDef status = HAL_OK;
    uint32_t random_number = 0;

    status = HAL_RNG_GenerateRandomNumber( &xHrng, &random_number );
    ( ( void ) data );
    *olen = 0;

    if( ( len < sizeof( uint32_t ) ) || ( HAL_OK != status ) )
    {
        return 0;
    }

    memcpy( output, &random_number, sizeof( uint32_t ) );
    *olen = sizeof( uint32_t );

    return 0;
}
/*-----------------------------------------------------------*/

int mbedtls_platform_entropy_poll( void * data,
                                   unsigned char * output,
                                   size_t len,
                                   size_t * olen )
{
    return mbedtls_hardware_poll( data, output, len, olen );
}
/*-----------------------------------------------------------*/

static void prvInitializeSNTP( void )
{
    uint32_t unixTime = 0;

    sntp_setoperatingmode( SNTP_OPMODE_POLL );

    for( uint8_t i = 0; i < numTimeServers; i++ )
    {
        sntp_setservername( i, pTimeServers[ i ] );
    }

    sntp_init();

    do
    {
        getTimeRTC( &unixTime );

        if( unixTime < configSNTP_INIT_WAIT )
        {
            configPRINTF( ( "SNTP not queried yet. Retrying.\r\n" ) );
            vTaskDelay( configSNTP_INIT_RETRY_DELAY / portTICK_PERIOD_MS );
        }
    } while( unixTime < configSNTP_INIT_WAIT );

    configPRINTF( ( "> SNTP Initialized: %lu\r\n", unixTime ) );
}
/*-----------------------------------------------------------*/

static void prvInitializeRTC( void )
{
    xHrtc.Instance = RTC;
    xHrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    xHrtc.Init.AsynchPrediv = 127;
    xHrtc.Init.SynchPrediv = 255;
    xHrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    xHrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    xHrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    if( HAL_RTC_Init( &xHrtc ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }

    setTimeRTC( 0 ); /* Initializing at epoch. */
}
/*-----------------------------------------------------------*/

static void getTimeRTC( uint32_t * pTime )
{
    struct tm calTime;
    RTC_TimeTypeDef sTime = { 0 };
    RTC_DateTypeDef sDate = { 0 };

    HAL_RTC_GetTime( &xHrtc, &sTime, RTC_FORMAT_BIN );
    HAL_RTC_GetDate( &xHrtc, &sDate, RTC_FORMAT_BIN );

    calTime.tm_sec = sTime.Seconds;
    calTime.tm_min = sTime.Minutes;
    calTime.tm_hour = sTime.Hours;
    calTime.tm_isdst = sTime.DayLightSaving;
    calTime.tm_mday = sDate.Date;
    calTime.tm_mon = sDate.Month;
    calTime.tm_year = sDate.Year;

    *pTime = mktime( &calTime );
}
/*-----------------------------------------------------------*/

void setTimeRTC( uint32_t sec )
{
    struct tm calTime;
    time_t unixTime = sec;
    RTC_TimeTypeDef sTime = { 0 };
    RTC_DateTypeDef sDate = { 0 };

    gmtime_r( &unixTime, &calTime );

    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    sTime.TimeFormat = RTC_HOURFORMAT_24;
    sTime.Seconds = calTime.tm_sec;
    sTime.SecondFraction = 0;
    sTime.SubSeconds = 0;
    sTime.Minutes = calTime.tm_min;
    sTime.Hours = calTime.tm_hour;
    sDate.WeekDay = calTime.tm_wday;
    sDate.Date = calTime.tm_mday;
    sDate.Month = calTime.tm_mon;
    sDate.Year = calTime.tm_year;

    HAL_RTC_SetTime( &xHrtc, &sTime, RTC_FORMAT_BIN );
    HAL_RTC_SetDate( &xHrtc, &sDate, RTC_FORMAT_BIN );
}
/*-----------------------------------------------------------*/

uint64_t GetUnixTime( void )
{
    uint32_t unixTime;

    getTimeRTC( &unixTime );
    return ( uint64_t ) unixTime;
}
/*-----------------------------------------------------------*/