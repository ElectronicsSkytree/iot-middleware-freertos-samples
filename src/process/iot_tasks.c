/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/* Copyright (c) Skytree B.V. All rights reserved. */

/* Local includes */
#include "config.h"
#include "errors.h"
#include "controller_data.h"
#include "main.h"
#include "time_wrapper.h"
#include "connection.h"
#include "telemetry.h"
#include "properties.h"

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

/* Azure Provisioning/IoT Hub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_provisioning_client.h"

/* Exponential backoff retry include. */
#include "backoff_algorithm.h"

/* Transport interface implementation include header for TLS. */
#include "transport_tls_socket.h"

/* Crypto helper header. */
#include "azure_sample_crypto.h"

/* Special Includes */
/**
 * @note This should be included at the end to make sure the
 *       assert is overriden to do a system reset.
 * 
 * @todo To improve this later instead of forced reset.
*/
#include "assert_override.h"

/*-----------------------------------------------------------*/

/* Compile time error for undefined configs. */
#if !defined( configHOSTNAME ) && !defined( enableDPS )
    #error "Define the config configHOSTNAME by following the instructions in file config.h."
#endif

#if !defined( dpsENDPOINT ) && defined( enableDPS )
    #error "Define the config dps endpoint by following the instructions in file config.h."
#endif

#ifndef configROOT_CA_PEM
    #error "Please define Root CA certificate of the IoT Hub(configROOT_CA_PEM) in config.h."
#endif

#if defined( deviceSYMMETRIC_KEY ) && defined( clientCERTIFICATE_PEM )
    #error "Please define only one auth deviceSYMMETRIC_KEY or clientCERTIFICATE_PEM in config.h."
#endif

#if !defined( deviceSYMMETRIC_KEY ) && !defined( clientCERTIFICATE_PEM )
    #error "Please define one auth deviceSYMMETRIC_KEY or clientCERTIFICATE_PEM in config.h."
#endif

/*-----------------------------------------------------------*/

/**
 * @brief The maximum number of retries for network operation with server.
 */
#define iotRETRY_MAX_ATTEMPTS                      ( 5U )

/**
 * @brief The maximum back-off delay (in milliseconds) for retrying failed operation
 *  with server.
 */
#define iotRETRY_MAX_BACKOFF_DELAY_MS              ( 5000U )

/**
 * @brief The base back-off delay (in milliseconds) to use for network operation retry
 * attempts.
 */
#define iotRETRY_BACKOFF_BASE_MS                   ( 500U )

/**
 * @brief Timeout for receiving CONNACK packet in milliseconds.
 */
#define iotCONNACK_RECV_TIMEOUT_MS                 ( 10 * 1000U )

/**
 * @brief  The content type of the Telemetry message published in this example.
 * @remark Message properties must be url-encoded.
 *         This message property is not required to send telemetry.
 */
#define iotMESSAGE_CONTENT_TYPE                    "application%2Fjson"

/**
 * @brief  The content encoding of the Telemetry message published in this example.
 * @remark Message properties must be url-encoded.
 *         This message property is not required to send telemetry.
 */
#define iotMESSAGE_CONTENT_ENCODING                "utf-8"

/**
 * @brief Time in ticks to wait between each cycle of the demo implemented
 * by prvMQTTDemoTask().
 */
#define iotDELAY_BETWEEN_ITERATIONS_TICKS     ( pdMS_TO_TICKS( 5000U ) )

/**
 * @brief communication task reconnects after this interval
 * 
 * @todo we are doing this as we have seen crashes at WiFi/TLS layer
*/
#define iotDELAY_RECONNECTION_AFTER     ( pdMS_TO_TICKS( 2400000U ) )

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
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
#define iotProvisioning_Registration_TIMEOUT_MS    ( 3 * 1000U )

/**
 * @brief Wait timeout for subscribe to finish.
 */
#define iotSUBSCRIBE_TIMEOUT                       ( 10 * 1000U )

/**
 * @brief Http Error codes for DirectMethod
*/
#define HTTP_BAD_REQUEST    400
#define HTTP_OK             200

/**
 * @brief Direct Methods
*/
#define METHOD_LOCK     "lock"
#define METHOD_UNLOCK   "unlock"

/*-----------------------------------------------------------*/

/* Define buffer for IoT Hub info.  */
#ifdef enableDPS
    static uint8_t ucSampleIotHubHostname[ 128 ];
    static uint8_t ucSampleIotHubDeviceId[ 128 ];
    static AzureIoTProvisioningClient_t xAzureIoTProvisioningClient;
#endif /* enableDPS */

/* Each compilation unit must define the NetworkContext struct. */
struct NetworkContext
{
    void * pParams;
};

static AzureIoTHubClient_t xAzureIoTHubClient;

