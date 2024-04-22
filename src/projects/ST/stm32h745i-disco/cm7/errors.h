/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef ERRORS_H
#define ERRORS_H

/* Local includes */
#include "log_utility.h"

/* Standard includes. */
#include <stdint.h>

/* HAL includes */
// #include "stm32l475e_iot01.h"
#include "stm32h7xx_hal.h"

/**
 * @brief generic success fail options
*/
enum errors {
	FAILED = 0,
	SUCCEEDED
};

typedef uint8_t error_t;

/**
 * @brief An error handler that can be called on occurence of any error.
 *
 * @note Right now to mitigate the issues we face,
 *       we are restarting the MCU.
 */
void Error_Handler( const char* func, int32_t line )
{
    // BSP_LED_Toggle( LED_GREEN );

    HAL_Delay( 2000 );

    /* To indicate IoT main is going down */
    // BSP_LED_Off( LED1 );

    // Force reset to retry
    // vLoggingPrintf( "[FATAL] [%s] [%s:%d] Reset device!!\r\n", getCurrentTime(), func, line );
    vLoggingPrintf( "[FATAL] [%s:%d] Reset device!!\r\n", func, line );
    NVIC_SystemReset();

    // Commented this as we always go for a system reset
    #if 0
    while( 1 )
    {
        // BSP_LED_Toggle( LED_GREEN );
        HAL_Delay( 200 );
    }
    #endif
}
/*-----------------------------------------------------------*/

#endif // ERRORS_H
