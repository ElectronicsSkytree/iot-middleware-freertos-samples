/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#ifndef AZURE_IOT_CONFIG_H
#define AZURE_IOT_CONFIG_H

/**************************************************/
/******* DO NOT CHANGE the following order ********/
/**************************************************/

/* Include logging header files and define logging macros in the following order:
 * 1. Include the header file "logging_levels.h".
 * 2. Define the LIBRARY_LOG_NAME and LIBRARY_LOG_LEVEL macros depending on
 * the logging configuration for AzureIoT middleware.
 * 3. Include the header file "logging_stack.h", if logging is enabled for AzureIoT middleware.
 */

#include "logging_levels.h"

/* Logging configuration for the AzureIoT middleware library. */
#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME    "AZ_IOT"
#endif

#ifndef LIBRARY_LOG_LEVEL
    #define LIBRARY_LOG_LEVEL    LOG_DEBUG
#endif

/**
 * Overriding the metadata to add timestamp
 * 
 * @todo The timestamp is being added after the log level, change this later
 */
extern const char* getCurrentTime();

#ifndef LOG_METADATA_FORMAT
    #define LOG_METADATA_FORMAT    "[%s] "                 /**< @brief Format of metadata prefix in log messages. */
#endif

#ifndef LOG_METADATA_ARGS
    #define LOG_METADATA_ARGS    getCurrentTime()          /**< @brief Arguments into the metadata logging prefix format. */
#endif

/* Prototype for the function used to print to console on Windows simulator
 * of FreeRTOS.
 * The function prints to the console before the network is connected;
 * then a UDP port after the network has connected. */
extern void vLoggingPrintf( const char * pcFormatString, ... );

/* Map the SdkLog macro to the logging function to enable logging
 * on Windows simulator. */
#ifndef SdkLog
    #define SdkLog( message )    vLoggingPrintf message
#endif

/* Middleware logging */
#if LIBRARY_LOG_LEVEL >= LOG_ERROR
    #define AZLogError( message )    SdkLog( ( "[ERROR] [%s] "LOG_METADATA_FORMAT, LIBRARY_LOG_NAME, LOG_METADATA_ARGS ) ); SdkLog( message ); SdkLog( ( "\r\n" ) )
#endif

#if LIBRARY_LOG_LEVEL >= LOG_INFO
    #define AZLogWarn( message )   SdkLog( ( "[WARN] [%s] "LOG_METADATA_FORMAT, LIBRARY_LOG_NAME, LOG_METADATA_ARGS ) ); SdkLog( message ); SdkLog( ( "\r\n" ) )
#endif

#if LIBRARY_LOG_LEVEL >= LOG_INF
    #define AZLogInfo( message )   SdkLog( ( "[INFO] [%s] "LOG_METADATA_FORMAT, LIBRARY_LOG_NAME, LOG_METADATA_ARGS ) ); SdkLog( message ); SdkLog( ( "\r\n" ) )
#endif

#if LIBRARY_LOG_LEVEL >= LOG_DEBUG
    #define AZLogDebug( message )   SdkLog( ( "[DEBUG] [%s] "LOG_METADATA_FORMAT, LIBRARY_LOG_NAME, LOG_METADATA_ARGS ) ); SdkLog( message ); SdkLog( ( "\r\n" ) )
#endif

#include "logging_stack.h"
/************ End of logging configuration ****************/

#endif /* AZURE_IOT_CONFIG_H */