#define SCRATCH_BUFFER_LENGTH 3200
#define PROPERTY_BUFFER_LENGTH 80
static uint8_t ucPropertyBuffer[ PROPERTY_BUFFER_LENGTH ];
static uint8_t ucScratchBuffer[ SCRATCH_BUFFER_LENGTH ];

/**
 * @brief For sending boot-up only once.
 * @todo Rely on CCU rather
*/
static bool xBootUpMessageSent = false;

// Alert related
static sequence_state_t xSkidLastState;
static sequence_state_t xUnitLastState;

/**
 * @brief System properties (CCU, Units and Tank) received
 *        as desired properties from IoTHub.
*/
SYSTEM_Properties_t xSystemProperties;

/**
 * @brief Event flags for tasks synchronization
*/
#define CONNECTED_BIT               (1 << 0)
#define DISCONNECTED_BIT            (1 << 1)
#define PROPERTIES_RECEIVED_BIT     (1 << 2)
extern EventGroupHandle_t xConnectionEventGroup;

/*-----------------------------------------------------------*/

#ifdef enableDPS

/**
 * @brief Gets the IoT Hub endpoint and deviceId from Provisioning service.
 *   This function will block for Provisioning service for result or return failure.
 *
 * @param[in] pXNetworkCredentials  Network credential used to connect to Provisioning service
 * @param[out] ppucIothubHostname  Pointer to uint8_t* IoT Hub hostname return from Provisioning Service
 * @param[in,out] pulIothubHostnameLength  Length of hostname
 * @param[out] ppucIothubDeviceId  Pointer to uint8_t* deviceId return from Provisioning Service
 * @param[in,out] pulIothubDeviceIdLength  Length of deviceId
 */
static void prvIoTHubInfoGet( NetworkCredentials_t * pXNetworkCredentials,
                              uint8_t ** ppucIothubHostname,
                              uint32_t * pulIothubHostnameLength,
                              uint8_t ** ppucIothubDeviceId,
                              uint32_t * pulIothubDeviceIdLength );

#endif /* enableDPS */

/**
 * @brief IoT telemetry task.
 * 
 * IoT telemetry is sent every 30 seconds through a timer.
 * 
 * @param pvParameters @todo
 */
static void prvIoTTelemetryTask( void * pvParameters );

/**
 * @brief IoT cloud message task.
 * 
 * This task handles,
 *      - Subscriptions
 *      - Direct Methods
 * 
 * @param pvParameters @todo
 */
static void prvIoTCloudMessageTask( void * pvParameters );

/**
 * @brief The task to initiate and manage MQTT connections/ APIs.
 *
 * @param[in] pvParameters Parameters as passed at the time of task creation. Not
 * used in this example.
 */
static void prvIoTBackendConnectionTask( void * pvParameters );

/**
 * @brief Connect to endpoint with reconnection retries.
 *
 * If connection fails, retry is attempted after a timeout.
 * Timeout value will exponentially increase until maximum
 * timeout value is reached or the number of attempts are exhausted.
 *
 * @param pcHostName Hostname of the endpoint to connect to.
 * @param ulPort Endpoint port.
 * @param pxNetworkCredentials Pointer to Network credentials.
 * @param pxNetworkContext Point to Network context created.
 * @return uint32_t The status of the final connection attempt.
 */
static uint32_t prvConnectToServerWithBackoffRetries( const char * pcHostName,
                                                      uint32_t ulPort,
                                                      NetworkCredentials_t * pxNetworkCredentials,
                                                      NetworkContext_t * pxNetworkContext );
/*-----------------------------------------------------------*/

/**
 * @brief Static buffer used to hold MQTT messages being sent and received.
 */
static uint8_t ucMQTTMessageBuffer[ configNETWORK_BUFFER_SIZE ];

/*-----------------------------------------------------------*/

/**
 * @brief Cloud message callback handler
 *
 * @param[in] pxMessage The message payload
 * @param[in] pvContext context/ handle of the iot client
 */
static void prvHandleCloudMessage( AzureIoTHubClientCloudToDeviceMessageRequest_t * pxMessage,
                                   void * pvContext )
{
    ( void ) pvContext;

    LogInfo( ( "Cloud message payload : %.*s \r\n",
               ( int ) pxMessage->ulPayloadLength,
               ( const char * ) pxMessage->pvMessagePayload ) );
}
/*-----------------------------------------------------------*/

/**
 * @brief Command message callback handler
 * 
 * Currently the below Direct Methods are implemented,
 *      -> Lock
 *      -> Unlock
 * 
 * @param[in] pxMessage The message payload
 * @param[in] pvContext context/ handle of the iot client
 */
