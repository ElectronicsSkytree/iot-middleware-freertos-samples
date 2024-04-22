/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/* Copyright (c) Skytree B.V. All rights reserved. */

/* Local includes */
#include "common_definition.h"
#include "process_cloud_msg.h"
#include "process_telemetry.h"
#include "config.h"
#include "controller_data.h"
#include "main.h"
#include "time_wrapper.h"
#include "connection.h"

/* Azure Provisioning/IoT Hub library includes */
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
*/
#include "assert_override.h"
/*-----------------------------------------------------------*/

/* Compile time error for undefined configs. */
#if !defined( configHOSTNAME ) && !defined( enableDPS )
    #error "Define the config configHOSTNAME by following the instructions in file config.h."
#endif

#if !defined( configENDPOINT ) && defined( enableDPS )
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
 * @brief communication task reconnects after this interval
 * 
 * @todo we are doing this as we have seen crashes at WiFi/TLS layer
*/
#define iotDELAY_RECONNECTION_AFTER     ( pdMS_TO_TICKS( 2400000U ) )

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

/**
 * @brief IoTHub client created on successful connection and then
 * used for sending/ receiving from IoTHub
 * 
 * @todo To review if we need protection (mutex)
*/
AzureIoTHubClient_t xAzureIoTHubClient;

/**
 * @brief System properties (CCU, Units and Tank) received
 *        as desired properties from IoTHub.
*/
SYSTEM_Properties_t xSystemProperties;

/**
 * @brief Static buffer used to hold MQTT messages being sent and received.
 */
static uint8_t ucMQTTMessageBuffer[ configNETWORK_BUFFER_SIZE ];
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
 * @brief Setup transport credentials.
 * 
 * @param[out] pxNetworkCredentials Pointer to network credentials struct.
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

    ulStatus = prvConnectToServerWithBackoffRetries( configENDPOINT, configIOTHUB_PORT,
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

    xResult = AzureIoTProvisioningClient_Init( &xAzureIoTProvisioningClient, ( const uint8_t * ) configENDPOINT,
                                               sizeof( configENDPOINT ) - 1, ( const uint8_t * ) configID_SCOPE,
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
 * 
 * @note These properties are expected to be populated through
 *       initial (desired) properties sync-up.
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

    configPRINTF( ( "---------IoT backend connection Task---------\r\n" ) );

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

    while( 1 )
    {
        configASSERT( IsConnectedToInternet() == true, false );
        configPRINTF( ( "(Re)starting to connect to the broker .\r\n" ) );

        /* Retry connection with a backoff */
        ulStatus = prvConnectToServerWithBackoffRetries( ( const char * ) pucIotHubHostname, configIOTHUB_PORT,
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
                                          &xHubOptions, ucMQTTMessageBuffer,
                                          sizeof( ucMQTTMessageBuffer ), GetUnixTime, &xTransport );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        /**
         * @todo rather have a macro if sysmmetric key way and get key through configuration
         */ 
        #ifdef deviceSYMMETRIC_KEY
            xResult = AzureIoTHubClient_SetSymmetricKey( &xAzureIoTHubClient, ( const uint8_t * ) deviceSYMMETRIC_KEY,
                                                         sizeof( deviceSYMMETRIC_KEY ) - 1, Crypto_HMAC );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        #endif

        /* Sends an MQTT Connect packet over the already established TLS connection,
         * and waits for connection acknowledgment (CONNACK) packet. */
        LogInfo( ( "Creating an MQTT connection to %s.\r\n", pucIotHubHostname ) );

        /* @todo We are setting clean session to false always, fine? */
        xResult = AzureIoTHubClient_Connect( &xAzureIoTHubClient, false, &xSessionPresent,
                                             iotCONNACK_RECV_TIMEOUT_MS );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        /**
         * Subscribe for the cloud communications
         * @note The process_loop is in cloud msg task, so everything is received
         *       in cloud msg task.
         */
        vSubscribeCloudStuff();

        // Set the CONNECTED_BIT so that other tasks continue their work
        xEventGroupSetBits(xConnectionEventGroup, CONNECTED_BIT);
        xEventGroupClearBits(xConnectionEventGroup, DISCONNECTED_BIT);
        xEventGroupClearBits(xConnectionEventGroup, PROPERTIES_RECEIVED_BIT);

        /**
         * Start sending telemetry messages and receiving cloud messages.
         * @todo Cloud message is handled in telemetry task currently. If needed
         *       separate cloud message task to be created.
         */
        configASSERT( pdTRUE == vCreateIoTTelemetryTask(), pdFALSE );

        /**
         * @todo The ethernet board might not have the intermittent connection loss issue,
         *       So maybe a good idea tonot do this?
        */
        LogInfo( ( "Connection task will reconnect after (%d minutes) .... \r\n\r\n", iotDELAY_RECONNECTION_AFTER / 60000 ) );
        vTaskDelay( iotDELAY_RECONNECTION_AFTER );

        // Destroy the telemetry task as it would be created again
        configASSERT( pdTRUE == vDeleteIoTTelemetryTask(), pdFALSE );

        /**
         * @todo With ethernet board if this works fine, we can remove this logic
         *       of reconnecting after iotDELAY_RECONNECTION_AFTER delay.
        */
        xEventGroupSetBits(xConnectionEventGroup, DISCONNECTED_BIT);
        xEventGroupClearBits(xConnectionEventGroup, CONNECTED_BIT);
        xEventGroupClearBits(xConnectionEventGroup, PROPERTIES_RECEIVED_BIT);

        if( IsConnectedToInternet() )
        {
            vUnsubscribeCloudStuff();
            xResult = AzureIoTHubClient_Disconnect( &xAzureIoTHubClient );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        }

        /* Close the network connection.  */
        TLS_Socket_Disconnect( &xNetworkContext );
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief IoT backend connection task.
 * 
 * Responsible for establishing and maintaining the connection
 * to Azure IoT Hub. It also handles reconnection logic.
 */
void vCreateIoTBackendConnectionTask( void )
{
    prvInitializeSystemProperties();

    xTaskCreate( prvIoTBackendConnectionTask,       /* Function that implements the task. */
                 "IoT_backend_connection_task",     /* Text name for the task - only used for debugging. */
                 1024U,                             /* Size of stack (in words, not bytes) to allocate for the task. */
                 NULL,                              /* Task parameter - not used in this case. */
                 tskIDLE_PRIORITY + 2,              /* Task priority, must be between 0 and configMAX_PRIORITIES - 1. */
                 NULL );                            /* Used to pass out a handle to the created task - not used in this case. */
}
/*-----------------------------------------------------------*/
