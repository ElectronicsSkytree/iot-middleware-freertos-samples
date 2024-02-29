/* Copyright (c) Skytree B.V. All rights reserved. */

/* Local includes */
#include "config.h"
#include "errors.h"
#include "time_wrapper.h"
#include "telemetry.h"
#include "properties.h"

/* FreeRTOS includes. */
#include "logging_levels.h"

/* Azure JSON includes */
#include "azure_iot_json_writer.h"

/* Special Includes */
/**
 * @note This should be included at the end to make sure the
 *       assert is overriden to let MCU reset.
 * 
 * @todo To improve this later instead of forced reset.
*/
#include "assert_override.h"

/**
 * @brief Telemetry Objects and Values
 */
// Primary objects
#define sampleazureiotTELEMETRY_OBJECT_CCU                        ( "ccu" )
#define sampleazureiotTELEMETRY_OBJECT_UNITS                      ( "units" )
#define sampleazureiotTELEMETRY_OBJECT_TANK                       ( "tank" )

// Secondary
#define sampleazureiotTELEMETRY_OBJECT_SENSOR_MEASUREMENTS        ( "sensor_measurements" )

// Sensors
#define sampleazureiotTELEMETRY_O2                                ( "o2" )
#define sampleazureiotTELEMETRY_MASSFLOW                          ( "mass_flow" )
#define sampleazureiotTELEMETRY_CO2                               ( "co2" )
#define sampleazureiotTELEMETRY_TANK_PRESSURE                     ( "tank_pressure" )
#define sampleazureiotTELEMETRY_TEMPERATURE                       ( "temperature" )
#define sampleazureiotTELEMETRY_HUMIDITY                          ( "humidity" )
#define sampleazureiotTELEMETRY_VACUUM_SENSOR                     ( "vacuum_sensor" )
#define sampleazureiotTELEMETRY_AMBIENT_HUMIDITY                  ( "ambient_humidity" )
#define sampleazureiotTELEMETRY_AMBIENT_TEMPERATURE               ( "ambient_temperature" )
#define sampleazureiotTELEMETRY_PROPOTIONAL_VALVE_SENSOR          ( "propotional_valve_pressure" )

// Heaters and catridge objects
#define sampleazureiotTELEMETRY_OBJECT_CARTRIDGES                 ( "cartridges" )
#define sampleazureiotTELEMETRY_OBJECT_ZONES                      ( "zones" )

// Status related
#define sampleazureiotTELEMETRY_OBJECT_COMPONENT_STATUS           ( "component_status" )
#define sampleazureiotTELEMETRY_OBJECT_CCU_STATUS                 ( "ccu_status" )
#define sampleazureiotTELEMETRY_OBJECT_UNIT_STATUS                ( "unit_status" )

// Telemetry
#define sampleazureiotTELEMETRY_MEASUREMENT_COUNT                 ( "measurement_count" )
#define sampleazureiotMESSAGE_VERSION                             ( "version" )
#define sampleazureiot_TIMESTAMP_UTC                              ( "timestamp_utc" )
#define sampleazureiotMESSAGE_TYPE                                ( "message_type" )
#define sampleazureiotTELEMETRY_SERIAL_NUMBER                     ( "serial_number" )
#define sampleazureiotTELEMETRY_SLOT                              ( "slot" )
#define sampleazureiotTELEMETRY_CCU_STATE                         ( "ccu_state" )
#define sampleazureiotTELEMETRY_UNIT_STATE                        ( "unit_state" )
#define sampleazureiotTELEMETRY_SENSOR_NAME                       ( "name" )
#define sampleazureiotTELEMETRY_ZONE                              ( "zone" )
#define sampleazureiotTELEMETRY_STATUS                            ( "status" )
#define sampleazureiotTELEMETRY_AVG                               ( "avg" )
#define sampleazureiotTELEMETRY_MAX                               ( "max" )
#define sampleazureiotTELEMETRY_MIN                               ( "min" )
#define sampleazureiotTELEMETRY_MEDIAN                            ( "median" )

// Error msg
#define sampleazureiotTELEMETRY_CURRENT_STATE                     ( "current_state" )
#define sampleazureiotTELEMETRY_PREVIOUS_STATE                    ( "previous_state" )
#define sampleazureiotTELEMETRY_ERROR_CODE                        ( "error_code" )
#define sampleazureiotTELEMETRY_LOCATION                          ( "location" )
#define sampleazureiotTELEMETRY_OBJECT_ERRORS                     ( "errors" )
#define sampleazureiotTELEMETRY_OBJECT_BODY                       ( "body" )

// components status
#define sampleazureiotTELEMETRY_TWO_WAY_GAS_VALVE_BEFORE_WATER_TRAP             ( "two_way_gas_valve_before_water_trap" )
#define sampleazureiotTELEMETRY_TWO_WAY_GAS_VALVE_IN_WATER_TRAP                 ( "two_way_gas_valve_in_water_trap" )
#define sampleazureiotTELEMETRY_TWO_WAY_GAS_VALVE_AFTER_WATER_TRAP              ( "two_way_gas_valve_after_water_trap" )
#define sampleazureiotTELEMETRY_VACUUM_RELEASE_VALVE_IN_WATER_TRAP              ( "vacuum_release_valve_in_water_trap" )
#define sampleazureiotTELEMETRY_THREE_WAY_VACUUM_RELEASE_VALVE_BEFORE_CONDENSER ( "three_way_vacuum_release_valve_before_condenser" )
#define sampleazureiotTELEMETRY_THREE_WAY_VALVE_AFTER_VACUUM_PUMP               ( "three_way_valve_after_vacuum_pump" )
#define sampleazureiotTELEMETRY_COMPRESSOR                                      ( "compressor" )
#define sampleazureiotTELEMETRY_VACUUM_PUMP                                     ( "vacuum_pump" )
#define sampleazureiotTELEMETRY_CONDENSER                                       ( "condenser" )
// Skid/Unit status
#define sampleazureiotTELEMETRY_ERROR_FLAG                                      ( "error_flag" )
#define sampleazureiotTELEMETRY_HALT_FLAG                                       ( "halt_flag" )
#define sampleazureiotTELEMETRY_RESET_FLAG                                      ( "reset_flag" )
#define sampleazureiotTELEMETRY_JUST_STARTED_FLAG                               ( "just_started_flag" )
#define sampleazureiotTELEMETRY_SETUP_STATE_SYNCHING_FLAG                       ( "setup_state_synching_flag" )
// Valve status
#define sampleazureiotTELEMETRY_FAN_STATUS                                      ( "fan_status" )
#define sampleazureiotTELEMETRY_BUTTERFLY_VALVE_1_STATUS                        ( "butterfly_valve_1_status" )
#define sampleazureiotTELEMETRY_BUTTERFLY_VALVE_2_STATUS                        ( "butterfly_valve_2_status" )

