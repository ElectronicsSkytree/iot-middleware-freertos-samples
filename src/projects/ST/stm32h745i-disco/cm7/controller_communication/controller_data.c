/* Copyright (c) Skytree B.V. All rights reserved. */

/* Local includes */
#include "controller_data.h"
#include "config.h"

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
#define MUTEX_MAX_BLOCKING_TIME 1000
#define NUMBER_OF_SAMPLES 30  // 30 samples of each 1s interval aggregrate before pushing to backend

//========================================================================================================== VARIABLES
UNIT_status_t unit_status[NUMBER_OF_SAMPLES] = {0};
SKID_status_t skid_status[NUMBER_OF_SAMPLES] = {0};

// We receive unit and skid separately so need to manage separate indexes
// Not good but that is how it is!!
static uint8_t unit_circular_buffer_index = 0;
static uint8_t skid_circular_buffer_index = 0;

//------------------------------------------ mutexes for read write operations on unit/skid status data structs
SemaphoreHandle_t skid_status_rw_mutex;
StaticSemaphore_t skid_mutex_buffer;

SemaphoreHandle_t unit_status_rw_mutex;
StaticSemaphore_t unit_mutex_buffer;

//========================================================================================================== FUNCTIONS DECLARATIONS
static void system_data_init(void);
static uint8_t CalcCrc(uint8_t data[], uint8_t nbrOfBytes);
static void read_incoming_system_data(void);
static void prvControllerCommunicationTask( void* arg );
static void read_unit_status(uint8_t incoming_data[]);
static void read_skid_status(uint8_t incoming_data[]);

static uint8_t get_latest_index(bool skid);
static void sensor_average_max_min(sensor_name_t name, uint8_t heater_index, sensor_info_t* sensor_info);
static uint8_t get_latest_index(bool skid);
static double get_sensor_value(uint8_t sample_index, sensor_name_t name, uint8_t heater_index);
//========================================================================================================== FUNCTIONS DEFINITIONS

/**
 * @brief system initialization
*/
static void system_data_init(void)
{
  skid_status_rw_mutex = xSemaphoreCreateMutexStatic(&skid_mutex_buffer);
  unit_status_rw_mutex = xSemaphoreCreateMutexStatic(&unit_mutex_buffer);

  gui_comm_init();
}
/*-----------------------------------------------------------*/

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
        // configPRINTF(("incoming_data[0] = %c\r\n", (char)(incoming_data[0])));
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
              // configPRINTF(("CRC: %X\n\r\n", CalcCrc(incoming_data, MESSAGE_LENGTH_SKID_STATUS - 1)));
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
              // configPRINTF(("CRC: %X\n\r\n", CalcCrc(incoming_data, MESSAGE_LENGTH_SKID_STATUS - 1)));
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

/**
 * @brief Controller communication task loop
 */
static void prvControllerCommunicationTask( void* arg )
{
    ( void ) arg;

    system_data_init();

    while( 1 )
    {
        read_incoming_system_data();

        // Experimenting
        taskYIELD();
    }
}
/*-----------------------------------------------------------*/

