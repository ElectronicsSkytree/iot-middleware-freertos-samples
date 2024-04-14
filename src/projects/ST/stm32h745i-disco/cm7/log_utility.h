/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef LOG_UTILITY_H
#define LOG_UTILITY_H

#ifdef __cplusplus
	extern "C" {
#endif

/* Local Includes */
#include "time_wrapper.h"

/* Standard Includes */
#include <stdarg.h>
#include <stdio.h>

/* HAL includes */
#include "stm32h7xx_hal.h"

static char cPrintString[ 512 ];
extern UART_HandleTypeDef huart3;

/**
 * @todo To avoid link error, have done this. Fix this later.
*/
const char* getCurrentTime()
{
    return GetUtcTimeInString();
}

/**
 * @brief print logs with timestamp
*/
void vLoggingPrintfWithTimestamp( const char * pcFormat, ... )
{
    size_t xLength = 0;

    va_list vargs;

    printf("[%s] ", getCurrentTime());

    va_start( vargs, pcFormat );

    xLength = vsnprintf( cPrintString, sizeof( cPrintString ), pcFormat, vargs );

    if( xLength > 0 )
    {
        xLength = xLength > sizeof( cPrintString ) ? sizeof( cPrintString ) : xLength;

        while( HAL_OK != HAL_UART_Transmit( &huart3, ( uint8_t * ) cPrintString, xLength, 30000 ) )
        {
        }
    }

    va_end( vargs );
}
/*-----------------------------------------------------------*/

/**
 * @brief print logs
*/
void vLoggingPrintf( const char * pcFormat, ... )
{
    size_t xLength = 0;

    va_list vargs;

    va_start( vargs, pcFormat );

    xLength = vsnprintf( cPrintString, sizeof( cPrintString ), pcFormat, vargs );

    if( xLength > 0 )
    {
        xLength = xLength > sizeof( cPrintString ) ? sizeof( cPrintString ) : xLength;

        while( HAL_OK != HAL_UART_Transmit( &huart3, ( uint8_t * ) cPrintString, xLength, 30000 ) )
        {
        }
    }

    va_end( vargs );
}
/*-----------------------------------------------------------*/

#ifdef __cplusplus
	}
#endif

#endif // LOG_UTILITY_H