// versions
#define sampleazureiot_AZURE_SDK_VERSION                                        ( "azure_sdk_version" )
#define sampleazureiot_FIRMWARE_VERSIONS                                        ( "firmware_versions" )
#define sampleazureiot_HARDWARE_VERSIONS                                        ( "hardware_versions" )
#define sampleazureiot_IDENTIFIER                                               ( "identifier" )

#define properties_NOT_AVAILABLE            "NOT_AVAILABLE"
// constant Values: telemetry
#define telemetry_MESSAGE_VERSION           "1.0"
#define telemetry_MEASUREMENT_COUNT         30              // Constant for now
#define telemetry_MESSAGE_TYPE              "telemetry"
// constant values: boot-up
#define bootup_MESSAGE_VERSION              "1.0"
#define bootup_MESSAGE_TYPE                 "boot-up"
// constant values: error
#define error_MESSAGE_VERSION               "1.0"
#define error_MESSAGE_TYPE                  "error"
#define azure_sdk_version                   "0.0.0"
#define ccu_IDENTIFIER                      "CCU"
#define unit1_IDENTIFIER                    "UNIT1"
#define ccu_FIRMWARE_VERSION                "0.0.0"
#define unit1_FIRMWARE_VERSION              "0.0.0"
#define ccu_LOCATION                        "CCU"
#define unit1_LOCATION                      "UNIT1"

// For getting the length of the telemetry names
#define lengthof( x )                  ( sizeof( x ) - 1 )

#define SCRATCH_BUFFER_LENGTH 3200
#define MINI_SCRATCH_BUFFER_LENGTH 2304

// Scratch buffers, improve these later
static uint8_t ucScratchTempBuffer[ SCRATCH_BUFFER_LENGTH ];
static uint8_t ucScratchTempHalfBuffer[ MINI_SCRATCH_BUFFER_LENGTH ];
static uint8_t ucScratchTempHalfBuffer2[ MINI_SCRATCH_BUFFER_LENGTH ];

extern SYSTEM_Properties_t xSystemProperties;