static void read_unit_status(uint8_t incoming_data[])
{
  static uint8_t end = 4;

  xSemaphoreTake(unit_status_rw_mutex, MUTEX_MAX_BLOCKING_TIME);

  end = 4;

  unit_status[unit_circular_buffer_index].unit_status = incoming_data[end++];

  unit_status[unit_circular_buffer_index].unit_state = incoming_data[end++];

  unit_status[unit_circular_buffer_index].heater_status = (((uint16_t)(incoming_data[end++]) << 8) & 0xFF00);
  unit_status[unit_circular_buffer_index].heater_status |= ((uint16_t)(incoming_data[end++]) & 0xFF);

  uint32_t tmp_sensor = 0;
  for(int i = 0;i<NUMBER_OF_HEATERS;i++)
  {
    tmp_sensor = 0;
    tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
    tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
    tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
    tmp_sensor |= (uint32_t)(incoming_data[end++]) & 0xFF;
    unit_status[unit_circular_buffer_index].heater_temperatures[i] = (double)tmp_sensor / (1 << 16);
  }

  unit_status[unit_circular_buffer_index].valve_status = incoming_data[end++];

  int32_t tmp_sensor_1 = 0;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor_1 |= (int32_t)(incoming_data[end++]) & 0xFF;
  unit_status[unit_circular_buffer_index].vacuum_sensor = (double)tmp_sensor_1 / (1 << 16);

  tmp_sensor = 0;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor |= (uint32_t)(incoming_data[end++]) & 0xFF;
  unit_status[unit_circular_buffer_index].ambient_humidity = (double)tmp_sensor / (1 << 16);

  tmp_sensor_1 = 0;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor_1 |= (int32_t)(incoming_data[end++]) & 0xFF;
  unit_status[unit_circular_buffer_index].ambient_temperature = (double)tmp_sensor_1 / (1 << 16);

  unit_status[unit_circular_buffer_index].errors = 0;
  unit_status[unit_circular_buffer_index].errors |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  unit_status[unit_circular_buffer_index].errors |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  unit_status[unit_circular_buffer_index].errors |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  unit_status[unit_circular_buffer_index].errors |= (uint32_t)(incoming_data[end++]) & 0xFF;

  // Update the circular index for next turn once current index is filled
  unit_circular_buffer_index++;
  if(unit_circular_buffer_index >= NUMBER_OF_SAMPLES)
  {
    unit_circular_buffer_index = 0;
  }

  xSemaphoreGive(unit_status_rw_mutex);
}
/*-----------------------------------------------------------*/

static void read_skid_status(uint8_t incoming_data[])
{
  static uint8_t end = 4;

  xSemaphoreTake(skid_status_rw_mutex, MUTEX_MAX_BLOCKING_TIME);

  end = 4;

  skid_status[skid_circular_buffer_index].skid_status = incoming_data[end++];
  skid_status[skid_circular_buffer_index].skid_state = incoming_data[end++];

  skid_status[skid_circular_buffer_index].outputs_status = (((uint16_t)(incoming_data[end++]) << 8) & 0xFF00); 
  skid_status[skid_circular_buffer_index].outputs_status |= ((uint16_t)(incoming_data[end++]) & 0xFF);

  uint32_t tmp_sensor = 0;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor |= (uint32_t)(incoming_data[end++]) & 0xFF;
  skid_status[skid_circular_buffer_index].o2_sensor = (double)tmp_sensor / (1 << 16);

  int32_t tmp_sensor_1 = 0;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor_1 |= (int32_t)(incoming_data[end++]) & 0xFF;
  skid_status[skid_circular_buffer_index].mass_flow = (double)tmp_sensor_1 / (1 << 16);

  tmp_sensor_1 = 0;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor_1 |= (int32_t)(incoming_data[end++]) & 0xFF;
  skid_status[skid_circular_buffer_index].co2_sensor = (double)tmp_sensor_1 / (1 << 16);

  tmp_sensor = 0;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor |= (uint32_t)(incoming_data[end++]) & 0xFF;
  skid_status[skid_circular_buffer_index].tank_pressure = (double)tmp_sensor / (1 << 16);

  tmp_sensor_1 = 0;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor_1 |= (int32_t)(incoming_data[end++]) & 0xFF;
  skid_status[skid_circular_buffer_index].proportional_valve_pressure = (double)tmp_sensor_1 / (1 << 16);

  tmp_sensor_1 = 0;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor_1 |= (int32_t)(incoming_data[end++]) & 0xFF;
  skid_status[skid_circular_buffer_index].temperature = (double)tmp_sensor_1 / (1 << 16);

  tmp_sensor = 0;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor |= (uint32_t)(incoming_data[end++]) & 0xFF;
  skid_status[skid_circular_buffer_index].humidity = (double)tmp_sensor / (1 << 16);

  skid_status[skid_circular_buffer_index].errors = 0;
  skid_status[skid_circular_buffer_index].errors |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  skid_status[skid_circular_buffer_index].errors |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  skid_status[skid_circular_buffer_index].errors |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  skid_status[skid_circular_buffer_index].errors |= (uint32_t)(incoming_data[end++]) & 0xFF;

  // Update the circular index for next turn once current index is filled
  skid_circular_buffer_index++;
  if(skid_circular_buffer_index >= NUMBER_OF_SAMPLES)
  {
    skid_circular_buffer_index = 0;
  }

  xSemaphoreGive(skid_status_rw_mutex);
}
/*-----------------------------------------------------------*/

