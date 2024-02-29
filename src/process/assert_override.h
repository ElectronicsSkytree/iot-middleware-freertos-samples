/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef ASSERT_OVERRIDE_H
#define ASSERT_OVERRIDE_H

#ifdef __cplusplus
    extern "C" {
#endif

/* Local includes */
#include "errors.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"

/**
 * Overriding the asserts to let IoT connectivity continue.
 * @todo Before restarting unsubscribing and TLS disconnect might not
 *       need to be done because the assert might be because of
 *       several software reasons (just not TLS/ socket disconnection).
 * @todo We may not need this in PCBA but it might be good to check
 *       if the TLS issue happens with ethernet too.
 * @todo We have system reset every failure, improve this by avoiding
 *       in unnecessary places.
*/
#undef configASSERT
#define configASSERT( x, y )                                        \
    if( ( x ) == 0 )                                                \
    {                                                               \
        taskDISABLE_INTERRUPTS();                                   \
        vLoggingPrintf( "[FATAL] Error code: %d, Reset device,"     \
            "hack to continue [%s:%d] \r\n",                        \
            (int32_t)y, __func__, __LINE__ );                       \
        vTaskDelay( pdMS_TO_TICKS( 2000U ) );                       \
        Error_Handler(__func__, __LINE__);                          \
    }

#ifdef __cplusplus
    }
#endif

#endif // ASSERT_OVERRIDE_H