uint32_t prvCreateSkidComponentStatusTelemetry( SKID_iot_status_t skid_data, uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_TWO_WAY_GAS_VALVE_BEFORE_WATER_TRAP, lengthof( sampleazureiotTELEMETRY_TWO_WAY_GAS_VALVE_BEFORE_WATER_TRAP ),
                                                                ( uint8_t * )valve_status_stringified[skid_data.two_way_gas_valve_before_water_trap], strlen(valve_status_stringified[skid_data.two_way_gas_valve_before_water_trap]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_TWO_WAY_GAS_VALVE_IN_WATER_TRAP, lengthof( sampleazureiotTELEMETRY_TWO_WAY_GAS_VALVE_IN_WATER_TRAP ),
                                                                ( uint8_t * )valve_status_stringified[skid_data.two_way_gas_valve_in_water_trap], strlen(valve_status_stringified[skid_data.two_way_gas_valve_in_water_trap]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_TWO_WAY_GAS_VALVE_AFTER_WATER_TRAP, lengthof( sampleazureiotTELEMETRY_TWO_WAY_GAS_VALVE_AFTER_WATER_TRAP ),
                                                                ( uint8_t * )valve_status_stringified[skid_data.two_way_gas_valve_after_water_trap], strlen(valve_status_stringified[skid_data.two_way_gas_valve_after_water_trap]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_VACUUM_RELEASE_VALVE_IN_WATER_TRAP, lengthof( sampleazureiotTELEMETRY_VACUUM_RELEASE_VALVE_IN_WATER_TRAP ),
                                                                ( uint8_t * )valve_status_stringified[skid_data.vacuum_release_valve_in_water_trap], strlen(valve_status_stringified[skid_data.vacuum_release_valve_in_water_trap]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_THREE_WAY_VACUUM_RELEASE_VALVE_BEFORE_CONDENSER, lengthof( sampleazureiotTELEMETRY_THREE_WAY_VACUUM_RELEASE_VALVE_BEFORE_CONDENSER ),
                                                                ( uint8_t * )three_way_valve_stringified[skid_data.three_way_vacuum_release_valve_before_condenser],
                                                                strlen(three_way_valve_stringified[skid_data.three_way_vacuum_release_valve_before_condenser]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_THREE_WAY_VALVE_AFTER_VACUUM_PUMP, lengthof( sampleazureiotTELEMETRY_THREE_WAY_VALVE_AFTER_VACUUM_PUMP ),
                                                                ( uint8_t * )three_way_valve_stringified[skid_data.three_valve_after_vacuum_pump],
                                                                strlen(three_way_valve_stringified[skid_data.three_valve_after_vacuum_pump]));
    configASSERT( xResult == eAzureIoTSuccess, xResult )

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_COMPRESSOR, lengthof( sampleazureiotTELEMETRY_COMPRESSOR ),
                                                                ( uint8_t * )component_status_stringified[skid_data.compressor], strlen(component_status_stringified[skid_data.compressor]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_VACUUM_PUMP, lengthof( sampleazureiotTELEMETRY_VACUUM_PUMP ),
                                                                ( uint8_t * )component_status_stringified[skid_data.vacuum_pump], strlen(component_status_stringified[skid_data.vacuum_pump]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_CONDENSER, lengthof( sampleazureiotTELEMETRY_CONDENSER ),
                                                                ( uint8_t * )component_status_stringified[skid_data.condenser], strlen(component_status_stringified[skid_data.condenser]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // End top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Skid component status data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

uint32_t prvCreateUnitComponentStatusTelemetry( UNIT_iot_status_t unit_data, uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_FAN_STATUS, lengthof( sampleazureiotTELEMETRY_FAN_STATUS ),
                                                                ( uint8_t * )component_status_stringified[unit_data.fan_status], strlen(component_status_stringified[unit_data.fan_status]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_BUTTERFLY_VALVE_1_STATUS, lengthof( sampleazureiotTELEMETRY_BUTTERFLY_VALVE_1_STATUS ),
                                                                ( uint8_t * )valve_status_stringified[unit_data.butterfly_valve_1_status], strlen(valve_status_stringified[unit_data.butterfly_valve_1_status]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_BUTTERFLY_VALVE_2_STATUS, lengthof( sampleazureiotTELEMETRY_BUTTERFLY_VALVE_2_STATUS ),
                                                                ( uint8_t * )valve_status_stringified[unit_data.butterfly_valve_2_status], strlen(valve_status_stringified[unit_data.butterfly_valve_2_status]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // End top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Skid component status data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

uint32_t prvCreateSkidStatusTelemetry( SKID_iot_status_t skid_data, uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_ERROR_FLAG, lengthof( sampleazureiotTELEMETRY_ERROR_FLAG ),
                                                                ( uint8_t * )flag_state_stringified[skid_data.error_flag], strlen(flag_state_stringified[skid_data.error_flag]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_HALT_FLAG, lengthof( sampleazureiotTELEMETRY_HALT_FLAG ),
                                                                ( uint8_t * )flag_state_stringified[skid_data.halt_flag], strlen(flag_state_stringified[skid_data.halt_flag]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_RESET_FLAG, lengthof( sampleazureiotTELEMETRY_RESET_FLAG ),
                                                                ( uint8_t * )flag_state_stringified[skid_data.reset_flag], strlen(flag_state_stringified[skid_data.reset_flag]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // End top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Skid status data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

uint32_t prvCreateUnitStatusTelemetry( UNIT_iot_status_t unit_data, uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_ERROR_FLAG, lengthof( sampleazureiotTELEMETRY_ERROR_FLAG ),
                                                                ( uint8_t * )flag_state_stringified[unit_data.error_flag], strlen(flag_state_stringified[unit_data.error_flag]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_HALT_FLAG, lengthof( sampleazureiotTELEMETRY_HALT_FLAG ),
                                                                ( uint8_t * )flag_state_stringified[unit_data.halt_flag], strlen(flag_state_stringified[unit_data.halt_flag]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_RESET_FLAG, lengthof( sampleazureiotTELEMETRY_RESET_FLAG ),
                                                                ( uint8_t * )flag_state_stringified[unit_data.reset_flag], strlen(flag_state_stringified[unit_data.reset_flag]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_JUST_STARTED_FLAG, lengthof( sampleazureiotTELEMETRY_JUST_STARTED_FLAG ),
                                                                ( uint8_t * )flag_state_stringified[unit_data.just_started_flag], strlen(flag_state_stringified[unit_data.just_started_flag]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_SETUP_STATE_SYNCHING_FLAG, lengthof( sampleazureiotTELEMETRY_SETUP_STATE_SYNCHING_FLAG ),
                                                                ( uint8_t * )flag_state_stringified[unit_data.setup_state_synching_flag], strlen(flag_state_stringified[unit_data.setup_state_synching_flag]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // End top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Unit status data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

uint32_t prvCreateSkidSensorTelemetry( sensor_name_t sensor_name, SKID_iot_status_t skid_data, uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    double sensor_avg = 0.0;
    double sensor_max = 0.0;
    double sensor_min = 0.0;
    double sensor_median = 0.0;
    char name[30] = {0};
    component_status_t status;

    // Get the desired sensor avg,max,min
    switch(sensor_name){
        // Skid sensors
        case SKID_O2:
            sensor_avg = skid_data.o2_sensor.stats.avg;
            sensor_max = skid_data.o2_sensor.stats.max;
            sensor_min = skid_data.o2_sensor.stats.min;
            sensor_median = skid_data.o2_sensor.stats.median;
            status = skid_data.o2_sensor.status;
            strncpy(name, sampleazureiotTELEMETRY_O2, lengthof(sampleazureiotTELEMETRY_O2));
        break;
        case SKID_MASS_FLOW:
            sensor_avg = skid_data.mass_flow.stats.avg;
            sensor_max = skid_data.mass_flow.stats.max;
            sensor_min = skid_data.mass_flow.stats.min;
            sensor_median = skid_data.mass_flow.stats.median;
            status = skid_data.mass_flow.status;
            strncpy(name, sampleazureiotTELEMETRY_MASSFLOW, lengthof(sampleazureiotTELEMETRY_MASSFLOW));
        break;
        case SKID_CO2:
            sensor_avg = skid_data.co2_sensor.stats.avg;
            sensor_max = skid_data.co2_sensor.stats.max;
            sensor_min = skid_data.co2_sensor.stats.min;
            sensor_median = skid_data.co2_sensor.stats.median;
            status = skid_data.co2_sensor.status;
            strncpy(name, sampleazureiotTELEMETRY_CO2, lengthof(sampleazureiotTELEMETRY_CO2));
        break;
        case SKID_PROPOTIONAL_VALVE_SENSOR:
            sensor_avg = skid_data.proportional_valve_pressure.stats.avg;
            sensor_max = skid_data.proportional_valve_pressure.stats.max;
            sensor_min = skid_data.proportional_valve_pressure.stats.min;
            sensor_median = skid_data.proportional_valve_pressure.stats.median;
            status = skid_data.proportional_valve_pressure.status;
            strncpy(name, sampleazureiotTELEMETRY_PROPOTIONAL_VALVE_SENSOR, lengthof(sampleazureiotTELEMETRY_PROPOTIONAL_VALVE_SENSOR));
        break;
        case SKID_TEMPERATURE:
            sensor_avg = skid_data.temperature.stats.avg;
            sensor_max = skid_data.temperature.stats.max;
            sensor_min = skid_data.temperature.stats.min;
            sensor_median = skid_data.temperature.stats.median;
            status = skid_data.temperature.status;
            strncpy(name, sampleazureiotTELEMETRY_TEMPERATURE, lengthof(sampleazureiotTELEMETRY_TEMPERATURE));
        break;
        case SKID_HUMIDITY:
            sensor_avg = skid_data.humidity.stats.avg;
            sensor_max = skid_data.humidity.stats.max;
            sensor_min = skid_data.humidity.stats.min;
            sensor_median = skid_data.humidity.stats.median;
            status = skid_data.humidity.status;
            strncpy(name, sampleazureiotTELEMETRY_HUMIDITY, lengthof(sampleazureiotTELEMETRY_HUMIDITY));
        break;
        // @todo later move tank pressure out of skid list
        case TANK_PRESSURE:
            sensor_avg = skid_data.tank_pressure.stats.avg;
            sensor_max = skid_data.tank_pressure.stats.max;
            sensor_min = skid_data.tank_pressure.stats.min;
            sensor_median = skid_data.tank_pressure.stats.median;
            status = skid_data.tank_pressure.status;
            strncpy(name, sampleazureiotTELEMETRY_TANK_PRESSURE, lengthof(sampleazureiotTELEMETRY_TANK_PRESSURE));
        break;
        default:
            LogError( ( "Invalid skid sensor selected" ) );
            return 0;
        break;
    }

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_SENSOR_NAME, lengthof( sampleazureiotTELEMETRY_SENSOR_NAME ), ( uint8_t * ) name, strlen(name));
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_STATUS, lengthof( sampleazureiotTELEMETRY_STATUS ),
                                                                ( uint8_t * )sensor_status_stringified[status], strlen(sensor_status_stringified[status]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_AVG, lengthof( sampleazureiotTELEMETRY_AVG ), sensor_avg, 3);
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_MAX, lengthof( sampleazureiotTELEMETRY_MAX ), sensor_max, 3);
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_MIN, lengthof( sampleazureiotTELEMETRY_MIN ), sensor_min, 3);
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_MEDIAN, lengthof( sampleazureiotTELEMETRY_MEDIAN ), sensor_median, 3);
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // End of top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Skid sensor data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

uint32_t prvCreateUnitSensorTelemetry( sensor_name_t sensor_name, UNIT_iot_status_t unit_data, uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    double sensor_avg = 0.0;
    double sensor_max = 0.0;
    double sensor_min = 0.0;
    double sensor_median = 0.0;
    char name[25] = {0};
    component_status_t status;

    // Get the desired sensor avg,max,min
    switch(sensor_name){
        // Skid sensors
        case UNIT_VACUUM_SENSOR:
            sensor_avg = unit_data.vacuum_sensor.stats.avg;
            sensor_max = unit_data.vacuum_sensor.stats.max;
            sensor_min = unit_data.vacuum_sensor.stats.min;
            sensor_median = unit_data.vacuum_sensor.stats.median;
            status = unit_data.vacuum_sensor.status;
            strncpy(name, sampleazureiotTELEMETRY_VACUUM_SENSOR, lengthof(sampleazureiotTELEMETRY_VACUUM_SENSOR));
        break;
        case UNIT_AMBIENT_HUMIDITY:
            sensor_avg = unit_data.ambient_humidity.stats.avg;
            sensor_max = unit_data.ambient_humidity.stats.max;
            sensor_min = unit_data.ambient_humidity.stats.min;
            sensor_median = unit_data.ambient_humidity.stats.median;
            status = unit_data.ambient_humidity.status;
            strncpy(name, sampleazureiotTELEMETRY_AMBIENT_HUMIDITY, lengthof(sampleazureiotTELEMETRY_AMBIENT_HUMIDITY));
        break;
        case UNIT_AMBIENT_TEMPERATURE:
            sensor_avg = unit_data.ambient_temperature.stats.avg;
            sensor_max = unit_data.ambient_temperature.stats.max;
            sensor_min = unit_data.ambient_temperature.stats.min;
            sensor_median = unit_data.ambient_humidity.stats.median;
            status = unit_data.ambient_humidity.status;
            strncpy(name, sampleazureiotTELEMETRY_AMBIENT_TEMPERATURE, lengthof(sampleazureiotTELEMETRY_AMBIENT_TEMPERATURE));
        break;
        default:
            LogError( ( "Invalid unit sensor selected" ) );
            return 0;
        break;
    }

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_SENSOR_NAME, lengthof( sampleazureiotTELEMETRY_SENSOR_NAME ), ( uint8_t * ) name, strlen(name));
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_STATUS, lengthof( sampleazureiotTELEMETRY_STATUS ),
                                                                ( uint8_t * )sensor_status_stringified[status], strlen(sensor_status_stringified[status]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_AVG, lengthof( sampleazureiotTELEMETRY_AVG ), sensor_avg, 3);
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_MAX, lengthof( sampleazureiotTELEMETRY_MAX ), sensor_max, 3);
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_MIN, lengthof( sampleazureiotTELEMETRY_MIN ), sensor_min, 3);
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_MEDIAN, lengthof( sampleazureiotTELEMETRY_MEDIAN ), sensor_median, 3);
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // End of top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Unit sensor data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

uint32_t prvCreateUnitHeatersTelemetry( uint8_t index, UNIT_iot_status_t unit_data, uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    double sensor_avg = unit_data.heater_info[index].stats.avg;
    double sensor_max = unit_data.heater_info[index].stats.max;
    double sensor_min = unit_data.heater_info[index].stats.avg;
    double sensor_median = unit_data.heater_info[index].stats.median;
    component_status_t status = unit_data.heater_info[index].status;

    xResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_ZONE, lengthof( sampleazureiotTELEMETRY_ZONE ), (index % NUMBER_OF_CARTRIDGES) + 1 );
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_STATUS, lengthof( sampleazureiotTELEMETRY_STATUS ),
                                                                ( uint8_t * )component_status_stringified[status], strlen(component_status_stringified[status]));
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_AVG, lengthof( sampleazureiotTELEMETRY_AVG ), sensor_avg, 3);
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_MAX, lengthof( sampleazureiotTELEMETRY_MAX ), sensor_max, 3);
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_MIN, lengthof( sampleazureiotTELEMETRY_MIN ), sensor_min, 3);
    configASSERT( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_MEDIAN, lengthof( sampleazureiotTELEMETRY_MEDIAN ), sensor_median, 3);
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // End of top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Units sensor data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

uint32_t prvCreateSkidTelemetry( SKID_iot_status_t skid_data, uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // Other non-nested stuff
    {
        xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_SERIAL_NUMBER, lengthof( sampleazureiotTELEMETRY_SERIAL_NUMBER ),
                                                                    ( uint8_t * )xSystemProperties.ccu.identity.serial_number, strlen(xSystemProperties.ccu.identity.serial_number));
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_CCU_STATE, lengthof( sampleazureiotTELEMETRY_CCU_STATE ),
                                                                    ( uint8_t * )sequence_state_stringified[skid_data.skid_state], strlen(sequence_state_stringified[skid_data.skid_state]));
        configASSERT( xResult == eAzureIoTSuccess, xResult );
    }

    // sensor measurements
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_SENSOR_MEASUREMENTS, lengthof( sampleazureiotTELEMETRY_OBJECT_SENSOR_MEASUREMENTS ) );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        xResult = AzureIoTJSONWriter_AppendBeginArray( &xWriter );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        // o2
        {
            // Sensor: name, status, avg, max, min, median
            memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
            lBytesWritten = prvCreateSkidSensorTelemetry( SKID_O2, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

            xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        }

        // mass flow
        {
            // Sensor: name, status, avg, max, min, median
            memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
            lBytesWritten = prvCreateSkidSensorTelemetry( SKID_MASS_FLOW, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

            xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        }

        // co2
        {
            // Sensor: name, status, avg, max, min, median
            memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
            lBytesWritten = prvCreateSkidSensorTelemetry( SKID_CO2, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

            xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        }

        // proptional valve sensor
        {
            // Sensor: name, status, avg, max, min, median
            memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
            lBytesWritten = prvCreateSkidSensorTelemetry( SKID_PROPOTIONAL_VALVE_SENSOR, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

            xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        }

        // temperature
        {
            // Sensor: name, status, avg, max, min, median
            memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
            lBytesWritten = prvCreateSkidSensorTelemetry( SKID_TEMPERATURE, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

            xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        }

        // humidity
        {
            // Sensor: name, status, avg, max, min, median
            memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
            lBytesWritten = prvCreateSkidSensorTelemetry( SKID_HUMIDITY, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

            xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        }
        
        xResult = AzureIoTJSONWriter_AppendEndArray( &xWriter );
        configASSERT( xResult == eAzureIoTSuccess, xResult );
    }

    // Skid status
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_CCU_STATUS, lengthof( sampleazureiotTELEMETRY_OBJECT_CCU_STATUS ) );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
        lBytesWritten = prvCreateSkidStatusTelemetry( skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess, xResult );
    }

    // Skid component status
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_COMPONENT_STATUS, lengthof( sampleazureiotTELEMETRY_OBJECT_COMPONENT_STATUS ) );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
        lBytesWritten = prvCreateSkidComponentStatusTelemetry( skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess, xResult );
    }

    // End of top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Skid data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

uint32_t prvCreateUnit1Telemetry( UNIT_iot_status_t unit_data, uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // Other non-nested stuff
    {
        xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_SERIAL_NUMBER, lengthof( sampleazureiotTELEMETRY_SERIAL_NUMBER ),
                                                                    ( uint8_t * )xSystemProperties.units[0].identity.serial_number, strlen(xSystemProperties.units[0].identity.serial_number));
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_UNIT_STATE, lengthof( sampleazureiotTELEMETRY_UNIT_STATE ),
                                                                    ( uint8_t * )sequence_state_stringified[unit_data.unit_state], strlen(sequence_state_stringified[unit_data.unit_state]));
        configASSERT( xResult == eAzureIoTSuccess, xResult );
    }

    // cartridges
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_CARTRIDGES, lengthof( sampleazureiotTELEMETRY_OBJECT_CARTRIDGES ) );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        // Begin of cartridges array
        xResult = AzureIoTJSONWriter_AppendBeginArray( &xWriter );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        // Begin cartridges loop
        // @todo: For now, we are filling by reading sequentially from 9 heaters
        //        to make it 3 cartridges with 3 zones each per cartridge.
        for(uint8_t cart = 0;cart<NUMBER_OF_CARTRIDGES;++cart){
            xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( 
                &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_SERIAL_NUMBER, lengthof( sampleazureiotTELEMETRY_SERIAL_NUMBER ),
                ( uint8_t * )xSystemProperties.units[0].cartridges[cart].identity.serial_number,
                strlen(xSystemProperties.units[0].cartridges[cart].identity.serial_number));
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            // @todo we need to get slot number also from desired properties?
            //       For now using sequential numbers 1, 2, 3...
            xResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_SLOT, lengthof( sampleazureiotTELEMETRY_SLOT ), cart+1);
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            // zones
            {
                xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_ZONES, lengthof( sampleazureiotTELEMETRY_OBJECT_ZONES ) );
                configASSERT( xResult == eAzureIoTSuccess, xResult );

                // Begin of Zones array
                xResult = AzureIoTJSONWriter_AppendBeginArray( &xWriter );
                configASSERT( xResult == eAzureIoTSuccess, xResult );

                // Zones array in a loop
                for(uint8_t zone = 0;zone<NUMBER_OF_ZONES_PER_CARTRIDGE;++zone){
                    // zone, status, avg, max, min, median
                    memset(ucScratchTempHalfBuffer2, '\0', sizeof(ucScratchTempHalfBuffer2));
                    lBytesWritten = prvCreateUnitHeatersTelemetry( (cart * NUMBER_OF_ZONES_PER_CARTRIDGE + zone), unit_data, ucScratchTempHalfBuffer2, sizeof(ucScratchTempHalfBuffer2) );

                    xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer2, lBytesWritten );
                    configASSERT( xResult == eAzureIoTSuccess, xResult );
                }

                // Endof zones array
                xResult = AzureIoTJSONWriter_AppendEndArray( &xWriter );
                configASSERT( xResult == eAzureIoTSuccess, xResult );
            }

            // Endof cartridges loop
            xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        }

        // Endof cartridges array
        xResult = AzureIoTJSONWriter_AppendEndArray( &xWriter );
        configASSERT( xResult == eAzureIoTSuccess, xResult );
    }

    // Unit status
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_UNIT_STATUS, lengthof( sampleazureiotTELEMETRY_OBJECT_UNIT_STATUS ) );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        memset(ucScratchTempHalfBuffer2, '\0', sizeof(ucScratchTempHalfBuffer2));
        lBytesWritten = prvCreateUnitStatusTelemetry( unit_data, ucScratchTempHalfBuffer2, sizeof(ucScratchTempHalfBuffer2) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer2, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess, xResult );
    }

    // component status
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_COMPONENT_STATUS, lengthof( sampleazureiotTELEMETRY_OBJECT_COMPONENT_STATUS ) );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        memset(ucScratchTempHalfBuffer2, '\0', sizeof(ucScratchTempHalfBuffer2));
        lBytesWritten = prvCreateUnitComponentStatusTelemetry( unit_data, ucScratchTempHalfBuffer2, sizeof(ucScratchTempHalfBuffer2) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer2, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess, xResult );
    }

    // sensor measurements
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_SENSOR_MEASUREMENTS, lengthof( sampleazureiotTELEMETRY_OBJECT_SENSOR_MEASUREMENTS ) );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        xResult = AzureIoTJSONWriter_AppendBeginArray( &xWriter );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        // vacuum sensor
        {
            // Sensor: name, status, avg, max, min, median
            memset(ucScratchTempHalfBuffer2, '\0', sizeof(ucScratchTempHalfBuffer2));
            lBytesWritten = prvCreateUnitSensorTelemetry( UNIT_VACUUM_SENSOR, unit_data, ucScratchTempHalfBuffer2, sizeof(ucScratchTempHalfBuffer2) );

            xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer2, lBytesWritten );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        }

        // ambient humidity
        {
            // Sensor: name, status, avg, max, min, median
            memset(ucScratchTempHalfBuffer2, '\0', sizeof(ucScratchTempHalfBuffer2));
            lBytesWritten = prvCreateUnitSensorTelemetry( UNIT_AMBIENT_HUMIDITY, unit_data, ucScratchTempHalfBuffer2, sizeof(ucScratchTempHalfBuffer2) );

            xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer2, lBytesWritten );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        }

        // ambient temperature
        {
            // Sensor: name, status, avg, max, min, median
            memset(ucScratchTempHalfBuffer2, '\0', sizeof(ucScratchTempHalfBuffer2));
            lBytesWritten = prvCreateUnitSensorTelemetry( UNIT_AMBIENT_TEMPERATURE, unit_data, ucScratchTempHalfBuffer2, sizeof(ucScratchTempHalfBuffer2) );

            xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer2, lBytesWritten );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        }

        xResult = AzureIoTJSONWriter_AppendEndArray( &xWriter );
        configASSERT( xResult == eAzureIoTSuccess, xResult );
    }

    // End top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Unit1 data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

