/* Copyright (c) Skytree B.V. All rights reserved. */

/* Local includes */
#include "controller_data.h"
#include "config.h"
#include "iot_communication.h"

/* Standard includes. */
#include <memory.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "semphr.h"

// @todo Make this log stuff bit more cleaner
#undef LIBRARY_LOG_NAME
#define LIBRARY_LOG_NAME    "ControllerData"

//========================================================================================================== DEFINITIONS AND MACROS
#define MUTEX_MAX_BLOCKING_TIME         (1000)    // Max blocking time for the mutex lock
#define NUMBER_OF_SAMPLES               (30)      // Number of samples to accummulate before pushing it to backend

#define WORKING_COMMUNICATION_VERSION   (16)      // 1.0
//========================================================================================================== VARIABLES
CCU_telemetry_t xCcuTelemetry[NUMBER_OF_SAMPLES] = {0};
CMU_telemetry_t xCmuTelemetry[NUMBER_OF_SAMPLES] = {0}; // @todo This should become an array of units

// We receive unit and skid separately so need to manage separate indexes
static uint8_t xCmuCircularBufferIndex = 0;
static uint8_t xCcuCircularBufferIndex = 0;

//------------------------------------------ mutexes for read write operations on CCU/CMU status data structs
SemaphoreHandle_t xCcuDataRWMutex;
StaticSemaphore_t xCcuDataMutexBuffer;

SemaphoreHandle_t xCmuDataRWMutex;
StaticSemaphore_t xCmuDataMutexBuffer;

//========================================================================================================== FUNCTIONS DECLARATIONS
static void prvSystemDataUnit(void);
static uint16_t prvCalculateCrc16(const uint8_t data[], size_t nbrOfBytes);
static void prvReadControllerData( void );
static void prvControllerCommunicationTask( void* arg );
static void prvReadCcuData( uint8_t ucIncomingData[] );
static void prvReadCmuData( uint8_t ucIncomingData[] );

static uint8_t prvGetLatestIndex(bool skid);
static void sensor_average_max_min(sensor_name_t name, uint8_t heater_index, sensor_info_t* sensor_info);
static uint8_t prvGetLatestIndex(bool skid);
static double get_sensor_value(uint8_t sample_index, sensor_name_t name, uint8_t heater_index);
//========================================================================================================== FUNCTIONS DEFINITIONS

/**
 * @brief Controller communication task loop
 */