static uint8_t get_latest_index(bool skid)
{
  uint8_t index = unit_circular_buffer_index;
  if (skid)
  {
    index = skid_circular_buffer_index;
  }

  if(index != 0)
  {
    // We do one back as the indexes are always jumped to next after updating
    index--;
  }
  else
  {
    index = NUMBER_OF_SAMPLES - 1;
  }

  return index;
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

static double get_median(double sensor_samples[], uint32_t number_of_valid_samples)
{
  qsort(sensor_samples, (size_t) number_of_valid_samples, sizeof(double), compare_for_sort);

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

static bool update_sensor_total(sensor_name_t name, double sensor_value, double* total, uint8_t* number_of_valid_samples)
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
      // double integer_part = 0;
      // modf(sensor_value, &integer_part);
      // LogDebug( ( "Invalid sensor (%d) value integer-part (%d)" , name, (int32_t)integer_part) );
      return false;
    }
    else
    {
      *total += sensor_value;
      *number_of_valid_samples += 1;
      return true;
    }
}
/*-----------------------------------------------------------*/

static void sensor_average_max_min(sensor_name_t name, uint8_t heater_index, sensor_info_t* sensor_info)
{
  double total = 0.0;
  uint32_t median_sample_index = 0;
  double sensor_samples[NUMBER_OF_SAMPLES];
  double sensor_value = 0.0;
  uint8_t number_of_valid_samples = 0;
  sensor_info->stats.avg = sensor_info->stats.max = sensor_info->stats.min = sensor_info->stats.median = 0.0; // Just to be sure everything is set to zero

  sensor_samples[0] = sensor_value = get_sensor_value(0, name, heater_index);
  if( update_sensor_total(name, sensor_value, &total, &number_of_valid_samples) )
  {
      // @note We are collecting the samples in a separate array to avoid updating the original
      sensor_samples[median_sample_index++] = sensor_value;
      sensor_info->stats.max = sensor_info->stats.min = sensor_info->stats.median = sensor_value;
  }

  for(uint8_t i=1;i<NUMBER_OF_SAMPLES;++i)
  {
    sensor_value = get_sensor_value(i, name, heater_index);

    if( update_sensor_total(name, sensor_value, &total, &number_of_valid_samples) )
    {
      // @note We are collecting the samples in a separate array to avoid updating the original
        sensor_samples[median_sample_index++] = sensor_value;

        if(sensor_value > sensor_info->stats.max)
        {
          sensor_info->stats.max = sensor_value;
        }
        if(sensor_value < sensor_info->stats.min)
        {
          sensor_info->stats.min = sensor_value;
        }
    }
  }  

  // get avg
  if(number_of_valid_samples != 0 && total > 0)
  {
      sensor_info->stats.avg = total / (double)number_of_valid_samples;
      // get median from the samples
      sensor_info->stats.median = get_median(sensor_samples, number_of_valid_samples);
  }
  else
  {
      sensor_info->status = SENSOR_DATA_INVALID;
  }
}
/*-----------------------------------------------------------*/