uint32_t prvCreateUnitsTelemetry( UNIT_iot_status_t unit_data, uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // Unit-1
    // @todo: For now we are having only single unit. With Controllino new code. Otherwise we should loop to get 3 units.
    {
        memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
        lBytesWritten = prvCreateUnit1Telemetry(unit_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess, xResult );
    }

    // Unit-2, Unit-3 later
    // @todo: This needs to be taken care with Controllino new code

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Units data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

/**
 * @brief Create and fill the error data.
 * 
 * @todo This handles only single unit. Multi
 *       unit to be supported later.
 * 
 * @param skid_data data pertaining to SKID.
 * @param unit_data data pertaining to UNIT.
 * @param pucTelemetryData buffer to be filled with telemetry
 * @param ulTelemetryDataLength length of the data buffer
 * 
 * @return number of bytes written on success, 0 on failure
 */
uint32_t CreateErrorMessage( 
    SKID_iot_status_t skid_data, UNIT_iot_status_t unit_data,
    uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // non-nested attributes
    {
        xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotMESSAGE_TYPE, lengthof( sampleazureiotMESSAGE_TYPE ),
                                                                    ( uint8_t * )error_MESSAGE_TYPE, strlen(error_MESSAGE_TYPE));
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotMESSAGE_VERSION, lengthof( sampleazureiotMESSAGE_VERSION ),
                                                                    ( uint8_t * )error_MESSAGE_VERSION, strlen(error_MESSAGE_VERSION));
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        const char* timestamp_utc = GetUtcTimeInString();
        xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiot_TIMESTAMP_UTC, lengthof( sampleazureiot_TIMESTAMP_UTC ),
                                                                   ( uint8_t * )timestamp_utc, strlen(timestamp_utc));
        configASSERT( xResult == eAzureIoTSuccess, xResult );
    }

    // nested errors
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_ERRORS, lengthof( sampleazureiotTELEMETRY_OBJECT_ERRORS ) );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        xResult = AzureIoTJSONWriter_AppendBeginArray( &xWriter );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        if( skid_data.skid_state == Lock_State )
        {
            xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            // @todo update timestamp while storing from SKid
            const char* timestamp_utc = GetUtcTimeInString();
            xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiot_TIMESTAMP_UTC, lengthof( sampleazureiot_TIMESTAMP_UTC ),
                                                                        ( uint8_t * )timestamp_utc, strlen(timestamp_utc));
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            char error_code[60] = {0};
            stringifyErrorCode(error_code, skid_data.errors);
            xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_ERROR_CODE, lengthof( sampleazureiotTELEMETRY_ERROR_CODE ),
                                                                        ( uint8_t * )error_code, strlen(error_code));
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotMESSAGE_VERSION, lengthof( sampleazureiotMESSAGE_VERSION ),
                                                                        ( uint8_t * )error_MESSAGE_VERSION, strlen(error_MESSAGE_VERSION));
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_LOCATION, lengthof( sampleazureiotTELEMETRY_LOCATION ),
                                                                        ( uint8_t * )ccu_LOCATION, strlen(ccu_LOCATION));
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_CURRENT_STATE, lengthof( sampleazureiotTELEMETRY_CURRENT_STATE ),
                                                                        ( uint8_t * )sequence_state_stringified[skid_data.skid_state], strlen(sequence_state_stringified[skid_data.skid_state]));
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            // @todo need to implement current and previous states
            xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_PREVIOUS_STATE, lengthof( sampleazureiotTELEMETRY_PREVIOUS_STATE ),
                                                                        ( uint8_t * )sequence_state_stringified[skid_data.skid_state], strlen(sequence_state_stringified[skid_data.skid_state]));
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            // sensor measurements
            {
                xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_SENSOR_MEASUREMENTS, lengthof( sampleazureiotTELEMETRY_OBJECT_SENSOR_MEASUREMENTS ) );
                configASSERT( xResult == eAzureIoTSuccess, xResult );

                xResult = AzureIoTJSONWriter_AppendBeginArray( &xWriter );
                configASSERT( xResult == eAzureIoTSuccess, xResult );

                // o2
                {
                    // Sensor: name, status, avg, max, min, median
                    memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
                    lBytesWritten = prvCreateSkidSensorTelemetry( SKID_O2, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

                    xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
                    configASSERT( xResult == eAzureIoTSuccess, xResult );
                }

                // mass flow
                {
                    // Sensor: name, status, avg, max, min, median
                    memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
                    lBytesWritten = prvCreateSkidSensorTelemetry( SKID_MASS_FLOW, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

                    xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
                    configASSERT( xResult == eAzureIoTSuccess, xResult );
                }

                // co2
                {
                    // Sensor: name, status, avg, max, min, median
                    memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
                    lBytesWritten = prvCreateSkidSensorTelemetry( SKID_CO2, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

                    xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
                    configASSERT( xResult == eAzureIoTSuccess, xResult );
                }

                // proptional valve sensor
                {
                    // Sensor: name, status, avg, max, min, median
                    memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
                    lBytesWritten = prvCreateSkidSensorTelemetry( SKID_PROPOTIONAL_VALVE_SENSOR, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

                    xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
                    configASSERT( xResult == eAzureIoTSuccess, xResult );
                }

                // temperature
                {
                    // Sensor: name, status, avg, max, min, median
                    memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
                    lBytesWritten = prvCreateSkidSensorTelemetry( SKID_TEMPERATURE, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

                    xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
                    configASSERT( xResult == eAzureIoTSuccess, xResult );
                }

                // humidity
                {
                    // Sensor: name, status, avg, max, min, median
                    memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
                    lBytesWritten = prvCreateSkidSensorTelemetry( SKID_HUMIDITY, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

                    xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
                    configASSERT( xResult == eAzureIoTSuccess, xResult );
                }

                // tank pressure
                {
                    // Sensor: name, status, avg, max, min, median
                    memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
                    lBytesWritten = prvCreateSkidSensorTelemetry( TANK_PRESSURE, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

                    xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
                    configASSERT( xResult == eAzureIoTSuccess, xResult );
                }
                
                xResult = AzureIoTJSONWriter_AppendEndArray( &xWriter );
                configASSERT( xResult == eAzureIoTSuccess, xResult );
            }

            // body, @todo nothing now
            {
                xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_BODY, lengthof( sampleazureiotTELEMETRY_OBJECT_BODY ) );
                configASSERT( xResult == eAzureIoTSuccess, xResult );

                xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
                configASSERT( xResult == eAzureIoTSuccess, xResult );

                xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
                configASSERT( xResult == eAzureIoTSuccess, xResult );
            }

            xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        }

        if( unit_data.unit_state == Lock_State )
        {
            xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            // @todo update timestamp while storing from SKid
            const char* timestamp_utc = GetUtcTimeInString();
            xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiot_TIMESTAMP_UTC, lengthof( sampleazureiot_TIMESTAMP_UTC ),
                                                                        ( uint8_t * )timestamp_utc, strlen(timestamp_utc));
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            char error_code[60] = {0};
            stringifyErrorCode(error_code, unit_data.errors);
            xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_ERROR_CODE, lengthof( sampleazureiotTELEMETRY_ERROR_CODE ),
                                                                        ( uint8_t * )error_code, strlen(error_code));
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotMESSAGE_VERSION, lengthof( sampleazureiotMESSAGE_VERSION ),
                                                                        ( uint8_t * )error_MESSAGE_VERSION, strlen(error_MESSAGE_VERSION));
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_LOCATION, lengthof( sampleazureiotTELEMETRY_LOCATION ),
                                                                        ( uint8_t * )unit1_LOCATION, strlen(unit1_LOCATION));
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_CURRENT_STATE, lengthof( sampleazureiotTELEMETRY_CURRENT_STATE ),
                                                                        ( uint8_t * )sequence_state_stringified[unit_data.unit_state], strlen(sequence_state_stringified[unit_data.unit_state]));
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            // @todo need to implement current and previous states
            xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_PREVIOUS_STATE, lengthof( sampleazureiotTELEMETRY_PREVIOUS_STATE ),
                                                                        ( uint8_t * )sequence_state_stringified[unit_data.unit_state], strlen(sequence_state_stringified[unit_data.unit_state]));
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            // sensor measurements
            {
                xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_SENSOR_MEASUREMENTS, lengthof( sampleazureiotTELEMETRY_OBJECT_SENSOR_MEASUREMENTS ) );
                configASSERT( xResult == eAzureIoTSuccess, xResult );

                xResult = AzureIoTJSONWriter_AppendBeginArray( &xWriter );
                configASSERT( xResult == eAzureIoTSuccess, xResult );

                // vacuum sensor
                {
                    // Sensor: name, status, avg, max, min, median
                    memset(ucScratchTempHalfBuffer2, '\0', sizeof(ucScratchTempHalfBuffer2));
                    lBytesWritten = prvCreateUnitSensorTelemetry( UNIT_VACUUM_SENSOR, unit_data, ucScratchTempHalfBuffer2, sizeof(ucScratchTempHalfBuffer2) );

                    xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer2, lBytesWritten );
                    configASSERT( xResult == eAzureIoTSuccess, xResult );
                }

                // ambient humidity
                {
                    // Sensor: name, status, avg, max, min, median
                    memset(ucScratchTempHalfBuffer2, '\0', sizeof(ucScratchTempHalfBuffer2));
                    lBytesWritten = prvCreateUnitSensorTelemetry( UNIT_AMBIENT_HUMIDITY, unit_data, ucScratchTempHalfBuffer2, sizeof(ucScratchTempHalfBuffer2) );

                    xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer2, lBytesWritten );
                    configASSERT( xResult == eAzureIoTSuccess, xResult );
                }

                // ambient temperature
                {
                    // Sensor: name, status, avg, max, min, median
                    memset(ucScratchTempHalfBuffer2, '\0', sizeof(ucScratchTempHalfBuffer2));
                    lBytesWritten = prvCreateUnitSensorTelemetry( UNIT_AMBIENT_TEMPERATURE, unit_data, ucScratchTempHalfBuffer2, sizeof(ucScratchTempHalfBuffer2) );

                    xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer2, lBytesWritten );
                    configASSERT( xResult == eAzureIoTSuccess, xResult );
                }

                xResult = AzureIoTJSONWriter_AppendEndArray( &xWriter );
                configASSERT( xResult == eAzureIoTSuccess, xResult );
            }

            // body, @todo might change
            {
                xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_BODY, lengthof( sampleazureiotTELEMETRY_OBJECT_BODY ) );
                configASSERT( xResult == eAzureIoTSuccess, xResult );

                xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
                configASSERT( xResult == eAzureIoTSuccess, xResult );
                
                // cartridges
                {
                    xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_CARTRIDGES, lengthof( sampleazureiotTELEMETRY_OBJECT_CARTRIDGES ) );
                    configASSERT( xResult == eAzureIoTSuccess, xResult );

                    // Begin of cartridges array
                    xResult = AzureIoTJSONWriter_AppendBeginArray( &xWriter );
                    configASSERT( xResult == eAzureIoTSuccess, xResult );

                    // Begin cartridges loop
                    // @todo: For now, we are filling by reading sequentially from 9 heaters
                    //        to make it 3 cartridges with 3 zones each per cartridge.
                    for(uint8_t cart = 0;cart<NUMBER_OF_CARTRIDGES;++cart){
                        xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
                        configASSERT( xResult == eAzureIoTSuccess, xResult );

                        xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue(
                            &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_SERIAL_NUMBER, lengthof( sampleazureiotTELEMETRY_SERIAL_NUMBER ),
                            ( uint8_t * )xSystemProperties.units[0].cartridges[cart].identity.serial_number,
                            strlen(xSystemProperties.units[0].cartridges[cart].identity.serial_number));
                        configASSERT( xResult == eAzureIoTSuccess, xResult );

                        // @todo we need to get slot number also from desired properties?
                        //       For now using sequential numbers 1, 2, 3...
                        xResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_SLOT, lengthof( sampleazureiotTELEMETRY_SLOT ), cart+1);
                        configASSERT( xResult == eAzureIoTSuccess, xResult );

                        // zones
                        {
                            xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_ZONES, lengthof( sampleazureiotTELEMETRY_OBJECT_ZONES ) );
                            configASSERT( xResult == eAzureIoTSuccess, xResult );

                            // Begin of Zones array
                            xResult = AzureIoTJSONWriter_AppendBeginArray( &xWriter );
                            configASSERT( xResult == eAzureIoTSuccess, xResult );

                            // Zones array in a loop
                            for(uint8_t zone = 0;zone<NUMBER_OF_ZONES_PER_CARTRIDGE;++zone){
                                // zone, status, avg, max, min, median
                                memset(ucScratchTempHalfBuffer2, '\0', sizeof(ucScratchTempHalfBuffer2));
                                lBytesWritten = prvCreateUnitHeatersTelemetry( (cart * NUMBER_OF_ZONES_PER_CARTRIDGE + zone), unit_data, ucScratchTempHalfBuffer2, sizeof(ucScratchTempHalfBuffer2) );

                                xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer2, lBytesWritten );
                                configASSERT( xResult == eAzureIoTSuccess, xResult );
                            }

                            // Endof zones array
                            xResult = AzureIoTJSONWriter_AppendEndArray( &xWriter );
                            configASSERT( xResult == eAzureIoTSuccess, xResult );
                        }

                        // Endof cartridges loop
                        xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
                        configASSERT( xResult == eAzureIoTSuccess, xResult );
                    }

                    // Endof cartridges array
                    xResult = AzureIoTJSONWriter_AppendEndArray( &xWriter );
                    configASSERT( xResult == eAzureIoTSuccess, xResult );
                }

                xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
                configASSERT( xResult == eAzureIoTSuccess, xResult );
            }

            xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        }

        xResult = AzureIoTJSONWriter_AppendEndArray( &xWriter );
        configASSERT( xResult == eAzureIoTSuccess, xResult );
    }

    // End top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "error message to be sent %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