static void prvControllerCommunicationTask( void* arg )
{
    ( void ) arg;

    prvSystemDataUnit();

    while( 1 )
    {
        prvReadControllerData();
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Controller communication initialization
*/
static void prvSystemDataUnit(void)
{
    xCcuDataRWMutex = xSemaphoreCreateMutexStatic(&xCcuDataMutexBuffer);
    xCmuDataRWMutex = xSemaphoreCreateMutexStatic(&xCmuDataMutexBuffer);

    gui_comm_init();
}
/*-----------------------------------------------------------*/

static uint16_t prvCalculateCrc16(const uint8_t data[], size_t nbrOfBytes)
{
    uint16_t crc = 0xFFFF; // Initial value
    for(size_t byte = 0; byte < nbrOfBytes; byte++) 
    {
        crc ^= ((uint16_t)(data[byte]) << 8);
        for (uint8_t bit = 0; bit < 8; bit++) 
        {
          if (crc & 0x8000) 
          {
            crc = (crc << 1) ^ CRC16_POLYNOMIAL;
          } 
          else 
          {
            crc = crc << 1;
          }
        }
      }

      return crc;
}
/*-----------------------------------------------------------*/

/**
 * @brief Read CMU data
*/
static void prvReadCmuData( uint8_t ucIncomingData[] )
{
    xSemaphoreTake(xCcuDataRWMutex, MUTEX_MAX_BLOCKING_TIME);

    // Source type: 1 byte
    //    We have already parsed and identified CMU
    // CMU id: 1 byte
    //    @todo use the id to store, probably make xCmuTelemetry an array
    // State: 1 byte
    xCmuTelemetry[xCcuCircularBufferIndex].ucCmuState = ucIncomingData[T_M_CMU_STATE_INDEX];
    // Error Code: 2 bytes
    xCmuTelemetry[xCcuCircularBufferIndex].xErrorCode = (((uint16_t)(ucIncomingData[T_M_CMU_ERROR_CODE_INDEX]) << 8) & 0xFF00); 
    xCmuTelemetry[xCcuCircularBufferIndex].xErrorCode |= ((uint16_t)(ucIncomingData[T_M_CMU_ERROR_CODE_INDEX + 1]) & 0xFF);
    // Working Flags: 2 bytes 
    xCmuTelemetry[xCcuCircularBufferIndex].xWorkingFlags.uFlags = (((uint16_t)(ucIncomingData[T_M_CMU_WORKING_FLAGS_INDEX]) << 8) & 0xFF00); 
    xCmuTelemetry[xCcuCircularBufferIndex].xWorkingFlags.uFlags |= ((uint16_t)(ucIncomingData[T_M_CMU_WORKING_FLAGS_INDEX + 1]) & 0xFF);
    // Output status: 2 bytes
    xCmuTelemetry[xCcuCircularBufferIndex].xOutputStatus.uStatus = (((uint16_t)(ucIncomingData[T_M_CMU_OUTPUT_STATUS_INDEX]) << 8) & 0xFF00); 
    xCmuTelemetry[xCcuCircularBufferIndex].xOutputStatus.uStatus |= ((uint16_t)(ucIncomingData[T_M_CMU_OUTPUT_STATUS_INDEX + 1]) & 0xFF);
    // Sensor status: 2 bytes
    xCmuTelemetry[xCcuCircularBufferIndex].xSensorStatus.uStatus = (((uint16_t)(ucIncomingData[T_M_CMU_SENSORS_STATUS_INDEX]) << 8) & 0xFF00); 
    xCmuTelemetry[xCcuCircularBufferIndex].xSensorStatus.uStatus |= ((uint16_t)(ucIncomingData[T_M_CMU_SENSORS_STATUS_INDEX + 1]) & 0xFF);
    // Vacuum chamber: 4 bytes
    uint32_t ulTemp = 0;
    ulTemp |= ((uint32_t)(ucIncomingData[T_M_CMU_VACUUM_CHAMBER_PRESSURE_VALUE_INDEX]) << 24) & 0xFF000000;
    ulTemp |= ((uint32_t)(ucIncomingData[T_M_CMU_VACUUM_CHAMBER_PRESSURE_VALUE_INDEX + 1]) << 16) & 0xFF0000;
    ulTemp |= ((uint32_t)(ucIncomingData[T_M_CMU_VACUUM_CHAMBER_PRESSURE_VALUE_INDEX + 2]) << 8) & 0xFF00;
    ulTemp |= (uint32_t)(ucIncomingData[T_M_CMU_VACUUM_CHAMBER_PRESSURE_VALUE_INDEX + 3]) & 0xFF;
    xCmuTelemetry[xCcuCircularBufferIndex].dVacuumChamber = (double)ulTemp / (1 << 16);
    // Ambient temperature: 4 bytes
    int32_t lTemp = 0;
    lTemp |= ((uint32_t)(ucIncomingData[T_M_CMU_AMBIENT_TEMPERATURE_SENSOR_VALUE_INDEX]) << 24) & 0xFF000000;
    lTemp |= ((uint32_t)(ucIncomingData[T_M_CMU_AMBIENT_TEMPERATURE_SENSOR_VALUE_INDEX + 1]) << 16) & 0xFF0000;
    lTemp |= ((uint32_t)(ucIncomingData[T_M_CMU_AMBIENT_TEMPERATURE_SENSOR_VALUE_INDEX + 2]) << 8) & 0xFF00;
    lTemp |= (uint32_t)(ucIncomingData[T_M_CMU_AMBIENT_TEMPERATURE_SENSOR_VALUE_INDEX + 3]) & 0xFF;
    xCmuTelemetry[xCcuCircularBufferIndex].dAmbientTemperature = (double)lTemp / (1 << 16);
    // Ambient humidity: 4 bytes
    ulTemp = 0;
    ulTemp |= ((uint32_t)(ucIncomingData[T_M_CMU_AMVIENT_HUMIDITY_SENSOR_VALUE_INDEX]) << 24) & 0xFF000000;
    ulTemp |= ((uint32_t)(ucIncomingData[T_M_CMU_AMVIENT_HUMIDITY_SENSOR_VALUE_INDEX + 1]) << 16) & 0xFF0000;
    ulTemp |= ((uint32_t)(ucIncomingData[T_M_CMU_AMVIENT_HUMIDITY_SENSOR_VALUE_INDEX + 2]) << 8) & 0xFF00;
    ulTemp |= (uint32_t)(ucIncomingData[T_M_CMU_AMVIENT_HUMIDITY_SENSOR_VALUE_INDEX + 3]) & 0xFF;
    xCmuTelemetry[xCcuCircularBufferIndex].dAmbientHumidity = (double)ulTemp / (1 << 16);
    // Number of heaters: 1 byte
    xCmuTelemetry[xCcuCircularBufferIndex].ucNumberOfHeaters = ucIncomingData[T_M_CMU_HEATERS_NUM_INDEX];
    // Heater status: 2 bytes
    xCmuTelemetry[xCcuCircularBufferIndex].uHeaterStatus = (((uint16_t)(ucIncomingData[T_M_CMU_HEATERS_STATUS_INDEX]) << 8) & 0xFF00); 
    xCmuTelemetry[xCcuCircularBufferIndex].uHeaterStatus |= ((uint16_t)(ucIncomingData[T_M_CMU_HEATERS_STATUS_INDEX + 1]) & 0xFF);
    // Heaters temperature: dynamic depends on number of heaters
    for( uint8_t ucIndex = 0; ucIndex < ucIncomingData[T_M_CMU_HEATERS_NUM_INDEX]; ++ucIndex )
    {
        // Hop to next heater by adding  (ucIndex * 4): Each heater is 4 bytes
        ulTemp = 0;
        ulTemp |= ((uint32_t)(ucIncomingData[T_M_CMU_HEATER_1_TEMPERAUTE_INDEX + (ucIndex * 4)]) << 24) & 0xFF000000;
        ulTemp |= ((uint32_t)(ucIncomingData[T_M_CMU_HEATER_1_TEMPERAUTE_INDEX + 1 + (ucIndex * 4)]) << 16) & 0xFF0000;
        ulTemp |= ((uint32_t)(ucIncomingData[T_M_CMU_HEATER_1_TEMPERAUTE_INDEX + 2 + (ucIndex * 4)]) << 8) & 0xFF00;
        ulTemp |= (uint32_t)(ucIncomingData[T_M_CMU_HEATER_1_TEMPERAUTE_INDEX + 3 + (ucIndex * 4)]) & 0xFF;
        xCmuTelemetry[xCcuCircularBufferIndex].dHeaterTemperatures[ucIndex] = (double)ulTemp / (1 << 16);
    }

    // Update the circular index for next turn once current index is filled
    xCmuCircularBufferIndex++;
    if(xCmuCircularBufferIndex >= NUMBER_OF_SAMPLES)
    {
      xCmuCircularBufferIndex = 0;
    }

    xSemaphoreGive(xCmuDataRWMutex);
}
/*-----------------------------------------------------------*/

/**
 * @brief Read CCU data
*/
static void prvReadCcuData( uint8_t ucIncomingData[] )
{
    xSemaphoreTake(xCcuDataRWMutex, MUTEX_MAX_BLOCKING_TIME);

    // Source type: 1 byte
    //    We have already parsed and identified CCU
    // CCU id: 1 byte
    //    It is not used right now
    // State: 1 byte
    xCcuTelemetry[xCcuCircularBufferIndex].ucCcuState = ucIncomingData[T_M_CCU_STATE_INDEX];
    // Error Code: 2 bytes
    xCcuTelemetry[xCcuCircularBufferIndex].xErrorCode = (((uint16_t)(ucIncomingData[T_M_CCU_ERROR_CODE_INDEX]) << 8) & 0xFF00); 
    xCcuTelemetry[xCcuCircularBufferIndex].xErrorCode |= ((uint16_t)(ucIncomingData[T_M_CCU_ERROR_CODE_INDEX + 1]) & 0xFF);
    // Working Flags: 2 bytes 
    xCcuTelemetry[xCcuCircularBufferIndex].xWorkingFlags.uFlags = (((uint16_t)(ucIncomingData[T_M_CCU_WORKING_FLAGS_INDEX]) << 8) & 0xFF00); 
    xCcuTelemetry[xCcuCircularBufferIndex].xWorkingFlags.uFlags |= ((uint16_t)(ucIncomingData[T_M_CCU_WORKING_FLAGS_INDEX + 1]) & 0xFF);
    // Output status: 2 bytes
    xCcuTelemetry[xCcuCircularBufferIndex].xOutputStatus.uStatus = (((uint16_t)(ucIncomingData[T_M_CCU_OUTPUT_STATUS_INDEX]) << 8) & 0xFF00); 
    xCcuTelemetry[xCcuCircularBufferIndex].xOutputStatus.uStatus |= ((uint16_t)(ucIncomingData[T_M_CCU_OUTPUT_STATUS_INDEX + 1]) & 0xFF);
    // Sensor status: 2 bytes
    xCcuTelemetry[xCcuCircularBufferIndex].xSensorStatus.uStatus = (((uint16_t)(ucIncomingData[T_M_CCU_SENSORS_STATUS_INDEX]) << 8) & 0xFF00); 
    xCcuTelemetry[xCcuCircularBufferIndex].xSensorStatus.uStatus |= ((uint16_t)(ucIncomingData[T_M_CCU_SENSORS_STATUS_INDEX + 1]) & 0xFF);
    // O2: 4 bytes
    uint32_t ulTemp = 0;
    ulTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_O2_SENSOR_VALUE_INDEX]) << 24) & 0xFF000000;
    ulTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_O2_SENSOR_VALUE_INDEX + 1]) << 16) & 0xFF0000;
    ulTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_O2_SENSOR_VALUE_INDEX + 2]) << 8) & 0xFF00;
    ulTemp |= (uint32_t)(ucIncomingData[T_M_CCU_O2_SENSOR_VALUE_INDEX + 3]) & 0xFF;
    xCcuTelemetry[xCcuCircularBufferIndex].dO2 = (double)ulTemp / (1 << 16);
    // Mass flow: 4 bytes
    int32_t lTemp = 0;
    lTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_MASS_FLOW_SENSOR_VALUE_INDEX]) << 24) & 0xFF000000;
    lTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_MASS_FLOW_SENSOR_VALUE_INDEX + 1]) << 16) & 0xFF0000;
    lTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_MASS_FLOW_SENSOR_VALUE_INDEX + 2]) << 8) & 0xFF00;
    lTemp |= (uint32_t)(ucIncomingData[T_M_CCU_MASS_FLOW_SENSOR_VALUE_INDEX + 3]) & 0xFF;
    xCcuTelemetry[xCcuCircularBufferIndex].dMassFlow = (double)lTemp / (1 << 16);
    // Co2: 4 bytes
    ulTemp = 0;
    ulTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_CO2_SENSOR_VALUE_INDEX]) << 24) & 0xFF000000;
    ulTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_CO2_SENSOR_VALUE_INDEX + 1]) << 16) & 0xFF0000;
    ulTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_CO2_SENSOR_VALUE_INDEX + 2]) << 8) & 0xFF00;
    ulTemp |= (uint32_t)(ucIncomingData[T_M_CCU_CO2_SENSOR_VALUE_INDEX + 3]) & 0xFF;
    xCcuTelemetry[xCcuCircularBufferIndex].dCo2 = (double)ulTemp / (1 << 16);
    // Tank pressure: 4 bytes
    ulTemp = 0;
    ulTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_TANK_PRESSURE_SENSOR_VALUE_INDEX]) << 24) & 0xFF000000;
    ulTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_TANK_PRESSURE_SENSOR_VALUE_INDEX + 1]) << 16) & 0xFF0000;
    ulTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_TANK_PRESSURE_SENSOR_VALUE_INDEX + 2]) << 8) & 0xFF00;
    ulTemp |= (uint32_t)(ucIncomingData[T_M_CCU_TANK_PRESSURE_SENSOR_VALUE_INDEX + 3]) & 0xFF;
    xCcuTelemetry[xCcuCircularBufferIndex].dTankPressure = (double)ulTemp / (1 << 16);
    // Propotional valve pressure: 4 bytes
    ulTemp = 0;
    ulTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_PVP_SENSOR_VALUE_INDEX]) << 24) & 0xFF000000;
    ulTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_PVP_SENSOR_VALUE_INDEX + 1]) << 16) & 0xFF0000;
    ulTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_PVP_SENSOR_VALUE_INDEX + 2]) << 8) & 0xFF00;
    ulTemp |= (uint32_t)(ucIncomingData[T_M_CCU_PVP_SENSOR_VALUE_INDEX + 3]) & 0xFF;
    xCcuTelemetry[xCcuCircularBufferIndex].dProportionalValvePressure = (double)ulTemp / (1 << 16);
    // Relative temperature: 4 bytes
    lTemp = 0;
    lTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_RELATIVE_TEMPERATURE_SENSOR_VALUE_INDEX]) << 24) & 0xFF000000;
    lTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_RELATIVE_TEMPERATURE_SENSOR_VALUE_INDEX + 1]) << 16) & 0xFF0000;
    lTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_RELATIVE_TEMPERATURE_SENSOR_VALUE_INDEX + 2]) << 8) & 0xFF00;
    lTemp |= (uint32_t)(ucIncomingData[T_M_CCU_RELATIVE_TEMPERATURE_SENSOR_VALUE_INDEX + 3]) & 0xFF;
    xCcuTelemetry[xCcuCircularBufferIndex].dRelativeTemperature = (double)lTemp / (1 << 16);
    // Relative humidity: 4 bytes
    lTemp = 0;
    lTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_RELATIVE_HUMIDITY_SENSOR_VALUE_INDEX]) << 24) & 0xFF000000;
    lTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_RELATIVE_HUMIDITY_SENSOR_VALUE_INDEX + 1]) << 16) & 0xFF0000;
    lTemp |= ((uint32_t)(ucIncomingData[T_M_CCU_RELATIVE_HUMIDITY_SENSOR_VALUE_INDEX + 2]) << 8) & 0xFF00;
    lTemp |= (uint32_t)(ucIncomingData[T_M_CCU_RELATIVE_HUMIDITY_SENSOR_VALUE_INDEX + 3]) & 0xFF;
    xCcuTelemetry[xCcuCircularBufferIndex].dRelativeHumidity = (double)lTemp / (1 << 16);

    // Update the circular index for next turn once current index is filled
    xCcuCircularBufferIndex++;
    if(xCcuCircularBufferIndex >= NUMBER_OF_SAMPLES)
    {
      xCcuCircularBufferIndex = 0;
    }

    xSemaphoreGive(xCcuDataRWMutex);
}
/*-----------------------------------------------------------*/

