/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef COMMON_DEFINITION_H
#define COMMON_DEFINITION_H

/* Local includes */
#include "properties.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

/* Azure IoT Hub library includes */
#include "azure_iot_hub_client.h"

/**
 * @brief Event flags for tasks synchronization
*/
#define CONNECTED_BIT               (1 << 0)
#define DISCONNECTED_BIT            (1 << 1)
#define PROPERTIES_RECEIVED_BIT     (1 << 2)

/**
 * @brief Event group for task synchronization
*/
extern EventGroupHandle_t xConnectionEventGroup;

/**
 * @brief System properties (CCU, Units and Tank) received
 *        as desired properties from IoTHub.
*/
extern SYSTEM_Properties_t xSystemProperties;

/**
 * @brief IoTHub client handle.
*/
extern AzureIoTHubClient_t xAzureIoTHubClient;

#endif // COMMON_DEFINITION_H
