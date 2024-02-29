/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef WIFI_WRAPPER_H
#define WIFI_WRAPPER_H

#ifdef __cplusplus
    extern "C" {
#endif

/* Local includes */
#include "errors.h"

/* Standard includes */
#include <stdbool.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"

/* Defined in es_wifi_io.c. */
extern void SPI_WIFI_ISR( void );

/**
 * @brief Check if WiFi driver received time
 * 
 * @param unixTime pointer to utc timestamp to be filled
*/
error_t GetWiFiTime( uint32_t* unixTime );

/**
 * @brief Initialize wifi module and connect to WiFi
*/
BaseType_t InitializeWifi( void );

#ifdef __cplusplus
    }
#endif

#endif // WIFI_WRAPPER_H
