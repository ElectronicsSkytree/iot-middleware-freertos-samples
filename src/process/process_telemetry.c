/* Copyright (c) Skytree B.V. All rights reserved. */

/* Local includes */
#include "common_definition.h"
// #include "time_wrapper.h"
#include "properties.h"
#include "telemetry.h"

/* Special Includes */
/**
 * @note This should be included at the end to make sure the
 *       assert is overriden to do a system reset.
*/
#include "assert_override.h"
/*-----------------------------------------------------------*/

/**
 * @brief  The content type of the Telemetry message published.
 * @remark Message properties must be url-encoded.
 *         This message property is not required to send telemetry.
 */
#define iotMESSAGE_CONTENT_TYPE                    "application%2Fjson"

/**
 * @brief  The content encoding of the Telemetry message published.
 * @remark Message properties must be url-encoded.
 *         This message property is not required to send telemetry.
 */
#define iotMESSAGE_CONTENT_ENCODING                "utf-8"

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

/**
 * @brief Delay (in ticks) at start/ boot-up.
 * 
 * Note: We do this to allow CCU to send enough telemtry data. We deduct 10s as
 * the boot-up itself can take a bit of time.
 */
#define iotDELAY_AT_START                          ( iotDELAY_BETWEEN_PUBLISHES_TICKS - pdMS_TO_TICKS( 10000U ) )

/**
 * @brief Delay after sending error.
 */
#define iotDELAY_ON_ERROR                          ( pdMS_TO_TICKS( 5000U ) )
/*-----------------------------------------------------------*/

/**
 * @brief Scratch buffer for filling all telemetries
 * 
 * Keeping everything here as our payload is very constant
 * there is no real variation in length of payload. Also to
 * avoid fragmentations and unwanted issues in heap.
 * 
 * @todo Review if we want to use heap.
*/
#define SCRATCH_BUFFER_LENGTH 3200
#define PROPERTY_BUFFER_LENGTH 80
static uint8_t ucPropertyBuffer[ PROPERTY_BUFFER_LENGTH ];
static uint8_t ucScratchBuffer[ SCRATCH_BUFFER_LENGTH ];

/**
 * @brief For managing alerts
 * 
 * @todo Rework and remove these once new ccu
 *       communication is implemented.
*/
static sequence_state_t xSkidLastState;
static sequence_state_t xUnitLastState;

/**
 * @brief Telemetry task handle.
 * 
 * We are creating and deleting telemetry task on re-connection.
 * 
 * @todo review task management
*/
static TaskHandle_t xTelemetryTaskHandle = NULL;
/*-----------------------------------------------------------*/

/**
 * @brief IoT telemetry task.
 * 
 * IoT telemetry is sent every 30 seconds.
 * 
 * @todo Sends error message while trying to send telemtry
 *       Later, the idea is to get communicated by the
 *       controller_communication_task.
 * 
 * @param[in] pvParameters pvParameters, unused now
 */