/**
 * @brief Read data sent from controller
*/
static void prvReadControllerData( void )
{
    static uint8_t ucIncomingData[MAX_PACKET_LENGTH] = {0};
    static uint8_t ucBytePosition = IOT_PACKET_HEADER_INDEX;
    static IoT_Communication_Command_t xCommand = INVALID_COMMAND;
    static uint8_t ucPayloadLength = 0;

    switch(ucBytePosition)
    {
        case IOT_PACKET_HEADER_INDEX:
            if( FAILED != gui_comm_check_for_received_data( ucIncomingData + IOT_PACKET_HEADER_INDEX, PACKET_HEADER_NUM_OF_BYTES ) )
            {
                /**
                 * @todo Validate header
                 */ 
                ucBytePosition += PACKET_HEADER_NUM_OF_BYTES;
            }
            break;
        case IOT_PACKET_COMMAND_INDEX:
            if( FAILED != gui_comm_check_for_received_data( ucIncomingData + IOT_PACKET_COMMAND_INDEX, COMMAND_NUM_OF_BYTES ) )
            {
                xCommand = ( IoT_Communication_Command_t )ucIncomingData[ucBytePosition];

                ucBytePosition += COMMAND_NUM_OF_BYTES;
            }
            break;
        case IOT_PACKET_VERSION_INDEX:
            if( FAILED != gui_comm_check_for_received_data( ucIncomingData + IOT_PACKET_VERSION_INDEX, VERSION_BYTE_NUM_OF_BYTES ) )
            {
                if( WORKING_COMMUNICATION_VERSION != ucIncomingData[IOT_PACKET_VERSION_INDEX] )
                {
                    // @todo validate and reject if version does not match
                    LogDebug( ( "version(%d) not supported", ucIncomingData[IOT_PACKET_VERSION_INDEX] ) );
                }
                ucBytePosition += VERSION_BYTE_NUM_OF_BYTES;
            }
            break;
        case IOT_PAYLOAD_LENGTH_INDEX:
            if( FAILED != gui_comm_check_for_received_data( ucIncomingData + IOT_PAYLOAD_LENGTH_INDEX, PAYLOAD_LENGTH_NUM_OF_BYTES ) )
            {
                ucPayloadLength = ucIncomingData[IOT_PAYLOAD_LENGTH_INDEX];
                ucBytePosition += PAYLOAD_LENGTH_NUM_OF_BYTES;
                LogDebug( ( "payload length - %d", ucIncomingData[IOT_PAYLOAD_LENGTH_INDEX] ) );
            }
            break;
        case IOT_PAYLOAD_INDEX:
            if( 0 == ucPayloadLength )
            {
                // Do nothing for now
                // @todo: Validate length against commands
            }

            if( FAILED != gui_comm_check_for_received_data( ucIncomingData + IOT_PAYLOAD_INDEX, ucPayloadLength ) )
            {
                if( prvCalculateCrc16( ucIncomingData, PACKET_METADATA_NUM_OF_BYTES + ucPayloadLength )
                                     == ( *((uint16_t*)&ucIncomingData[PACKET_METADATA_NUM_OF_BYTES + ucPayloadLength] ) ) )
                {
                    switch( xCommand )
                    {
                        case TELEMETRY_MSG:
                            // Check the source of data: CCU or CMU
                            if( TELEMETRY_SOURCE_CCU_IDENTIFIER == ucIncomingData[PACKET_METADATA_NUM_OF_BYTES] )
                            {
                                prvReadCcuData(ucIncomingData + PACKET_METADATA_NUM_OF_BYTES + 1);
                            }
                            else if( TELEMETRY_SOURCE_CMU_IDENTIFIER == ucIncomingData[PACKET_METADATA_NUM_OF_BYTES] )
                            {
                                prvReadCmuData(ucIncomingData + PACKET_METADATA_NUM_OF_BYTES + 1);
                            }
                            break;
                        case BOOTUP_STATUS:   // @todo 
                        case ERROR_MSG:       // @todo
                        case SEND_LOCK_ACK:   // @todo
                        case SEND_UNLOCK_ACK: // @todo
                        default:
                            break;
                    }
                }
            }

            // As we have read data, reset the index for next read
            ucBytePosition = IOT_RESTART_INDEX;
            break;
        
        case IOT_RESTART_INDEX:
            // Restart reading as we are done with reading
            ucBytePosition = IOT_PACKET_HEADER_INDEX;
            memset( ucIncomingData, 0, sizeof(ucIncomingData) );
            break;
        
        default:
            // We should never reach here
            break;
    }
}
/*-----------------------------------------------------------*/


