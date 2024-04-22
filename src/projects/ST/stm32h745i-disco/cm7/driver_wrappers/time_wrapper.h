/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef TIME_WRAPPER_H
#define TIME_WRAPPER_H

/* Standard includes */
#include <stdint.h>

/**
 * @brief Get unix time in seconds
 * 
 * @return unix time in seconds
*/
uint64_t GetUnixTime( void );

/**
 * @brief Initializes the system clock.
 */
void SystemClockConfig( void );

/**
 * @brief Initialize NTP servers
*/
void InitializeSNTP( void );

/**
 * @brief Initialize RTC
*/
void InitializeRTC( void );

/**
 * @brief Start time sync
 * 
 * Starts a timer to make sure time is synced and sane.
*/
void StartTimeSync( void );

#endif // TIME_WRAPPER_H
