/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef ERRORS_H
#define ERRORS_H

#ifdef __cplusplus
	extern "C" {
#endif

/* Local includes */
#include "log_utility.h"

/* Standard includes. */
#include <stdint.h>

/* HAL includes */
#include "stm32l475e_iot01.h"

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
    vLoggingPrintf( "[FATAL] [%s] [%s:%d] Error_Handler: Reset device!!\r\n", getCurrentTime(), func, line );
    BSP_LED_Toggle( LED_GREEN );

    // HAL_Delay( 1000 );

    // Force reset to retry
    NVIC_SystemReset();

    // Commented this as we always go for a system reset
    #if 0
    while( 1 )
    {
        BSP_LED_Toggle( LED_GREEN );
        HAL_Delay( 200 );
    }
    #endif
}
/*-----------------------------------------------------------*/

#ifdef __cplusplus
	}
#endif

#endif // ERRORS_H