#if 0
/**
 * @brief Get latest idex for accessing the data samples
 * 
 * @param[in] bCcu if the index is for Ccu or Cmu
*/
static uint8_t prvGetLatestIndex( bool bCcu )
{
    uint8_t ucIndex = xCmuCircularBufferIndex;

    if( bCcu )
    {
        ucIndex = xCcuCircularBufferIndex;
    }

    if( ucIndex != 0 )
    {
        // We do one back as the indexes are always jumped to next after updating
        ucIndex--;
    }
    else
    {
        ucIndex = NUMBER_OF_SAMPLES - 1;
    }

    return ucIndex;
}
/*-----------------------------------------------------------*/

static double get_sensor_value(uint8_t sample_index, sensor_name_t name, uint8_t heater_index)
{
  double sensor_value = 0.0;
  switch(name)
  {
    // Skid sensors
    case SKID_O2:
      sensor_value = skid_status[sample_index].o2_sensor;
    break;
    case SKID_MASS_FLOW:
      sensor_value = skid_status[sample_index].mass_flow;
    break;
    case SKID_CO2:
      sensor_value = skid_status[sample_index].co2_sensor;
    break;
    case SKID_PROPOTIONAL_VALVE_SENSOR:
      sensor_value = skid_status[sample_index].proportional_valve_pressure;
    break;
    case SKID_TEMPERATURE:
      sensor_value = skid_status[sample_index].temperature;
    break;
    case SKID_HUMIDITY:
      sensor_value = skid_status[sample_index].humidity;
    break;
    // Unit sensors
    case UNIT_VACUUM_SENSOR:
      sensor_value = unit_status[sample_index].vacuum_sensor;
    break;
    case UNIT_AMBIENT_HUMIDITY:
      sensor_value = unit_status[sample_index].ambient_humidity;
    break;
    case UNIT_AMBIENT_TEMPERATURE:
      sensor_value = unit_status[sample_index].ambient_temperature;
    break;
    case UNIT_HEATER:
      sensor_value = unit_status[sample_index].heater_temperatures[heater_index];
    break;
    case TANK_PRESSURE:
      sensor_value = skid_status[sample_index].tank_pressure;
    break;
    default:
      break;
  }

  return sensor_value;
}
/*-----------------------------------------------------------*/

