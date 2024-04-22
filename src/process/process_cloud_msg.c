/* Copyright (c) Skytree B.V. All rights reserved. */

/* Local includes */
#include "process_cloud_msg.h"
#include "common_definition.h"
#include "controller_data.h"

/* Special Includes */
/**
 * @note This should be included at the end to make sure the
 *       assert is overriden to do a system reset.
*/
#include "assert_override.h"
/*-----------------------------------------------------------*/

/**
 * @brief Timeout for MQTT_ProcessLoop in milliseconds.
 */
#define iotPROCESS_LOOP_TIMEOUT_MS                 ( 10000U )

/**
 * @brief Delay (in ticks) between consecutive cycles of MQTT publish operations in a
 * demo iteration.
 *
 * Note that the process loop also has a timeout, so the total time between
 * publishes is the sum of the two delays.
 */
#define iotDELAY_BETWEEN_PUBLISHES_TICKS           ( pdMS_TO_TICKS( 30000U ) )


#define iotDELAY_PROCESS_LOOP                      ( pdMS_TO_TICKS( 10000U ) )

/**
 * @brief Delay (in ticks) at start/ boot-up.
 * 
 * Note: We do this to allow SKID to send enough telemtry data. We deduct 10s as
 * the boot-up itself can take a bit of time.
*/
#define iotDELAY_AT_START                          ( iotDELAY_BETWEEN_PUBLISHES_TICKS - pdMS_TO_TICKS( 10000U ) )

#define iotDELAY_ON_ERROR                          ( pdMS_TO_TICKS( 5000U ) )

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
#define iotTRANSPORT_SEND_RECV_TIMEOUT_MS          ( 2000U )

/**
 * @brief Wait timeout for subscribe to finish.
 */
#define iotSUBSCRIBE_TIMEOUT                       ( 10 * 1000U )

/**
 * @brief Http Error codes for DirectMethod
*/
#define HTTP_BAD_REQUEST    (400)
#define HTTP_OK             (200)

/**
 * @brief Direct Methods
*/
#define METHOD_LOCK     "lock"
#define METHOD_UNLOCK   "unlock"
/*-----------------------------------------------------------*/

/**
 * @brief IoT cloud message task.
 * 
 * This task handles,
 *      - Subscriptions
 *      - Direct Methods
 * 
 * @param pvParameters Unused
 * 
 * @todo Sending Reported properties (not needed for now)
 */