/**
 * @brief Create and fill the boot-up data.
 * 
 * @todo This handles only single unit. Multi
 *       unit to be supported later.
 * 
 * @param pucTelemetryData buffer to be filled with telemetry
 * @param ulTelemetryDataLength length of the data buffer
 * 
 * @return number of bytes written on success, 0 on failure
 */
uint32_t CreateBootUpMessage( uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // non-nested attributes
    {
        xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotMESSAGE_TYPE, lengthof( sampleazureiotMESSAGE_TYPE ),
                                                                    ( uint8_t * )bootup_MESSAGE_TYPE, strlen(bootup_MESSAGE_TYPE));
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotMESSAGE_VERSION, lengthof( sampleazureiotMESSAGE_VERSION ),
                                                                    ( uint8_t * )bootup_MESSAGE_VERSION, strlen(bootup_MESSAGE_VERSION));
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiot_AZURE_SDK_VERSION, lengthof( sampleazureiot_AZURE_SDK_VERSION ),
                                                                    ( uint8_t * )azure_sdk_version, strlen(azure_sdk_version));
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        const char* timestamp_utc = GetUtcTimeInString();
        xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiot_TIMESTAMP_UTC, lengthof( sampleazureiot_TIMESTAMP_UTC ),
                                                                   ( uint8_t * )timestamp_utc, strlen(timestamp_utc));
        configASSERT( xResult == eAzureIoTSuccess, xResult );
    }

    // firmware versions
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiot_FIRMWARE_VERSIONS, lengthof( sampleazureiot_FIRMWARE_VERSIONS ) );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        xResult = AzureIoTJSONWriter_AppendBeginArray( &xWriter );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        // skid
        {
            xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiot_IDENTIFIER, lengthof( sampleazureiot_IDENTIFIER ),
                                                                        ( uint8_t * )ccu_IDENTIFIER, strlen(ccu_IDENTIFIER));
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotMESSAGE_VERSION, lengthof( sampleazureiotMESSAGE_VERSION ),
                                                                        ( uint8_t * )ccu_FIRMWARE_VERSION, strlen(ccu_FIRMWARE_VERSION));
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        }

        // Unit1
        // @todo - for now we have only 1 unit
        {
            xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiot_IDENTIFIER, lengthof( sampleazureiot_IDENTIFIER ),
                                                                        ( uint8_t * )unit1_IDENTIFIER, strlen(unit1_IDENTIFIER));
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotMESSAGE_VERSION, lengthof( sampleazureiotMESSAGE_VERSION ),
                                                                        ( uint8_t * )unit1_FIRMWARE_VERSION, strlen(unit1_FIRMWARE_VERSION));

            xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        }

        xResult = AzureIoTJSONWriter_AppendEndArray( &xWriter );
        configASSERT( xResult == eAzureIoTSuccess, xResult );
    }

    // hardware versions
    {
        // @todo
    }

    // End top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "bootup message to be sent %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}