// Predicate for qsort used to get to media
static int compare_for_sort(const void *a, const void *b)
{
    return (*(double *)a - *(double *)b);
}
/*-----------------------------------------------------------*/

static double get_median(double sensor_samples[])
{
  qsort(sensor_samples, NUMBER_OF_SAMPLES, sizeof(double), compare_for_sort);

  if (NUMBER_OF_SAMPLES % 2 == 0)
  {
      // If the number of elements is even
      return (sensor_samples[NUMBER_OF_SAMPLES / 2 - 1] + sensor_samples[NUMBER_OF_SAMPLES / 2]) / 2.0;
  }
  else
  {
      // If the number of elements is odd
      return skid_status[NUMBER_OF_SAMPLES / 2].o2_sensor;
  }
}
/*-----------------------------------------------------------*/

static void update_sensor_total(sensor_name_t name, double sensor_value, double* total, uint8_t* number_of_valid_samples)
{
    if( (name == SKID_O2 && (sensor_value < 0 || sensor_value > 23)) ||
        (name == SKID_MASS_FLOW && (sensor_value > 60)) ||
        (name == SKID_CO2 && (sensor_value < 0 || sensor_value > 1)) ||
        (name == SKID_PROPOTIONAL_VALVE_SENSOR && (sensor_value < 0 || sensor_value > 3)) ||
        (name == SKID_TEMPERATURE && (sensor_value < -20 || sensor_value > 120)) ||
        (name == SKID_HUMIDITY && (sensor_value < 0 || sensor_value > 100)) ||
        (name == UNIT_VACUUM_SENSOR && (sensor_value < 0 || sensor_value > 1.2)) ||
        (name == UNIT_AMBIENT_HUMIDITY && (sensor_value < 0 || sensor_value > 100)) ||
        (name == UNIT_AMBIENT_TEMPERATURE && (sensor_value < -20 || sensor_value > 120)) ||
        (name == UNIT_HEATER && (sensor_value < 0 || sensor_value > 150)) ||
        (name == TANK_PRESSURE && (sensor_value < 0 || sensor_value > 6)) )
    {
      double integer_part = 0;
      modf(sensor_value, &integer_part);
      LogDebug( ( "Invalid sensor (%d) value integer-part (%d)" , name, (int32_t)integer_part) );
    }
    else
    {
      *total += sensor_value;
      *number_of_valid_samples += 1;
    }
}
/*-----------------------------------------------------------*/

static void sensor_average_max_min(sensor_name_t name, uint8_t heater_index, sensor_info_t* sensor_info)
{
  double total = 0.0;
  double sensor_samples[NUMBER_OF_SAMPLES];
  double sensor_value = 0.0;
  uint8_t number_of_valid_samples = 0;

  sensor_samples[0] = sensor_info->stats.avg = sensor_info->stats.max
    = sensor_info->stats.min = sensor_info->stats.median = sensor_value = get_sensor_value(0, name, heater_index);
  update_sensor_total(name, sensor_value, &total, &number_of_valid_samples);

  for(uint8_t i=1;i<NUMBER_OF_SAMPLES;++i)
  {
    sensor_value = sensor_samples[i] = get_sensor_value(i, name, heater_index);

    update_sensor_total(name, sensor_value, &total, &number_of_valid_samples);

    if(sensor_value > sensor_info->stats.max)
    {
      sensor_info->stats.max = sensor_value;
    }
    if(sensor_value < sensor_info->stats.min)
    {
      sensor_info->stats.min = sensor_value;
    }
  }

  // get median from the samples
  // @note We are collecting the samples in a separate array to avoid updating the original
  sensor_info->stats.median = get_median(sensor_samples);

  // get avg
  if(number_of_valid_samples != 0)
  {
    sensor_info->stats.avg = total / (double)number_of_valid_samples;
  }
  else
  {
    sensor_info->status = SENSOR_DATA_INVALID;
  }
}
/*-----------------------------------------------------------*/