static void prvIoTCloudMessageTask( void * pvParameters )
{
    ( void ) pvParameters;
    AzureIoTResult_t xResult;

    configPRINTF( ( "---------IoT Cloud Msg Task---------\r\n" ) );

    while( 1 )
    {
        // Wait for the connection
        EventBits_t bits = xEventGroupWaitBits(xConnectionEventGroup, CONNECTED_BIT, pdFALSE, pdFALSE, 3000);

        // Check if the CONNECTED_BIT is set
        if( bits & CONNECTED_BIT )
        {
            LogInfo( ( "Attempt to receive publish message from IoT Hub.\r\n" ) );
            xResult = AzureIoTHubClient_ProcessLoop( &xAzureIoTHubClient, iotPROCESS_LOOP_TIMEOUT_MS );
            
            // @todo, revisit this: if we are already disconnected then no need to assert?
            bits = xEventGroupGetBits(xConnectionEventGroup);
            if( xResult != eAzureIoTSuccess && bits & DISCONNECTED_BIT )
            {
                LogInfo( ( "Process loop failed, do nothing as already in disconnected state\r\n" ) );
            }
            else if( xResult != eAzureIoTSuccess )
            {
                configASSERT( xResult == eAzureIoTSuccess, xResult );
            }
        }
        else
        {
            LogInfo( ( "CONNECTED_BIT not set, bits = %d\r\n", bits ) );
        }
        vTaskDelay( iotDELAY_PROCESS_LOOP );
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Cloud message callback handler
 * 
 * @note We do not have any use-case currently
 *
 * @param[in] pxMessage The message payload
 * @param[in] pvContext context/ handle of the iot client
 */
void prvHandleCloudMessage( AzureIoTHubClientCloudToDeviceMessageRequest_t * pxMessage,
                            void * pvContext )
{
    ( void ) pvContext;

    LogInfo( ( "Cloud message payload : %.*s \r\n",
               ( int ) pxMessage->ulPayloadLength,
               ( const char * ) pxMessage->pvMessagePayload ) );
}
/*-----------------------------------------------------------*/

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
void prvHandleCommand( AzureIoTHubClientCommandRequest_t * pxMessage, void * pvContext )
{
    LogInfo( ( "Command(%.*s) with payload : %.*s \r\n",
               pxMessage->usCommandNameLength,
               ( const char* ) pxMessage->pucCommandName, 
               ( int ) pxMessage->ulPayloadLength,
               ( const char * ) pxMessage->pvMessagePayload ) );

    AzureIoTHubClient_t * xHandle = ( AzureIoTHubClient_t * ) pvContext;

    if(strncmp( ( const char* )pxMessage->pucCommandName, METHOD_LOCK, strlen( METHOD_LOCK ) ) == 0 )
    {
        // @todo this does not work
        send_lock_status();
    }
    else if( strncmp( ( const char* )pxMessage->pucCommandName, METHOD_UNLOCK, strlen( METHOD_LOCK ) ) == 0 )
    {
        // @todo this does not work
        send_unlock_status();
    }
    else
    {
        // method not supported, respond bad request
        char ucCommandPayload[50] = {0};
        sprintf( ucCommandPayload, "{\"error\": \"method(%.*s) not supported\"}", 
                 pxMessage->usCommandNameLength, ( const char* ) pxMessage->pucCommandName );
        if( AzureIoTHubClient_SendCommandResponse( xHandle, pxMessage, HTTP_BAD_REQUEST, ( const uint8_t* ) ucCommandPayload,
                                                   strlen( ucCommandPayload ) ) != eAzureIoTSuccess )
        {
            LogError( ( "Error sending command response\r\n" ) );
        }

        return;
    }

    /**
     * @todo we need to get response from CCU
    */
    if( AzureIoTHubClient_SendCommandResponse( xHandle, pxMessage, HTTP_OK, NULL, 0 ) != eAzureIoTSuccess )
    {
        LogError( ( "Error sending command response\r\n" ) );
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Property message callback handler
 * 
 * @param[in] pxMessage The message payload
 * @param[in] pvContext context/ handle of the iot client
 */
void prvHandlePropertiesMessage( AzureIoTHubClientPropertiesResponse_t * pxMessage, void * pvContext )
{
    ( void ) pvContext;

    switch( pxMessage->xMessageType )
    {
        case eAzureIoTHubPropertiesRequestedMessage:
            LogInfo( ( "Property document payload : %.*s \r\n",
               ( int ) pxMessage->ulPayloadLength,
               ( const char * ) pxMessage->pvMessagePayload ) );
            ParseSystemProperties(
                ( const uint8_t * ) pxMessage->pvMessagePayload, strlen(( const char* )pxMessage->pvMessagePayload), &xSystemProperties );
            
            /**
             * @todo After the separate cloud msg task is re-enabled, rework on this
            */
            // xEventGroupSetBits(xConnectionEventGroup, PROPERTIES_RECEIVED_BIT);
            break;

        case eAzureIoTHubPropertiesReportedResponseMessage:
            LogInfo( ( "Device property reported property response received" ) );
            break;

        case eAzureIoTHubPropertiesWritablePropertyMessage:
            LogInfo( ( "Desired property document payload : %.*s \r\n",
               ( int ) pxMessage->ulPayloadLength,
               ( const char * ) pxMessage->pvMessagePayload ) );
            ParseDesiredProperties(
                ( const uint8_t * ) pxMessage->pvMessagePayload, strlen(( const char* )pxMessage->pvMessagePayload), &xSystemProperties );
            break;

        default:
            LogError( ( "Unknown property message" ) );
            break;
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Subscribe to cloud communications
 *          - Properties (serial numbers come in desired properties)
 *          - C2D
 *          - Commands (Direct Methods)
*/
void vSubscribeCloudStuff( void )
{
    AzureIoTResult_t xResult;

    xResult = AzureIoTHubClient_SubscribeCloudToDeviceMessage( &xAzureIoTHubClient, prvHandleCloudMessage,
                                                               &xAzureIoTHubClient, iotSUBSCRIBE_TIMEOUT );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    xResult = AzureIoTHubClient_SubscribeCommand( &xAzureIoTHubClient, prvHandleCommand,
                                                  &xAzureIoTHubClient, iotSUBSCRIBE_TIMEOUT );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    xResult = AzureIoTHubClient_SubscribeProperties( &xAzureIoTHubClient, prvHandlePropertiesMessage,
                                                     &xAzureIoTHubClient, iotSUBSCRIBE_TIMEOUT );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    /* Get property document after initial connection */
    xResult = AzureIoTHubClient_RequestPropertiesAsync( &xAzureIoTHubClient );
    configASSERT( xResult == eAzureIoTSuccess, xResult );
}
/*-----------------------------------------------------------*/

/**
 * @brief Unsubscribe cloud communications
 *          - Properties (serial numbers come in desired properties)
 *          - C2D
 *          - Commands (Direct Method)
*/
void vUnsubscribeCloudStuff( void )
{
    AzureIoTResult_t xResult;

    xResult = AzureIoTHubClient_UnsubscribeProperties( &xAzureIoTHubClient );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    xResult = AzureIoTHubClient_UnsubscribeCommand( &xAzureIoTHubClient );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    xResult = AzureIoTHubClient_UnsubscribeCloudToDeviceMessage( &xAzureIoTHubClient );
    configASSERT( xResult == eAzureIoTSuccess, xResult );
}
/*-----------------------------------------------------------*/

/**
 * @brief IoT cloud message task.
 * 
 * This task handles,
 *      - Subscriptions
 *      - Direct Methods
 */
void vIoTCloudMessageTask( void )
{
    xTaskCreate( prvIoTCloudMessageTask,            /* Function that implements the task. */
                 "IoT_cloud_message_task",          /* Text name for the task - only used for debugging. */
                 1024U,                             /* Size of stack (in words, not bytes) to allocate for the task. */
                 NULL,                              /* Task parameter - not used in this case. */
                 tskIDLE_PRIORITY + 2,              /* Task priority, must be between 0 and configMAX_PRIORITIES - 1. */
                 NULL );                            /* Used to pass out a handle to the created task - not used in this case. */
}
/*-----------------------------------------------------------*/