/*-----------------------------------------------------------*/

/**
 * @brief Create and fill the telemetry data.
 * 
 * @todo This handles only single unit. Multi
 *       unit to be supported later.
 * 
 * @param skid_data data pertaining to SKID.
 * @param unit_data data pertaining to UNIT.
 * @param pucTelemetryData buffer to be filled with telemetry
 * @param ulTelemetryDataLength length of the data buffer
 * 
 * @return number of bytes written on success, 0 on failure
 */
uint32_t CreateTelemetry(
    SKID_iot_status_t skid_data, UNIT_iot_status_t unit_data,
    uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    // Note: Later optimize the code separating common code etc!!

    // non-nested attributes
    {
        xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotMESSAGE_VERSION, lengthof( sampleazureiotMESSAGE_VERSION ),
                                                                    ( uint8_t * )telemetry_MESSAGE_VERSION, strlen(telemetry_MESSAGE_VERSION));
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        // For now the count is constant, to be deduced from sampling frequency later
        xResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_MEASUREMENT_COUNT, lengthof( sampleazureiotTELEMETRY_MEASUREMENT_COUNT ),
                                                                   telemetry_MEASUREMENT_COUNT);
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        const char* timestamp_utc = GetUtcTimeInString();
        xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiot_TIMESTAMP_UTC, lengthof( sampleazureiot_TIMESTAMP_UTC ),
                                                                   ( uint8_t * )timestamp_utc, strlen(timestamp_utc));
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotMESSAGE_TYPE, lengthof( sampleazureiotMESSAGE_TYPE ),
                                                                    ( uint8_t * )telemetry_MESSAGE_TYPE, strlen(telemetry_MESSAGE_TYPE));
        configASSERT( xResult == eAzureIoTSuccess, xResult );
    }

    // Skid
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_CCU, lengthof( sampleazureiotTELEMETRY_OBJECT_CCU ) );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        // All skid sensors
        memset(ucScratchTempBuffer, '\0', sizeof(ucScratchTempBuffer));
        lBytesWritten = prvCreateSkidTelemetry(skid_data, ucScratchTempBuffer, sizeof(ucScratchTempBuffer) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempBuffer, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess, xResult );
    }
    // Endof Skid

    // Units
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_UNITS, lengthof( sampleazureiotTELEMETRY_OBJECT_UNITS ) );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        xResult = AzureIoTJSONWriter_AppendBeginArray( &xWriter );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        // All Units
        memset(ucScratchTempBuffer, '\0', sizeof(ucScratchTempBuffer));
        lBytesWritten = prvCreateUnitsTelemetry( unit_data, ucScratchTempBuffer, sizeof(ucScratchTempBuffer) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempBuffer, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        xResult = AzureIoTJSONWriter_AppendEndArray( &xWriter );
        configASSERT( xResult == eAzureIoTSuccess, xResult );
    }
    // Endof Units

    // Tank
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_TANK, lengthof( sampleazureiotTELEMETRY_OBJECT_TANK ) );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        // Begin tank object
        xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( 
            &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_SERIAL_NUMBER, lengthof( sampleazureiotTELEMETRY_SERIAL_NUMBER ),
            ( uint8_t * )xSystemProperties.tank.identity.serial_number, strlen(xSystemProperties.tank.identity.serial_number));
        configASSERT( xResult == eAzureIoTSuccess, xResult );

        // sensor measurements
        {
            xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_SENSOR_MEASUREMENTS, lengthof( sampleazureiotTELEMETRY_OBJECT_SENSOR_MEASUREMENTS ) );
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            xResult = AzureIoTJSONWriter_AppendBeginArray( &xWriter );
            configASSERT( xResult == eAzureIoTSuccess, xResult );

            // tank pressure
            {
                // Sensor: name, status, avg, max, min, median
                memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
                lBytesWritten = prvCreateSkidSensorTelemetry( TANK_PRESSURE, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

                xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
                configASSERT( xResult == eAzureIoTSuccess, xResult );
            }

            xResult = AzureIoTJSONWriter_AppendEndArray( &xWriter );
            configASSERT( xResult == eAzureIoTSuccess, xResult );
        }

        // End tank object
        xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
        configASSERT( xResult == eAzureIoTSuccess, xResult );
    }
    // Endof Tank

    // End top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess, xResult );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Telemetry message to be sent %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}
/*-----------------------------------------------------------*/