SKID_iot_status_t GetCcuData(sequence_state_t last_skid_state)
{
  static SKID_iot_status_t tmp;

  xSemaphoreTake(xCcuDataRWMutex, MUTEX_MAX_BLOCKING_TIME);
  
  // First the items from the latest index
  uint8_t ucIndex = prvGetLatestIndex(true);
  tmp.error_flag = FLAG_UNSET;
  tmp.halt_flag = xCcuTelemetry[ucIndex].xWorkingFlags.Bits.uHalted;
  tmp.reset_flag = FLAG_UNSET;
  
  tmp.skid_state = skid_status[ucIndex].skid_state;

  // Hack - Check if alert should be sent or not
  tmp.send_alert = false;
  if((tmp.skid_state == Lock_State && last_skid_state != Lock_State)
     || (tmp.skid_state == Safe_State && last_skid_state != Safe_State))
  {
    tmp.send_alert = true;
  }

  tmp.two_way_gas_valve_before_water_trap = (skid_status[ucIndex].outputs_status & 0x0001) ? ONE:ZERO;
  tmp.two_way_gas_valve_in_water_trap = (skid_status[ucIndex].outputs_status & 0x0002) ? ONE:ZERO;
  tmp.two_way_gas_valve_after_water_trap = (skid_status[ucIndex].outputs_status & 0x0004) ? ONE:ZERO;
  tmp.vacuum_release_valve_in_water_trap = (skid_status[ucIndex].outputs_status & 0x0008) ? ONE:ZERO;
  tmp.three_way_vacuum_release_valve_before_condenser = (skid_status[ucIndex].outputs_status & 0x0010) ? ONE:ZERO;
  tmp.three_valve_after_vacuum_pump = (skid_status[ucIndex].outputs_status & 0x0020) ? ONE:ZERO;
  tmp.compressor = (skid_status[ucIndex].outputs_status & 0x0040) ? ONE:ZERO;
  tmp.vacuum_pump = (skid_status[ucIndex].outputs_status & 0x0080) ? ONE:ZERO;
  tmp.condenser = (skid_status[ucIndex].outputs_status & 0x0100) ? ONE:ZERO;

  // The transformed ones (Avg, Max, Min and Median)
  sensor_average_max_min(SKID_O2, 0, &tmp.o2_sensor);
  sensor_average_max_min(SKID_MASS_FLOW, 0, &tmp.mass_flow);
  sensor_average_max_min(SKID_CO2, 0, &tmp.co2_sensor);
  sensor_average_max_min(TANK_PRESSURE, 0, &tmp.tank_pressure);
  sensor_average_max_min(SKID_PROPOTIONAL_VALVE_SENSOR, 0, &tmp.proportional_valve_pressure);
  sensor_average_max_min(SKID_TEMPERATURE, 0, &tmp.temperature);
  sensor_average_max_min(SKID_HUMIDITY, 0, &tmp.humidity);

  tmp.errors = skid_status[ucIndex].errors;

  xSemaphoreGive(xCcuDataRWMutex);

  return tmp;
}
/*-----------------------------------------------------------*/

UNIT_iot_status_t get_unit_status(sequence_state_t last_unit_state)
{
  static UNIT_iot_status_t tmp;

  xSemaphoreTake(xCmuDataRWMutex, MUTEX_MAX_BLOCKING_TIME);
  
  // First the items from the latest index
  uint8_t index = prvGetLatestIndex(false);
  tmp.error_flag = (unit_status[index].unit_status & 0x01) ? FLAG_SET:FLAG_UNSET;
  tmp.halt_flag = (unit_status[index].unit_status & 0x02) ? FLAG_SET:FLAG_UNSET;
  tmp.reset_flag = (unit_status[index].unit_status & 0x04) ? FLAG_SET:FLAG_UNSET;
  tmp.just_started_flag = (unit_status[index].unit_status & 0x08) ? FLAG_SET:FLAG_UNSET;
  tmp.setup_state_synching_flag = (unit_status[index].unit_status & 0x10) ? FLAG_SET:FLAG_UNSET;

  tmp.unit_state = unit_status[index].unit_state;

  // Hack - Check if alert should be sent or not
  tmp.send_alert = false;
  if((tmp.unit_state == Lock_State && last_unit_state != Lock_State)
     || (tmp.unit_state == Safe_State && last_unit_state != Safe_State))
  {
    tmp.send_alert = true;
  }

  for(uint8_t i=0;i<NUMBER_OF_HEATERS;++i)
  {
    tmp.heater_info[i].status = (unit_status[index].heater_status & (1 << i)) ? ONE:ZERO;

    // The transformed ones (Avg, Max and Min)
    sensor_average_max_min(UNIT_HEATER, i, &tmp.heater_info[i]);
  }

  tmp.fan_status = (unit_status[index].valve_status & 0x01) ? ONE:ZERO;
  tmp.butterfly_valve_1_status = (unit_status[index].valve_status & 0x02) ? ONE:ZERO;
  tmp.butterfly_valve_2_status = (unit_status[index].valve_status & 0x04) ? ONE:ZERO;

  // The transformed ones (Avg, Max, Min and Median)
  sensor_average_max_min(UNIT_VACUUM_SENSOR, 0, &tmp.vacuum_sensor);
  sensor_average_max_min(UNIT_AMBIENT_HUMIDITY, 0, &tmp.ambient_humidity);
  sensor_average_max_min(UNIT_AMBIENT_TEMPERATURE, 0, &tmp.ambient_temperature);

  tmp.errors = unit_status[index].errors;

  xSemaphoreGive(xCmuDataRWMutex);

  return tmp;
}
/*-----------------------------------------------------------*/

#endif

