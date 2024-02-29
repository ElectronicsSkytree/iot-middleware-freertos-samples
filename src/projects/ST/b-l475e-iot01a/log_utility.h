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
#include <stdbool.h>

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
    va_list vargs;

    printf("[%s] ", getCurrentTime());

    va_start( vargs, pcFormat );
    vprintf( pcFormat, vargs );
    va_end( vargs );
}
/*-----------------------------------------------------------*/

/**
 * @brief print logs
*/
void vLoggingPrintf( const char * pcFormat, ... )
{
    va_list vargs;

    va_start( vargs, pcFormat );
    vprintf( pcFormat, vargs );
    va_end( vargs );
}
/*-----------------------------------------------------------*/

#ifdef __cplusplus
	}
#endif

#endif // LOG_UTILITY_H