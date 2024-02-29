/* Copyright (c) Skytree B.V. All rights reserved. */

/* Local Includes */
#include "uart_wrapper.h"

/* HAL includes */
#include "stm32l4xx_hal.h"
#include "stm32l475e_iot01.h"

/* Private variables */
static UART_HandleTypeDef xConsoleUart;

/**
 * @brief Publishes a character to the STM32L475 UART
 *
 * This is used to implement the tinyprintf created by Spare Time Labs
 * http://www.sparetimelabs.com/tinyprintf/tinyprintf.php
 *
 * @param pv    unused void pointer for compliance with tinyprintf
 * @param ch    character to be printed
 */
void vSTM32L475putc( void * pv,
                     char ch )
{
    while( HAL_OK != HAL_UART_Transmit( &xConsoleUart, ( uint8_t * ) &ch, 1, 30000 ) )
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
    vSTM32L475putc( NULL, ch );

    return 0;
}
/*-----------------------------------------------------------*/

/**
 * @brief UART console initialization
*/
void ConsoleUARTInit( void )
{
    xConsoleUart.Instance = USART1;
    xConsoleUart.Init.BaudRate = 115200;
    xConsoleUart.Init.WordLength = UART_WORDLENGTH_8B;
    xConsoleUart.Init.StopBits = UART_STOPBITS_1;
    xConsoleUart.Init.Parity = UART_PARITY_NONE;
    xConsoleUart.Init.Mode = UART_MODE_TX_RX;
    xConsoleUart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    xConsoleUart.Init.OverSampling = UART_OVERSAMPLING_16;
    xConsoleUart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    xConsoleUart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    BSP_COM_Init( COM1, &xConsoleUart );
}
/*-----------------------------------------------------------*/