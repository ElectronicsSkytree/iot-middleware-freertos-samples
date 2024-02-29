/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#ifndef CORE_MQTT_CONFIG_H
#define CORE_MQTT_CONFIG_H

/**************************************************/
/******* DO NOT CHANGE the following order ********/
/**************************************************/

/* Include logging header files and define logging macros in the following order:
 * 1. Include the header file "logging_levels.h".
 * 2. Define the LIBRARY_LOG_NAME and LIBRARY_LOG_LEVEL macros depending on
 * the logging configuration for MQTT.
 * 3. Include the header file "logging_stack.h", if logging is enabled for MQTT.
 */

#include "logging_levels.h"

/* Logging configuration for the MQTT library. */
#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME    "MQTT"
#endif

#ifndef LIBRARY_LOG_LEVEL
    #define LIBRARY_LOG_LEVEL    LOG_INFO
#endif

/**
 * Overriding the metadata to add timestamp
 * 
 * @todo The timestamp is being added after the log level, change this later
 */
extern const char* getCurrentTime();

#ifndef LOG_METADATA_FORMAT
    #define LOG_METADATA_FORMAT    "[%s] [%s:%d] "                  /**< @brief Format of metadata prefix in log messages. */
#endif

#ifndef LOG_METADATA_ARGS
    #define LOG_METADATA_ARGS    getCurrentTime(), __FUNCTION__, __LINE__  /**< @brief Arguments into the metadata logging prefix format. */
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

#include "logging_stack.h"
/************ End of logging configuration ****************/

/**
 * @brief The maximum number of MQTT PUBLISH messages that may be pending
 * acknowledgement at any time.
 *
 * QoS 1 and 2 MQTT PUBLISHes require acknowledgment from the server before
 * they can be completed. While they are awaiting the acknowledgment, the
 * client must maintain information about their state. The value of this
 * macro sets the limit on how many simultaneous PUBLISH states an MQTT
 * context maintains.
 */
#define MQTT_STATE_ARRAY_MAX_COUNT    10U

#endif /* ifndef CORE_MQTT_CONFIG_H */
