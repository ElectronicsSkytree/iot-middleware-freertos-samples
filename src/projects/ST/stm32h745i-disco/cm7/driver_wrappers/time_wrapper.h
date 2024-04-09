/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef TIME_WRAPPER_H
#define TIME_WRAPPER_H

/* FreeRTOS includes. */
#include "FreeRTOS.h"

#ifdef __cplusplus
    extern "C" {
#endif

#define TWO_HOURS pdMS_TO_TICKS(7200000)

/**
 * @brief Initializes the system clock.
 */
void SystemClockConfig( void );

/**
 * @brief RTC init function.
 */
void RTCInit( void );

/**
 * @brief Set RTC with wifi utc
 * 
 * Retrieve utc (gmt) time from WiFi driver and update
 * local RTC with the time and date.
 * 
 * @todo Share the time with Controller
*/
BaseType_t InitializeSNTP( void );

/**
 * @brief Get current utc timestamp in a string.
 *
 * @param timestamp_utc timestamp in zulu format.
 * 
 * @note Caller needs to take care of memory.
 */
const char* GetUtcTimeInString( void );

/**
 * @brief Get current utc time
*/
uint64_t GetUnixTime( void );

/**
 * @brief Start time sync
 * 
 * Starts a timer to make sure time is synced and sane.
*/
void StartTimeSync();

#ifdef __cplusplus
    }
#endif

#endif // TIME_WRAPPER_H