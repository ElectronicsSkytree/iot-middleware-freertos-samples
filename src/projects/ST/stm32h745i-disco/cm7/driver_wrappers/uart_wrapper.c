/* Copyright (c) Skytree B.V. All rights reserved. */

/* Local Includes */
#include "uart_wrapper.h"

/* HAL includes */
#include "stm32l4xx_hal.h"
#include "stm32l475e_iot01.h"

/* Private variables */
static UART_HandleTypeDef huart3;

/**
 * @brief Publishes a character to the STM32L475 UART
 *
 * This is used to implement the tinyprintf created by Spare Time Labs
 * http://www.sparetimelabs.com/tinyprintf/tinyprintf.php
 *
 * @param pv    unused void pointer for compliance with tinyprintf
 * @param ch    character to be printed
 */
void vSTM32h745putc( void * pv, char ch )
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
 * @brief USART3 Initialization Function
 * @param None
 * @retval None
 */
void USART3_UARTInit( void )
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
        Error_Handler();
    }

    if( HAL_UARTEx_SetTxFifoThreshold( &huart3, UART_TXFIFO_THRESHOLD_1_8 ) != HAL_OK )
    {
        Error_Handler();
    }

    if( HAL_UARTEx_SetRxFifoThreshold( &huart3, UART_RXFIFO_THRESHOLD_1_8 ) != HAL_OK )
    {
        Error_Handler();
    }

    if( HAL_UARTEx_DisableFifoMode( &huart3 ) != HAL_OK )
    {
        Error_Handler();
    }

    /* USER CODE BEGIN USART3_Init 2 */

    /* USER CODE END USART3_Init 2 */
}
/*-----------------------------------------------------------*/