void stringifyErrorCode(char* error_stringified, ErrorCodes_t code)
{
    switch (code)
    {
        case NO_ERROR:            
            sprintf(error_stringified, "%s", "NO_ERROR");
            break;
        case VACUUM_SENSOR_FAULT:
            sprintf(error_stringified, "%s", "VACUUM_SENSOR_FAULT");
            break;
        case TANK_PRESSURE_SENSOR_FAULT:
            sprintf(error_stringified, "%s", "TANK_PRESSURE_SENSOR_FAULT");
            break;
        case TANK_PRESSURE_SENSOR_OUT_OF_BOUNDARY:
            sprintf(error_stringified, "%s", "TANK_PRESSURE_SENSOR_OUT_OF_BOUNDARY");
            break;
        case TANK_PRESSURE_FAULT:
            sprintf(error_stringified, "%s", "TANK_PRESSURE_FAULT");
            break;
        case PROPORTIONAL_VALVE_PRESSURE_SENSOR_FAULT:
            sprintf(error_stringified, "%s", "PROPORTIONAL_VALVE_PRESSURE_SENSOR_FAULT");
            break;
        case PROPORTIONAL_VALVE_PRESSURE_OUT_OF_BOUNDARY:
            sprintf(error_stringified, "%s", "PROPORTIONAL_VALVE_PRESSURE_OUT_OF_BOUNDARY");
            break;
        case PROPORTIONAL_VALVE_PRESSURE_CRITICAL_STATE_1:
            sprintf(error_stringified, "%s", "PROPORTIONAL_VALVE_PRESSURE_CRITICAL_STATE_1");
            break;
        case PROPORTIONAL_VALVE_PRESSURE_CRITICAL_STATE_2:
            sprintf(error_stringified, "%s", "PROPORTIONAL_VALVE_PRESSURE_CRITICAL_STATE_2");
            break;
        case VACUUM_CHAMBER_SENSOR_TIMEOUT_ERROR:
            sprintf(error_stringified, "%s", "VACUUM_CHAMBER_SENSOR_TIMEOUT_ERROR");
            break;
        default:
            sprintf(error_stringified, "%s%d", "UNKNOWN_CODE: ", code);
            break;
    }
}
/*-----------------------------------------------------------*/

void send_lock_status(void){
    static uint8_t data_to_send[MESSAGE_LENGTH_IOT_COMMAND] = {'C', 0, 'L', 0, 'L', 0x88};
    gui_comm_send_data(data_to_send, MESSAGE_LENGTH_IOT_COMMAND, NULL, NULL, NULL);
}
/*-----------------------------------------------------------*/

void send_unlock_status(void){
    static uint8_t data_to_send[MESSAGE_LENGTH_IOT_COMMAND] = {'C', 0, 'L', 0, 'U', 0x34};
    gui_comm_send_data(data_to_send, MESSAGE_LENGTH_IOT_COMMAND, NULL, NULL, NULL);
}
/*-----------------------------------------------------------*/

/**
 * @brief CCU communication task.
 * 
 * Responsible for establishing and maintaining the connection
 * to Azure IoT Hub. It also handles reconnection logic in case
 * of connection loss.
 */
void vControllerCommunicationTask( void )
{
    xTaskCreate( prvControllerCommunicationTask,    /* Function that implements the task. */
                 "controller_communication_task",   /* Text name for the task - only used for debugging. */
                 256,                               /* Size of stack (in words, not bytes) to allocate for the task. */
                 NULL,                              /* Task parameter - not used in this case. */
                 tskIDLE_PRIORITY,                  /* Task priority, must be between 0 and configMAX_PRIORITIES - 1. */
                 NULL );                            /* Used to pass out a handle to the created task - not used in this case. */
}
/*-----------------------------------------------------------*/






#if 0
static uint8_t CalcCrc(uint8_t data[], uint8_t nbrOfBytes)
{
  uint8_t bit;        // bit mask
  uint8_t crc = 0xFF; // calculated checksum
  uint8_t byteCtr;    // byte counter
  
  // calculates 8-Bit checksum with given polynomial
  for(byteCtr = 0; byteCtr < nbrOfBytes; byteCtr++)
  {
    crc ^= data[byteCtr];
    for(bit = 8; bit > 0; --bit)
    {
      if(crc & 0x80)
      {
        crc = (crc << 1) ^ CRC_POLYNOMIAL;
      }
      else
      {
        crc = (crc << 1);
      }
    }
  }
  
  return crc;
}
/*-----------------------------------------------------------*/

static void read_unit_status(uint8_t incoming_data[])
{
  static uint8_t end = 4;

  xSemaphoreTake(xCmuDataRWMutex, MUTEX_MAX_BLOCKING_TIME);

  end = 4;

  unit_status[xCmuCircularBufferIndex].unit_status = incoming_data[end++];

  unit_status[xCmuCircularBufferIndex].unit_state = incoming_data[end++];

  unit_status[xCmuCircularBufferIndex].heater_status = (((uint16_t)(incoming_data[end++]) << 8) & 0xFF00);
  unit_status[xCmuCircularBufferIndex].heater_status |= ((uint16_t)(incoming_data[end++]) & 0xFF);

  uint32_t tmp_sensor = 0;
  for(int i = 0;i<NUMBER_OF_HEATERS;i++)
  {
    tmp_sensor = 0;
    tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
    tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
    tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
    tmp_sensor |= (uint32_t)(incoming_data[end++]) & 0xFF;
    unit_status[xCmuCircularBufferIndex].heater_temperatures[i] = (double)tmp_sensor / (1 << 16);
  }

  unit_status[xCmuCircularBufferIndex].valve_status = incoming_data[end++];

  int32_t tmp_sensor_1 = 0;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor_1 |= (int32_t)(incoming_data[end++]) & 0xFF;
  unit_status[xCmuCircularBufferIndex].vacuum_sensor = (double)tmp_sensor_1 / (1 << 16);

  tmp_sensor = 0;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor |= (uint32_t)(incoming_data[end++]) & 0xFF;
  unit_status[xCmuCircularBufferIndex].ambient_humidity = (double)tmp_sensor / (1 << 16);

  tmp_sensor_1 = 0;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor_1 |= (int32_t)(incoming_data[end++]) & 0xFF;
  unit_status[xCmuCircularBufferIndex].ambient_temperature = (double)tmp_sensor_1 / (1 << 16);

  unit_status[xCmuCircularBufferIndex].errors = 0;
  unit_status[xCmuCircularBufferIndex].errors |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  unit_status[xCmuCircularBufferIndex].errors |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  unit_status[xCmuCircularBufferIndex].errors |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  unit_status[xCmuCircularBufferIndex].errors |= (uint32_t)(incoming_data[end++]) & 0xFF;

  // Update the circular index for next turn once current index is filled
  xCmuCircularBufferIndex++;
  if(xCmuCircularBufferIndex >= NUMBER_OF_SAMPLES)
  {
    xCmuCircularBufferIndex = 0;
  }

  xSemaphoreGive(xCmuDataRWMutex);
}
/*-----------------------------------------------------------*/

