/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef PROCESS_CLOUD_MSG_H
#define PROCESS_CLOUD_MSG_H

/* Azure Provisioning/IoT Hub library includes */
#include "azure_iot_hub_client.h"

/**
 * @brief IoT cloud message task.
 * 
 * This task handles,
 *      - Subscriptions
 *      - Direct Methods
 */
void vIoTCloudMessageTask( void );

/**
 * @brief Subscribe to cloud communications
 *          - Properties (serial numbers come in desired properties)
 *          - C2D
 *          - Commands (Direct Methods)
*/
void vSubscribeCloudStuff( void );

/**
 * @brief Unsubscribe cloud communications
 *          - Properties (serial numbers come in desired properties)
 *          - C2D
 *          - Commands (Direct Method)
*/
void vUnsubscribeCloudStuff( void );

/**
 * @brief Cloud message callback handler
 * 
 * @note We do not have any use-case currently
 *
 * @param[in] pxMessage The message payload
 * @param[in] pvContext context/ handle of the iot client
 */
void prvHandleCloudMessage( AzureIoTHubClientCloudToDeviceMessageRequest_t * pxMessage, void * pvContext );

/**
 * @brief Command (Direct Method) message callback handler
 * 
 * Currently the below Direct Methods are implemented,
 *      -> Lock
 *      -> Unlock
 * 
 * @param[in] pxMessage The message payload
 * @param[in] pvContext context/ handle of the iot client
 */
void prvHandleCommand( AzureIoTHubClientCommandRequest_t * pxMessage, void * pvContext );

/**
 * @brief Property message callback handler
 * 
 * @param[in] pxMessage The message payload
 * @param[in] pvContext context/ handle of the iot client
 */
void prvHandlePropertiesMessage( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                                 void * pvContext );

#endif // PROCESS_CLOUD_MSG_H