static void prvIoTTelemetryTask( void * pvParameters )
{
    ( void ) pvParameters;
    AzureIoTMessageProperties_t xPropertyBag;
    AzureIoTResult_t xResult;
    uint32_t ulScratchBufferLength = 0U;
    // EventBits_t uxBitsToWaitFor = PROPERTIES_RECEIVED_BIT | CONNECTED_BIT;

    configPRINTF( ( "---------IoT telemetry Task---------\r\n" ) );

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
     * @todo Rework as part of ccu-iot new structure
    */
    while( 1 ) {
        // Wait for the connection
        EventBits_t bits = xEventGroupWaitBits(xConnectionEventGroup, CONNECTED_BIT, pdFALSE, pdFALSE, 3000);

        if( bits & CONNECTED_BIT )
        {
            memset(ucScratchBuffer, '\0', sizeof (ucScratchBuffer) );
            // Create the json boot-up msg payload
            configPRINTF( ("prvIoTTelemetryTask create boot-up payload\r\n") );
            ulScratchBufferLength = CreateBootUpMessage( ucScratchBuffer, sizeof( ucScratchBuffer ) );
            LogInfo( ( "boot-up msg: ucScratchBuffer = %s and ulScratchBufferLength =%d\r\n", ucScratchBuffer, ulScratchBufferLength ) );

            xResult = AzureIoTHubClient_SendTelemetry( &xAzureIoTHubClient,
                                                        ucScratchBuffer, ulScratchBufferLength,
                                                        &xPropertyBag, eAzureIoTHubMessageQoS1, NULL );
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            // Not to be sent again, so break
            break;
        }
    }

    /* Idle for some time so that telemetry is populated first time upon boot. */
    LogInfo( ( "On boot-up: Keeping Connection Idle for %d seconds...\r\n\r\n", iotDELAY_AT_START / 1000 ) );
    vTaskDelay( iotDELAY_AT_START );

    while( 1 )
    {
        // Wait until the connection is (re)established
        while (1) {
            // Wait for the connection event to be set
            EventBits_t bits = xEventGroupWaitBits(xConnectionEventGroup, CONNECTED_BIT, pdFALSE, pdFALSE, 3000);

            // Check if the CONNECTED_BIT is set
            if( bits & CONNECTED_BIT )
            {
                break;
            }
        }

        /**
         * @todo ccu communication with new structure
        */
        // Read CCU sensor data
        SKID_iot_status_t skid_data = get_skid_status(xSkidLastState);

        // Read Units sensor data
        UNIT_iot_status_t unit_data = get_unit_status(xUnitLastState);

        /**
         * @todo Handle error handling this way till we jump to new communication code 
        */
        if( (skid_data.skid_state == Safe_State && skid_data.send_alert) ||
            (unit_data.unit_state == Safe_State && unit_data.send_alert) )
        {
            skid_data.previous_skid_state = xSkidLastState;
            unit_data.previous_unit_state = xUnitLastState;

            memset(ucScratchBuffer, '\0', sizeof (ucScratchBuffer) );
            // Create the json error msg payload
            ulScratchBufferLength = CreateErrorMessage( skid_data, unit_data, ucScratchBuffer, sizeof( ucScratchBuffer ) );
            LogInfo( ( "error msg: ucScratchBuffer = %s and ulScratchBufferLength =%d\r\n", ucScratchBuffer, ulScratchBufferLength ) );

            xResult = AzureIoTHubClient_SendTelemetry( &xAzureIoTHubClient, ucScratchBuffer, ulScratchBufferLength,
                                                       &xPropertyBag, eAzureIoTHubMessageQoS1, NULL );
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            /* Idle for some time so that telemetry is populated first time upon boot. */
            LogInfo( ( "On error: Keeping Connection Idle for %d seconds...\r\n\r\n", iotDELAY_ON_ERROR / 1000 ) );
            vTaskDelay( iotDELAY_ON_ERROR );

            xSkidLastState = skid_data.skid_state;
            xUnitLastState = unit_data.unit_state;

            // Continue so that we check for the CONNECTED_BIT again
            continue;
        }
        else
        {
            xSkidLastState = skid_data.skid_state;
            xUnitLastState = unit_data.unit_state;
        }

        /**
         * @todo doing the process loop in telemetry task till we figure out the task management issue
        */
        {
            LogInfo( ( "Attempt to receive publish message from IoT Hub.\r\n" ) );
            xResult = AzureIoTHubClient_ProcessLoop( &xAzureIoTHubClient, iotPROCESS_LOOP_TIMEOUT_MS );
            EventBits_t bits = xEventGroupGetBits(xConnectionEventGroup);
            if( xResult != eAzureIoTSuccess && bits & DISCONNECTED_BIT )
            {
                LogWarn( ( "Process loop failed, do nothing as already in disconnected state\r\n" ) );
            }
            else if( xResult != eAzureIoTSuccess )
            {
                configASSERT( xResult == eAzureIoTSuccess, xResult );
            }
        }

        /**
         * Create the json telemetry payload
         * @todo We are sending single unit data, ok for now till we re-work on this
        */
        memset(ucScratchBuffer, '\0', sizeof (ucScratchBuffer) );
        ulScratchBufferLength = CreateTelemetry( skid_data, unit_data, ucScratchBuffer, sizeof( ucScratchBuffer ) );
        LogInfo( ( "ucScratchBuffer = %s and ulScratchBufferLength =%d\r\n", ucScratchBuffer, ulScratchBufferLength ) );

        xResult = AzureIoTHubClient_SendTelemetry( &xAzureIoTHubClient, ucScratchBuffer, ulScratchBufferLength,
                                                   &xPropertyBag, eAzureIoTHubMessageQoS1, NULL );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        /**
         * @todo Leave Connection Idle for some time. Check if we want to have a timer.
        */
        LogInfo( ( "Keeping Connection Idle for %d seconds...\r\n\r\n", iotDELAY_BETWEEN_PUBLISHES_TICKS / 1000 ) );
        vTaskDelay( iotDELAY_BETWEEN_PUBLISHES_TICKS );
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Create IoT telemetry task.
 * 
 * Sending telemetry to the backend every 30 seconds.
 * 
 * @return pdTRUE if telemetry task created successfully and
 *         pdFALSE if failed to create the telemetry task.
 */
BaseType_t vCreateIoTTelemetryTask( void )
{
    return xTaskCreate( prvIoTTelemetryTask,               /* Function that implements the task. */
                        "IoT_telemetry_task",              /* Text name for the task - only used for debugging. */
                        2048U,                             /* Size of stack (in words, not bytes) to allocate for the task. */
                        NULL,                              /* Task parameter - not used in this case. */
                        tskIDLE_PRIORITY + 3,              /* Task priority, must be between 0 and configMAX_PRIORITIES - 1. */
                        &xTelemetryTaskHandle );           /* Task handle. */
}
/*-----------------------------------------------------------*/

/**
 * @brief Delete IoT telemetry task.
 * 
 * @return pdTRUE if telemetry task created successfully and
 *         pdFALSE if failed to create the telemetry task.
 */
BaseType_t vDeleteIoTTelemetryTask( void )
{
    if(NULL != xTelemetryTaskHandle)
    {
        vTaskDelete(xTelemetryTaskHandle);
        return pdTRUE;
    }

    return pdFALSE;
}
/*-----------------------------------------------------------*/