SKID_iot_status_t get_skid_status(sequence_state_t last_skid_state)
{
  static SKID_iot_status_t tmp;

  xSemaphoreTake(skid_status_rw_mutex, MUTEX_MAX_BLOCKING_TIME);
  
  // First the items from the latest index
  uint8_t index = get_latest_index(true);
  tmp.error_flag = (skid_status[index].skid_status & 0x01) ? FLAG_SET:FLAG_UNSET;
  tmp.halt_flag = (skid_status[index].skid_status & 0x02) ? FLAG_SET:FLAG_UNSET;
  tmp.reset_flag = (skid_status[index].skid_status & 0x04) ? FLAG_SET:FLAG_UNSET;
  
  tmp.skid_state = skid_status[index].skid_state;

  // Hack - Check if alert should be sent or not
  tmp.send_alert = false;
  if((tmp.skid_state == Lock_State && last_skid_state != Lock_State)
     || (tmp.skid_state == Safe_State && last_skid_state != Safe_State))
  {
    tmp.send_alert = true;
  }

  tmp.two_way_gas_valve_before_water_trap = (skid_status[index].outputs_status & 0x0001) ? ONE:ZERO;
  tmp.two_way_gas_valve_in_water_trap = (skid_status[index].outputs_status & 0x0002) ? ONE:ZERO;
  tmp.two_way_gas_valve_after_water_trap = (skid_status[index].outputs_status & 0x0004) ? ONE:ZERO;
  tmp.vacuum_release_valve_in_water_trap = (skid_status[index].outputs_status & 0x0008) ? ONE:ZERO;
  tmp.three_way_vacuum_release_valve_before_condenser = (skid_status[index].outputs_status & 0x0010) ? ONE:ZERO;
  tmp.three_valve_after_vacuum_pump = (skid_status[index].outputs_status & 0x0020) ? ONE:ZERO;
  tmp.compressor = (skid_status[index].outputs_status & 0x0040) ? ONE:ZERO;
  tmp.vacuum_pump = (skid_status[index].outputs_status & 0x0080) ? ONE:ZERO;
  tmp.condenser = (skid_status[index].outputs_status & 0x0100) ? ONE:ZERO;

  // The transformed ones (Avg, Max, Min and Median)
  sensor_average_max_min(SKID_O2, 0, &tmp.o2_sensor);
  sensor_average_max_min(SKID_MASS_FLOW, 0, &tmp.mass_flow);
  sensor_average_max_min(SKID_CO2, 0, &tmp.co2_sensor);
  sensor_average_max_min(TANK_PRESSURE, 0, &tmp.tank_pressure);
  sensor_average_max_min(SKID_PROPOTIONAL_VALVE_SENSOR, 0, &tmp.proportional_valve_pressure);
  sensor_average_max_min(SKID_TEMPERATURE, 0, &tmp.temperature);
  sensor_average_max_min(SKID_HUMIDITY, 0, &tmp.humidity);

  tmp.errors = skid_status[index].errors;

  xSemaphoreGive(skid_status_rw_mutex);

  return tmp;
}
/*-----------------------------------------------------------*/

UNIT_iot_status_t get_unit_status(sequence_state_t last_unit_state)
{
  static UNIT_iot_status_t tmp;

  xSemaphoreTake(unit_status_rw_mutex, MUTEX_MAX_BLOCKING_TIME);
  
  // First the items from the latest index
  uint8_t index = get_latest_index(false);
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

  xSemaphoreGive(unit_status_rw_mutex);

  return tmp;
}
/*-----------------------------------------------------------*/

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
 * Responsible for RxTx UART data to/from the ccu
 */
void vControllerCommunicationTask( void )
{
    xTaskCreate( prvControllerCommunicationTask,    /* Function that implements the task. */
                 "controller_communication_task",   /* Text name for the task - only used for debugging. */
                 256,                               /* Size of stack (in words, not bytes) to allocate for the task. */
                 NULL,                              /* Task parameter - not used in this case. */
                 tskIDLE_PRIORITY + 1,              /* Task priority, must be between 0 and configMAX_PRIORITIES - 1. */
                 NULL );                            /* Used to pass out a handle to the created task - not used in this case. */
}
/*-----------------------------------------------------------*/