/**
 * @brief Read incoming data from CCU
 */
static void read_incoming_system_data(void)
{
  static uint8_t incoming_data[64] = {0};
  static uint8_t state = 0;

  switch(state){
    case 0:
      if(gui_comm_check_for_received_data(incoming_data, 1) != FAILED)
      {
        //configPRINTF(("incoming_data[0] = %c\r\n", (char)(incoming_data[0])));
        if(incoming_data[0] == 'D')
        {
          state = 1;
          // configPRINTF( ( "---------RECEIVED D---------\r\n" ) );
        }
      }
      break;
    case 1:
      if(gui_comm_check_for_received_data((incoming_data + 1), 3) != FAILED)
      {
        //configPRINTF(("incoming_data[0] = %c\r\n", (char)(incoming_data[2])));
        if((incoming_data[2] == 'U') || (incoming_data[2] == 'S'))
        {
          state = 2;
          // configPRINTF( ( "---------RECEIVED U or S---------\r\n" ) );
        }
        else
        {
          state = 3;
        }
      }
      break;
    case 2: // Read data
      switch(incoming_data[2])
      {
        case 'U': // Unit data
          //Read rest data from uart
          if(gui_comm_check_for_received_data((incoming_data + 4), MESSAGE_LENGTH_UNIT_STATUS - 4) != FAILED)
          {
              //configPRINTF(("CRC: %X\n\r\n", CalcCrc(incoming_data, MESSAGE_LENGTH_SKID_STATUS - 1)));
              if(CalcCrc(incoming_data, MESSAGE_LENGTH_UNIT_STATUS - 1) == incoming_data[MESSAGE_LENGTH_UNIT_STATUS - 1])
              {
                  read_unit_status(incoming_data);
              }
              state = 3;
          }
          break;
        case 'S': // SKID data
          //Read rest data from uart
          if(gui_comm_check_for_received_data((incoming_data + 4), MESSAGE_LENGTH_SKID_STATUS - 4) != FAILED)
          {
              if(CalcCrc(incoming_data, MESSAGE_LENGTH_SKID_STATUS - 1) == incoming_data[MESSAGE_LENGTH_SKID_STATUS - 1])
              {
                  read_skid_status(incoming_data);
              }
              state = 3;
          }
          break;
      }
      break;
    case 3: //Reset state 
      state = 0;
      memset(incoming_data, 0, sizeof(incoming_data));
      break;
  }
}
/*-----------------------------------------------------------*/

static void read_skid_status(uint8_t incoming_data[])
{
  static uint8_t end = 4;

  xSemaphoreTake(xCcuDataRWMutex, MUTEX_MAX_BLOCKING_TIME);

  end = 4;

  skid_status[xCcuCircularBufferIndex].skid_status = incoming_data[end++];
  skid_status[xCcuCircularBufferIndex].skid_state = incoming_data[end++];

  skid_status[xCcuCircularBufferIndex].outputs_status = (((uint16_t)(incoming_data[end++]) << 8) & 0xFF00); 
  skid_status[xCcuCircularBufferIndex].outputs_status |= ((uint16_t)(incoming_data[end++]) & 0xFF);

  uint32_t tmp_sensor = 0;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor |= (uint32_t)(incoming_data[end++]) & 0xFF;
  skid_status[xCcuCircularBufferIndex].o2_sensor = (double)tmp_sensor / (1 << 16);

  int32_t tmp_sensor_1 = 0;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor_1 |= (int32_t)(incoming_data[end++]) & 0xFF;
  skid_status[xCcuCircularBufferIndex].mass_flow = (double)tmp_sensor_1 / (1 << 16);

  tmp_sensor_1 = 0;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor_1 |= (int32_t)(incoming_data[end++]) & 0xFF;
  skid_status[xCcuCircularBufferIndex].co2_sensor = (double)tmp_sensor_1 / (1 << 16);

  tmp_sensor = 0;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor |= (uint32_t)(incoming_data[end++]) & 0xFF;
  skid_status[xCcuCircularBufferIndex].tank_pressure = (double)tmp_sensor / (1 << 16);

  tmp_sensor_1 = 0;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor_1 |= (int32_t)(incoming_data[end++]) & 0xFF;
  skid_status[xCcuCircularBufferIndex].proportional_valve_pressure = (double)tmp_sensor_1 / (1 << 16);

  tmp_sensor_1 = 0;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor_1 |= (int32_t)(incoming_data[end++]) & 0xFF;
  skid_status[xCcuCircularBufferIndex].temperature = (double)tmp_sensor_1 / (1 << 16);

  tmp_sensor = 0;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor |= (uint32_t)(incoming_data[end++]) & 0xFF;
  skid_status[xCcuCircularBufferIndex].humidity = (double)tmp_sensor / (1 << 16);

  skid_status[xCcuCircularBufferIndex].errors = 0;
  skid_status[xCcuCircularBufferIndex].errors |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  skid_status[xCcuCircularBufferIndex].errors |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  skid_status[xCcuCircularBufferIndex].errors |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  skid_status[xCcuCircularBufferIndex].errors |= (uint32_t)(incoming_data[end++]) & 0xFF;

  // Update the circular index for next turn once current index is filled
  xCcuCircularBufferIndex++;
  if(xCcuCircularBufferIndex >= NUMBER_OF_SAMPLES)
  {
    xCcuCircularBufferIndex = 0;
  }

  xSemaphoreGive(xCcuDataRWMutex);
}
#endif
/*-----------------------------------------------------------*/