static void prvHandleCommand( AzureIoTHubClientCommandRequest_t * pxMessage, void * pvContext )
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
            LogInfo( ( "Error sending command response\r\n" ) );
        }

        return;
    }

    /**
     * @todo we need to get response from CCU
    */
    if( AzureIoTHubClient_SendCommandResponse( xHandle, pxMessage, HTTP_OK, NULL, 0 ) != eAzureIoTSuccess )
    {
        LogInfo( ( "Error sending command response\r\n" ) );
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Property mesage callback handler
 * 
 * @param[in] pxMessage The message payload
 * @param[in] pvContext context/ handle of the iot client
 */
static void prvHandlePropertiesMessage( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                                        void * pvContext )
{
    ( void ) pvContext;

    switch( pxMessage->xMessageType )
    {
        case eAzureIoTHubPropertiesRequestedMessage:
            LogInfo( ( "Device property document GET received" ) );
            ParseSystemProperties(
                ( const uint8_t * ) pxMessage->pvMessagePayload, strlen(( const char* )pxMessage->pvMessagePayload), &xSystemProperties );
            break;

        case eAzureIoTHubPropertiesReportedResponseMessage:
            LogInfo( ( "Device property reported property response received" ) );
            break;

        case eAzureIoTHubPropertiesWritablePropertyMessage:
            LogInfo( ( "Device property desired property received" ) );
            ParseDesiredProperties(
                ( const uint8_t * ) pxMessage->pvMessagePayload, strlen(( const char* )pxMessage->pvMessagePayload), &xSystemProperties );
            break;

        default:
            LogError( ( "Unknown property message" ) );
            break;
    }

    LogInfo( ( "Property document payload : %.*s \r\n",
               ( int ) pxMessage->ulPayloadLength,
               ( const char * ) pxMessage->pvMessagePayload ) );

    xEventGroupSetBits(xConnectionEventGroup, PROPERTIES_RECEIVED_BIT);
}
/*-----------------------------------------------------------*/

/**
 * @brief Setup transport credentials.
 * 
 * @param pxNetworkCredentials Pointer to network credentials struct.
 */
static void prvSetupNetworkCredentials( NetworkCredentials_t * pxNetworkCredentials )
{
    pxNetworkCredentials->xDisableSni = pdFALSE;

    /* Set the credentials for establishing a TLS connection. */
    pxNetworkCredentials->pucRootCa = ( const unsigned char * ) configROOT_CA_PEM;
    pxNetworkCredentials->xRootCaSize = sizeof( configROOT_CA_PEM );

    #ifdef clientCERTIFICATE_SUPPORTED
        pxNetworkCredentials->pucClientCert = ( const unsigned char * ) clientCERTIFICATE_PEM;
        pxNetworkCredentials->xClientCertSize = sizeof( clientCERTIFICATE_PEM );
        pxNetworkCredentials->pucPrivateKey = ( const unsigned char * ) clientPRIVATE_KEY_PEM;
        pxNetworkCredentials->xPrivateKeySize = sizeof( clientPRIVATE_KEY_PEM );
    #endif
}
/*-----------------------------------------------------------*/

/**
 * @brief IoT telemetry task.
 * 
 * IoT telemetry is sent every 30 seconds through a timer.
 * 
 * @param pvParameters @todo
 */
static void prvIoTTelemetryTask( void * pvParameters )
{
    ( void ) pvParameters;
    AzureIoTMessageProperties_t xPropertyBag;
    AzureIoTResult_t xResult;
    uint32_t ulScratchBufferLength = 0U;

#ifdef TESTING_IN_SINGLE_TASK
    uint8_t count = 10;
#else
    configPRINTF( ( "---------IoT telemetry Task---------\r\n" ) );
#endif

    /* Initialized the properties bag */
    xResult = AzureIoTMessage_PropertiesInit( &xPropertyBag, ucPropertyBuffer, 0, sizeof( ucPropertyBuffer ) );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    /* Sending a default property (Content-Type). */
    xResult = AzureIoTMessage_PropertiesAppend( &xPropertyBag,
                                                ( uint8_t * ) AZ_IOT_MESSAGE_PROPERTIES_CONTENT_TYPE, sizeof( AZ_IOT_MESSAGE_PROPERTIES_CONTENT_TYPE ) - 1,
                                                ( uint8_t * ) iotMESSAGE_CONTENT_TYPE, sizeof( iotMESSAGE_CONTENT_TYPE ) - 1 );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    /* Sending a default property (Content-Encoding). */
    xResult = AzureIoTMessage_PropertiesAppend( &xPropertyBag,
                                                ( uint8_t * ) AZ_IOT_MESSAGE_PROPERTIES_CONTENT_ENCODING, sizeof( AZ_IOT_MESSAGE_PROPERTIES_CONTENT_ENCODING ) - 1,
                                                ( uint8_t * ) iotMESSAGE_CONTENT_ENCODING, sizeof( iotMESSAGE_CONTENT_ENCODING ) - 1 );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    /**
     * @todo SDK crashes sometimes because of WiFi/ TLS issues,
     * so ideally we are supposed to send boot-up only when SKID/Unit restarts
    */
    while(1) {
#ifndef TESTING_IN_SINGLE_TASK
        // Wait for the connection
        EventBits_t bits = xEventGroupWaitBits(xConnectionEventGroup, PROPERTIES_RECEIVED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

        // Check if the PROPERTIES_RECEIVED_BIT is set
        if (bits & PROPERTIES_RECEIVED_BIT)
#endif
        {
            memset(ucScratchBuffer, '\0', sizeof (ucScratchBuffer) );
            // Create the json boot-up msg payload
            ulScratchBufferLength = CreateBootUpMessage( ucScratchBuffer, sizeof( ucScratchBuffer ) );
            LogInfo( ( "boot-up msg: ucScratchBuffer = %s and ulScratchBufferLength =%d\r\n", ucScratchBuffer, ulScratchBufferLength ) );

            xResult = AzureIoTHubClient_SendTelemetry( &xAzureIoTHubClient,
                                                        ucScratchBuffer, ulScratchBufferLength,
                                                        &xPropertyBag, eAzureIoTHubMessageQoS1, NULL );
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            // Not to be sent again
            xBootUpMessageSent = true;

            break;
        }
    }

    /* Idle for some time so that telemetry is populated first time upon boot. */
    LogInfo( ( "On boot-up: Keeping Connection Idle for %d seconds...\r\n\r\n", iotDELAY_AT_START / 1000 ) );
    vTaskDelay( iotDELAY_AT_START );

#ifdef TESTING_IN_SINGLE_TASK
    while(count >= 0)
    {
#else
    while(1)
    {
        // Wait until the connection is established
        while (1) {
            // Wait for the connection event to be set
            EventBits_t bits = xEventGroupWaitBits(xConnectionEventGroup, PROPERTIES_RECEIVED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

            // Check if the PROPERTIES_RECEIVED_BIT is set
            if (bits & PROPERTIES_RECEIVED_BIT) {
                break;
            }
        }
#endif
        // Read the current sensor data
        SKID_iot_status_t skid_data = get_skid_status(xSkidLastState);
        xSkidLastState = skid_data.skid_state;

        // Read the current sensor data
        UNIT_iot_status_t unit_data = get_unit_status(xUnitLastState);
        xUnitLastState = unit_data.unit_state;

        memset(ucScratchBuffer, '\0', sizeof (ucScratchBuffer) );

        // @todo error handling till we jump to new code
        if( (skid_data.skid_state == Lock_State && skid_data.send_alert) ||
            (unit_data.unit_state == Lock_State && unit_data.send_alert) )
        {
            // Create the json error msg payload
            ulScratchBufferLength = CreateErrorMessage( skid_data, unit_data, ucScratchBuffer, sizeof( ucScratchBuffer ) );
            LogInfo( ( "error msg: ucScratchBuffer = %s and ulScratchBufferLength =%d\r\n", ucScratchBuffer, ulScratchBufferLength ) );

            xResult = AzureIoTHubClient_SendTelemetry( &xAzureIoTHubClient, ucScratchBuffer, ulScratchBufferLength,
                                                       &xPropertyBag, eAzureIoTHubMessageQoS1, NULL );
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            /* Idle for some time so that telemetry is populated first time upon boot. */
            LogInfo( ( "On error: Keeping Connection Idle for %d seconds...\r\n\r\n", iotDELAY_ON_ERROR / 1000 ) );
            vTaskDelay( iotDELAY_ON_ERROR );

            // Continue so that we check for the PROPERTIES_RECEIVED_BIT again
            continue;
        }

        // Create the json telemetry payload
        // @todo We are sending single unit data, ok for now till we re-work on this
        ulScratchBufferLength = CreateTelemetry( skid_data, unit_data, ucScratchBuffer, sizeof( ucScratchBuffer ) );
        LogInfo( ( "ucScratchBuffer = %s and ulScratchBufferLength =%d\r\n", ucScratchBuffer, ulScratchBufferLength ) );

        xResult = AzureIoTHubClient_SendTelemetry( &xAzureIoTHubClient, ucScratchBuffer, ulScratchBufferLength,
                                                   &xPropertyBag, eAzureIoTHubMessageQoS1, NULL );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

#ifdef TESTING_IN_SINGLE_TASK
        count--;
#endif

        /* Leave Connection Idle for some time. */
        LogInfo( ( "Keeping Connection Idle for %d seconds...\r\n\r\n", iotDELAY_BETWEEN_PUBLISHES_TICKS / 1000 ) );
        vTaskDelay( iotDELAY_BETWEEN_PUBLISHES_TICKS );
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Subscribe to cloud communications
 *          - Properties (serial numbers come in desired properties)
 *          - C2D
 *          - Commands (Direct Methods)
*/
static void prvSubscribeCloudStuff()
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
static void prvUnsubscribeCloudStuff()
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

    while(1) {
        // Wait for the connection
        EventBits_t bits = xEventGroupWaitBits(xConnectionEventGroup, CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

        // Check if the CONNECTED_BIT is set
        if (bits & CONNECTED_BIT) {
            LogInfo( ( "Attempt to receive publish message from IoT Hub.\r\n" ) );
            xResult = AzureIoTHubClient_ProcessLoop( &xAzureIoTHubClient, iotPROCESS_LOOP_TIMEOUT_MS );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        }

        vTaskDelay( iotDELAY_PROCESS_LOOP );
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Azure IoT backend connection task.
 * 
 * The task to connect to Azure IoT Hub and also to handle
 * retry logics with backoff algorithm for TLS connection.
 * 
 * @param[in] pvParameters task parameters, unused
 */
static void prvIoTBackendConnectionTask( void * pvParameters )
{
    ( void ) pvParameters;
    NetworkCredentials_t xNetworkCredentials = { 0 };
    AzureIoTTransportInterface_t xTransport;
    NetworkContext_t xNetworkContext = { 0 };
    TlsTransportParams_t xTlsTransportParams = { 0 };
    AzureIoTResult_t xResult;
    uint32_t ulStatus;
    AzureIoTHubClientOptions_t xHubOptions = { 0 };
    bool xSessionPresent;

    #ifdef enableDPS
        uint8_t * pucIotHubHostname = NULL;
        uint8_t * pucIotHubDeviceId = NULL;
        uint32_t pulIothubHostnameLength = 0;
        uint32_t pulIothubDeviceIdLength = 0;
    #else
        uint8_t * pucIotHubHostname = ( uint8_t * ) configHOSTNAME;
        uint8_t * pucIotHubDeviceId = ( uint8_t * ) configDEVICE_ID;
        uint32_t pulIothubHostnameLength = sizeof( configHOSTNAME ) - 1;
        uint32_t pulIothubDeviceIdLength = sizeof( configDEVICE_ID ) - 1;
    #endif /* enableDPS */

    configPRINTF( ( "---------IoT backend connection Task---------\r\n" ) );

    /* Initialize Azure IoT Middleware.  */
    xResult = AzureIoT_Init();
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    prvSetupNetworkCredentials( &xNetworkCredentials );

    #ifdef enableDPS
        /* Error handling is inside through assert */
        prvIoTHubInfoGet( &xNetworkCredentials, &pucIotHubHostname, &pulIothubHostnameLength,
                          &pucIotHubDeviceId, &pulIothubDeviceIdLength );
    #endif /* enableDPS */

    xNetworkContext.pParams = &xTlsTransportParams;

    while(1)
    {
        configASSERT( IsConnectedToInternet() == true, false );

        /* Retry connection with a backoff */
        ulStatus = prvConnectToServerWithBackoffRetries( ( const char * ) pucIotHubHostname,
                                                         plainIOTHUB_PORT,
                                                         &xNetworkCredentials, &xNetworkContext );
        configASSERT( ulStatus == 0, ulStatus );

        /* Transport send and receive function pointers. */
        xTransport.pxNetworkContext = &xNetworkContext;
        xTransport.xSend = TLS_Socket_Send;
        xTransport.xRecv = TLS_Socket_Recv;

        /* Init IoT Hub option */
        xResult = AzureIoTHubClient_OptionsInit( &xHubOptions );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        xResult = AzureIoTHubClient_Init( &xAzureIoTHubClient,
                                          pucIotHubHostname, pulIothubHostnameLength,
                                          pucIotHubDeviceId, pulIothubDeviceIdLength,
                                          &xHubOptions,
                                          ucMQTTMessageBuffer, sizeof( ucMQTTMessageBuffer ),
                                          GetUnixTime,
                                          &xTransport );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        // @todo rather have a macro if sysmmetric key way and get key through other means
        #ifdef deviceSYMMETRIC_KEY
            xResult = AzureIoTHubClient_SetSymmetricKey( &xAzureIoTHubClient, ( const uint8_t * ) deviceSYMMETRIC_KEY,
                                                         sizeof( deviceSYMMETRIC_KEY ) - 1, Crypto_HMAC );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        #endif /* deviceSYMMETRIC_KEY */

        /* Sends an MQTT Connect packet over the already established TLS connection,
         * and waits for connection acknowledgment (CONNACK) packet. */
        LogInfo( ( "Creating an MQTT connection to %s.\r\n", pucIotHubHostname ) );

        /* @todo We are setting clean session to false always, fine? */
        xResult = AzureIoTHubClient_Connect( &xAzureIoTHubClient, false, &xSessionPresent,
                                             iotCONNACK_RECV_TIMEOUT_MS );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

#ifdef TESTING_IN_SINGLE_TASK
        int p = 0;
        prvIoTTelemetryTask(&p);

        LogInfo( ( "Connection task will reconnect after (%d seconds) .... \r\n\r\n", iotDELAY_BETWEEN_ITERATIONS_TICKS / 1000 ) );
        vTaskDelay( iotDELAY_BETWEEN_ITERATIONS_TICKS );
#else

        /**
         * Subscribe for the cloud communications
         * @note The process_loop is in cloud msg task, so everything is received
         *       in cloud msg task.
         */
        prvSubscribeCloudStuff();

        // Set the CONNECTED_BIT so that other tasks continue their work
        xEventGroupSetBits(xConnectionEventGroup, CONNECTED_BIT);
        xEventGroupClearBits(xConnectionEventGroup, DISCONNECTED_BIT);

        LogInfo( ( "Connection task will reconnect after (%d seconds) .... \r\n\r\n", iotDELAY_RECONNECTION_AFTER / 1000 ) );
        vTaskDelay( iotDELAY_RECONNECTION_AFTER );

        // Time to do a reconnection (well this is the limitation we have)
        xEventGroupSetBits(xConnectionEventGroup, DISCONNECTED_BIT);
        xEventGroupClearBits(xConnectionEventGroup, CONNECTED_BIT);
#endif

        /* Before going down, unsubscribe from cloud msgs */
        if( IsConnectedToInternet() )
        {
            prvUnsubscribeCloudStuff();

            xResult = AzureIoTHubClient_Disconnect( &xAzureIoTHubClient );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        }

        /* Close the network connection.  */
        TLS_Socket_Disconnect( &xNetworkContext );
    }
}
/*-----------------------------------------------------------*/

#ifdef enableDPS

/**
 * @brief Gets the IoT Hub endpoint and deviceId from Provisioning service.
 * 
 * @note This function will block for Provisioning service for result or return failure.
 *
 * @param[in] pXNetworkCredentials  Network credential used to connect to Provisioning service
 * @param[out] ppucIothubHostname  Pointer to uint8_t* IoT Hub hostname return from Provisioning Service
 * @param[in,out] pulIothubHostnameLength  Length of hostname
 * @param[out] ppucIothubDeviceId  Pointer to uint8_t* deviceId return from Provisioning Service
 * @param[in,out] pulIothubDeviceIdLength  Length of deviceId
 */
static void prvIoTHubInfoGet( 
    NetworkCredentials_t * pXNetworkCredentials, uint8_t ** ppucIothubHostname,
    uint32_t * pulIothubHostnameLength, uint8_t ** ppucIothubDeviceId, uint32_t * pulIothubDeviceIdLength )
{
    NetworkContext_t xNetworkContext = { 0 };
    TlsTransportParams_t xTlsTransportParams = { 0 };
    AzureIoTResult_t xResult;
    AzureIoTTransportInterface_t xTransport;
    uint32_t ucSamplepIothubHostnameLength = sizeof( ucSampleIotHubHostname );
    uint32_t ucSamplepIothubDeviceIdLength = sizeof( ucSampleIotHubDeviceId );
    uint32_t ulStatus;

    /* Set the pParams member of the network context with desired transport. */
    xNetworkContext.pParams = &xTlsTransportParams;

    ulStatus = prvConnectToServerWithBackoffRetries( dpsENDPOINT, plainIOTHUB_PORT,
                                                        pXNetworkCredentials, &xNetworkContext );
    configASSERT( ulStatus == 0, ulStatus );

    /* Fill in Transport Interface send and receive function pointers. */
    xTransport.pxNetworkContext = &xNetworkContext;
    xTransport.xSend = TLS_Socket_Send;
    xTransport.xRecv = TLS_Socket_Recv;

    #ifdef configUSE_HSM
        /* Redefine the configREGISTRATION_ID macro using registration ID
         * generated dynamically using the HSM */

        /* We use a pointer instead of a buffer so that the getRegistrationId
         * function can allocate the necessary memory depending on the HSM */
        char * registration_id = NULL;
        ulStatus = getRegistrationId( &registration_id );
        configASSERT( ulStatus == 0 );
        #undef configREGISTRATION_ID
        #define configREGISTRATION_ID    registration_id
    #endif

    xResult = AzureIoTProvisioningClient_Init( &xAzureIoTProvisioningClient, ( const uint8_t * ) dpsENDPOINT,
                                               sizeof( dpsENDPOINT ) - 1, ( const uint8_t * ) configID_SCOPE,
                                               sizeof( configID_SCOPE ) - 1, ( const uint8_t * ) configREGISTRATION_ID,
                                               #ifdef configUSE_HSM
                                                   strlen( configREGISTRATION_ID ),
                                               #else
                                                   sizeof( configREGISTRATION_ID ) - 1,
                                               #endif
                                               NULL, ucMQTTMessageBuffer, sizeof( ucMQTTMessageBuffer ),
                                               GetUnixTime, &xTransport );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    #ifdef deviceSYMMETRIC_KEY
        xResult = AzureIoTProvisioningClient_SetSymmetricKey( &xAzureIoTProvisioningClient,
                                                              ( const uint8_t * ) deviceSYMMETRIC_KEY,
                                                              sizeof( deviceSYMMETRIC_KEY ) - 1,
                                                              Crypto_HMAC );
        configASSERT( xResult == eAzureIoTSuccess, xResult );
    #endif /* deviceSYMMETRIC_KEY */

    do
    {
        xResult = AzureIoTProvisioningClient_Register( &xAzureIoTProvisioningClient, iotProvisioning_Registration_TIMEOUT_MS );
    } while( xResult == eAzureIoTErrorPending );

    configASSERT( xResult == eAzureIoTSuccess, xResult );

    xResult = AzureIoTProvisioningClient_GetDeviceAndHub( &xAzureIoTProvisioningClient,
                                                          ucSampleIotHubHostname, &ucSamplepIothubHostnameLength,
                                                          ucSampleIotHubDeviceId, &ucSamplepIothubDeviceIdLength );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    AzureIoTProvisioningClient_Deinit( &xAzureIoTProvisioningClient );

    /* Close the network connection.  */
    TLS_Socket_Disconnect( &xNetworkContext );

    *ppucIothubHostname = ucSampleIotHubHostname;
    *pulIothubHostnameLength = ucSamplepIothubHostnameLength;
    *ppucIothubDeviceId = ucSampleIotHubDeviceId;
    *pulIothubDeviceIdLength = ucSamplepIothubDeviceIdLength;
}
/*-----------------------------------------------------------*/

#endif /* enableDPS */
/*-----------------------------------------------------------*/

/**
 * @brief Connect to server with backoff retries.
 * 
 * Attempt to establish TLS session with IoT Hub. If connection fails,
 * retry after a timeout. Timeout value will be exponentially increased
 * until  the maximum number of attempts are reached or the maximum timeout
 * value is reached. The function returns a failure status if the TCP
 * connection cannot be established to the IoT Hub after the configured
 * number of attempts.
 */
static uint32_t prvConnectToServerWithBackoffRetries( 
    const char * pcHostName, uint32_t port, NetworkCredentials_t * pxNetworkCredentials, NetworkContext_t * pxNetworkContext )
{
    TlsTransportStatus_t xNetworkStatus;
    BackoffAlgorithmStatus_t xBackoffAlgStatus = BackoffAlgorithmSuccess;
    BackoffAlgorithmContext_t xReconnectParams;
    uint16_t usNextRetryBackOff = 0U;

    /* Initialize reconnect attempts and interval. */
    BackoffAlgorithm_InitializeParams( &xReconnectParams,
                                       iotRETRY_BACKOFF_BASE_MS,
                                       iotRETRY_MAX_BACKOFF_DELAY_MS,
                                       iotRETRY_MAX_ATTEMPTS );

    /* Attempt to connect to IoT Hub. If connection fails, retry after
     * a timeout. Timeout value will exponentially increase till maximum
     * attempts are reached. */
    do
    {
        configPRINTF( ( "Creating a TLS connection to %s:%lu.\r\n", pcHostName, port ) );
        /* Attempt to create a mutually authenticated TLS connection. */
        xNetworkStatus = TLS_Socket_Connect( pxNetworkContext,
                                             pcHostName, port,
                                             pxNetworkCredentials,
                                             iotTRANSPORT_SEND_RECV_TIMEOUT_MS,
                                             iotTRANSPORT_SEND_RECV_TIMEOUT_MS );

        if( xNetworkStatus != eTLSTransportSuccess )
        {
            /* Generate a random number and calculate backoff value (in milliseconds) for
             * the next connection retry.
             * Note: It is recommended to seed the random number generator with a device-specific
             * entropy source so that possibility of multiple devices retrying failed network operations
             * at similar intervals can be avoided. */
            xBackoffAlgStatus = BackoffAlgorithm_GetNextBackoff( &xReconnectParams, configRAND32(), &usNextRetryBackOff );

            if( xBackoffAlgStatus == BackoffAlgorithmRetriesExhausted )
            {
                LogError( ( "Connection to the IoT Hub failed, all attempts exhausted." ) );
            }
            else if( xBackoffAlgStatus == BackoffAlgorithmSuccess )
            {
                LogWarn( ( "Connection to the IoT Hub failed [%d]. "
                           "Retrying connection with backoff and jitter [%d]ms.",
                           xNetworkStatus, usNextRetryBackOff ) );
                vTaskDelay( pdMS_TO_TICKS( usNextRetryBackOff ) );
            }
        }
    } while( ( xNetworkStatus != eTLSTransportSuccess ) && ( xBackoffAlgStatus == BackoffAlgorithmSuccess ) );

    return xNetworkStatus == eTLSTransportSuccess ? 0 : 1;
}
/*-----------------------------------------------------------*/

/**
 * @brief Initialize the system properties to default
 */
static void prvInitializeSystemProperties()
{
    xSystemProperties.telemetry_interval = 30;
    xSystemProperties.number_of_units = MAX_NUMBER_OF_UNITS;
    xSystemProperties.version = 1.0;
    strncpy(xSystemProperties.ccu.identity.serial_number, "NOT_AVAILABLE", sizeof("NOT_AVAILABLE"));
    strncpy(xSystemProperties.tank.identity.serial_number, "NOT_AVAILABLE", sizeof("NOT_AVAILABLE"));
    for(uint8_t i = 0; i < MAX_NUMBER_OF_UNITS; ++i)
    {
        xSystemProperties.units[i].number_of_cartridges = MAX_NUMBER_OF_CARTRIDGES;
        strncpy(xSystemProperties.units[i].identity.serial_number, "NOT_AVAILABLE", sizeof("NOT_AVAILABLE"));
        for(uint8_t j = 0; j < MAX_NUMBER_OF_UNITS; ++j)
        {
            xSystemProperties.units[i].cartridges[j].slot_number = j + 1;
            strncpy(xSystemProperties.units[i].cartridges[j].identity.serial_number, "NOT_AVAILABLE", sizeof("NOT_AVAILABLE"));
        }
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief IoT backend connection task.
 * 
 * Responsible for establishing and maintaining the connection
 * to Azure IoT Hub. It also handles reconnection logic in case
 * of connection loss.
 */
void vIoTBackendConnectionTask( void )
{
    prvInitializeSystemProperties();

    xTaskCreate( prvIoTBackendConnectionTask,       /* Function that implements the task. */
                 "IoT_backend_connection_task",     /* Text name for the task - only used for debugging. */
                 config_STACKSIZE,                  /* Size of stack (in words, not bytes) to allocate for the task. */
                 NULL,                              /* Task parameter - not used in this case. */
                 tskIDLE_PRIORITY,                  /* Task priority, must be between 0 and configMAX_PRIORITIES - 1. */
                 NULL );                            /* Used to pass out a handle to the created task - not used in this case. */
}
/*-----------------------------------------------------------*/

/**
 * @brief IoT telemetry task.
 * 
 * Sending telemetry to the backend every 30 seconds.
 * 
 * @todo Sends error message while trying to send telemtry
 *       Later, the idea is to get communicated by the
 *       controller_communication_task.
 */
void vIoTTelemetryTask( void )
{
    xTaskCreate( prvIoTTelemetryTask,               /* Function that implements the task. */
                 "IoT_telemetry_task",              /* Text name for the task - only used for debugging. */
                 config_STACKSIZE,                  /* Size of stack (in words, not bytes) to allocate for the task. */
                 NULL,                              /* Task parameter - not used in this case. */
                 tskIDLE_PRIORITY,                  /* Task priority, must be between 0 and configMAX_PRIORITIES - 1. */
                 NULL );                            /* Used to pass out a handle to the created task - not used in this case. */
}
/*-----------------------------------------------------------*/

/**
 * @brief IoT backend connection task.
 * 
 * Responsible for establishing and maintaining the connection
 * to Azure IoT Hub. It also handles reconnection logic in case
 * of connection loss.
 */
void vIoTCloudMessageTask( void )
{
    xTaskCreate( prvIoTCloudMessageTask,            /* Function that implements the task. */
                 "IoT_cloud_message_task",          /* Text name for the task - only used for debugging. */
                 1024U,                             /* Size of stack (in words, not bytes) to allocate for the task. */
                 NULL,                              /* Task parameter - not used in this case. */
                 tskIDLE_PRIORITY,                  /* Task priority, must be between 0 and configMAX_PRIORITIES - 1. */
                 NULL );                            /* Used to pass out a handle to the created task - not used in this case. */
}
/*-----------------------------------------------------------*/
