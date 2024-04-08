/*
 * system_data.h
 *
 *  Created on: November 29, 2023
 *      Author: Maciej Rogalinski
 */

#ifndef SYSTEM_DATA_H_
#define SYSTEM_DATA_H_

#ifdef __cplusplus
 extern "C" {
#endif

//Includes
#include "gui_comm_api.h"
#include <stdio.h>
#include <stdbool.h>

//Defines
#define CRC_POLYNOMIAL 0x42
#define MESSAGE_LENGTH_UNIT_STATUS 62   //UNIT_status 57 bytes + header 4 bytes + crc 1 byte = 62 bytes
#define MESSAGE_LENGTH_SKID_STATUS 41   //SKID_status 36 bytes + header 4 bytes + crc 1 byte = 41 bytes
#define MESSAGE_LENGTH_IOT_COMMAND 6    //IOT_COMMAND header 5 bytes + crc 1 byte = 6 bytes
#define NUMBER_OF_HEATERS 9             // Its a continuous array for now
#define NUMBER_OF_CARTRIDGES 3          // For now we have hardcoded this as skid also has hardcoded 9 heaters
#define NUMBER_OF_ZONES_PER_CARTRIDGE 3 // This is hardcoded as well

#if 0
// Structures in controllino for reference
typedef struct{
	uint8_t unit_status; //Flags//000//setup_state_synching_flag//just_started_flag//reset_flag//halt_flag//error_flag
    uint8_t unit_state;
    uint16_t heater_status;  //Heater//000000//9//8//7//6//5//4//3//2//1
    uint32_t heater_temperatures[9];
    uint8_t valve_status; //00000//Butterfly2//Butterfly1//Fan1
    int32_t vacuum_sensor;
    uint32_t ambient_humidity;
    int32_t ambient_temperature;
    uint32_t errors;
}UNIT_status; //57 bytes

typedef struct{
	uint8_t skid_status; //Flags//0000//reset_flag//halt_flag//error_flag
    uint8_t skid_state;
    uint16_t outputs_status; 
//0000000//CONDENSER//VACUUM_PUMP//COMPRESSOR//THREE_WAY_VALVE_AFTER_VACUUM_PUMP//THREE_WAY_VACUUM_RELEASE_VALVE_BEFORE_CONDENSATOR//VACUUM_RELEASE_VALVE_IN_WT//TWO_WAY_GAS_VALVE_AFTER_WT//TWO_WAY_WATER_OUTLET_VALVE_IN_WT//TWO_WAY_GAS_VALVE_BEFORE_WT
    uint32_t o2_sensor;
    int32_t mass_flow;
    int32_t co2_sensor;
    uint32_t tank_pressure;
    int32_t proportional_valve_pressure;
    int32_t temperature;
    uint32_t humidity;
    uint32_t errors;
}SKID_status; //36 bytes
#endif

typedef struct{
    uint8_t unit_status; //Flags//000//setup_state_synching_flag//just_started_flag//reset_flag//halt_flag//error_flag
    uint8_t unit_state;
    uint16_t heater_status;  //Heater//000000//9//8//7//6//5//4//3//2//1
    double heater_temperatures[NUMBER_OF_HEATERS];
    uint8_t valve_status; //00000//Butterfly2//Butterfly1//Fan1
    double vacuum_sensor;
    double ambient_humidity;
    double ambient_temperature;
    uint32_t errors;
}UNIT_status_t;

typedef struct{
    uint8_t skid_status; //Flags//0000//reset_flag//halt_flag//error_flag
    uint8_t skid_state;
    uint16_t outputs_status;
//0000000//CONDENSER//VACUUM_PUMP//COMPRESSOR//THREE_WAY_VALVE_AFTER_VACUUM_PUMP//THREE_WAY_VACUUM_RELEASE_VALVE_BEFORE_CONDENSATOR//VACUUM_RELEASE_VALVE_IN_WT//TWO_WAY_GAS_VALVE_AFTER_WT//TWO_WAY_WATER_OUTLET_VALVE_IN_WT//TWO_WAY_GAS_VALVE_BEFORE_WT
    double o2_sensor;
    double mass_flow;
    double co2_sensor;
    double tank_pressure;
    double proportional_valve_pressure;
    double temperature;
    double humidity;
    uint32_t errors;
}SKID_status_t;

// Data structures that the sample iot runner will receive on get calls
// Keeping things simple now, we will anyway ditch this and have SDK on MEGA
typedef struct{
    double avg;
    double max;
    double min;
    double median;
}sensor_stats_t;

typedef enum{
    ZERO,
    ONE
}component_status_t;
typedef struct{
    component_status_t status;
    sensor_stats_t stats; 
}sensor_info_t;

const char* valve_status_stringified[] = {
    "CLOSED",        // 0
    "OPENED"         // 1
};

const char* three_way_valve_stringified[] = {
    "to_Tank",        // 0
    "to_Air"          // 1
};

const char* component_status_stringified[] = {
    "OFF",      // 0
    "ON"        // 1
};

const char* sensor_status_stringified[] = {
    "NO-ERROR",    // 0
    "ERROR"        // 1
};

typedef enum{
    FLAG_UNSET,
    FLAG_SET
}flag_state_t;

const char* flag_state_stringified[] = {
    "UNSET",   // 0
    "SET"      // 1
};

// Enum for unit/skid high level state
typedef enum{
    Error_Handling = 0,
    Init_State = 1,
    Adsorb_State = 2,
    Evacuation_State = 3,
    Desorb_State = 4,
    Vacuum_Release_State = 5,
    Lock_State = 6,
    Desorb_Setup_State = 7,
    Safe_State = 8,
    Unlock_State = 9
}sequence_state_t;

const char* sequence_state_stringified[] = {
    "Error_Handling",         // 0
    "Init_State",             // 1
    "Adsorb_State",           // 2
    "Evacuation_State",       // 3
    "Desorb_State",           // 4
    "Vacuum_Release_State",   // 5
    "Lock_State",             // 6
    "Desorb_Setup_State",     // 7
    "Safe_State",             // 8
    "Unlock_State"            // 9
};

typedef enum{
    SKID_O2,
    SKID_MASS_FLOW,
    SKID_CO2,
    SKID_PROPOTIONAL_VALVE_SENSOR,
    SKID_TEMPERATURE,
    SKID_HUMIDITY,
    UNIT_VACUUM_SENSOR,
    UNIT_AMBIENT_HUMIDITY,
    UNIT_AMBIENT_TEMPERATURE,
    UNIT_HEATER,
    TANK_PRESSURE
}sensor_name_t;

typedef struct{
	// uint8_t unit_status; //Flags//000//setup_state_synching_flag//just_started_flag//reset_flag//halt_flag//error_flag
    flag_state_t error_flag;
    flag_state_t halt_flag;
    flag_state_t reset_flag;
    flag_state_t just_started_flag;
    flag_state_t setup_state_synching_flag;

    sequence_state_t unit_state;

    sensor_info_t heater_info[NUMBER_OF_HEATERS];

    component_status_t fan_status;
    component_status_t butterfly_valve_1_status;
    component_status_t butterfly_valve_2_status;

    sensor_info_t vacuum_sensor;
    sensor_info_t ambient_humidity;
    sensor_info_t ambient_temperature;
    bool send_alert;
    uint32_t errors;
}UNIT_iot_status_t;

typedef struct{
    flag_state_t error_flag;
    flag_state_t halt_flag;
    flag_state_t reset_flag;

    sequence_state_t skid_state;

    component_status_t two_way_gas_valve_before_water_trap;
    component_status_t two_way_gas_valve_in_water_trap;
    component_status_t two_way_gas_valve_after_water_trap;
    component_status_t vacuum_release_valve_in_water_trap;
    component_status_t three_way_vacuum_release_valve_before_condenser;
    component_status_t three_valve_after_vacuum_pump;
    component_status_t compressor;
    component_status_t vacuum_pump;
    component_status_t condenser;

    sensor_info_t o2_sensor;
    sensor_info_t mass_flow;
    sensor_info_t co2_sensor;
    sensor_info_t tank_pressure;    // @todo: Do we move to tank structure?
    sensor_info_t proportional_valve_pressure;
    sensor_info_t temperature;
    sensor_info_t humidity;
    bool send_alert;
    uint32_t errors;
}SKID_iot_status_t;

#if 0   // For reference from Controllino code
// Unit error codes
enum ErrorCodes
{
    NO_ERROR = 0,
    /* Errors 1 - 99 reserved for unit errors*/
    /* Errors 1 - 19 reserved for heater over temperature */
    /* Errors 11 - 29 reserved for heater under heating / disconnection */
    VACUUM_SENSOR_FAULT                                 = 90               // Sensor disconnected
};

// SKID error codes
enum ErrorCodes
{
    NO_ERROR = 0,
    /* Errors 1 - 99 reserved for unit errors*/
    TANK_PRESSURE_SENSOR_FAULT                          = 100,              // Sensor disconnected
    TANK_PRESSURE_SENSOR_OUT_OF_BOUNDARY                = 101,              // Sensor value out of bound
    TANK_PRESSURE_FAULT                                 = 102,              // Pressure is not what expected
    /* 100 - 105 reserved for Tank Pressure Sensor errors */
    PROPORTIONAL_VALVE_PRESSURE_SENSOR_FAULT            = 106,              // Proportional valve sensor is disconnected
    PROPORTIONAL_VALVE_PRESSURE_OUT_OF_BOUNDARY         = 107,              // Proportional valve pressure value is out of bound
    PROPORTIONAL_VALVE_PRESSURE_CRITICAL_STATE_1        = 108,              // Proportional valve pressure above 1.1 bar for more than 15 consecutive seconds
    PROPORTIONAL_VALVE_PRESSURE_CRITICAL_STATE_2        = 109,              // Proportional valve pressure above 1.5 bar for more than 2 consecutive seconds
    /* 106 - 110 reserved for Proportional Valve Pressure errors */
    VACUUM_CHAMBER_SENSOR_TIMEOUT_ERROR                 = 111,              // Vacuum sensor didn't reach 20 mBar before timeout
    /* 10000 - 20000 reserved for warnings*/
    WARNING_MASS_FLOW_SENSOR_FAULT                      = 10001,            // Mass flow sensor is disconnected or I2C has problem
    WARNING_O2_SENSOR_FAULT                             = 10002,            // O2 sensor is disconnected or I2C has problem
    WARNING_CO2_SENSOR_FAULT                            = 10003,            // CO2 sensor is disconnected
};
#endif

typedef enum{
    NO_ERROR = 0,
    /* Unit errors: Errors 1 - 99 reserved for unit errors*/
    VACUUM_SENSOR_FAULT                                 = 90,               // Sensor disconnected
    /* Skid errors */
    TANK_PRESSURE_SENSOR_FAULT                          = 100,              // Sensor disconnected
    TANK_PRESSURE_SENSOR_OUT_OF_BOUNDARY                = 101,              // Sensor value out of bound
    TANK_PRESSURE_FAULT                                 = 102,              // Pressure is not what expected
    /* 100 - 105 reserved for Tank Pressure Sensor errors */
    PROPORTIONAL_VALVE_PRESSURE_SENSOR_FAULT            = 106,              // Proportional valve sensor is disconnected
    PROPORTIONAL_VALVE_PRESSURE_OUT_OF_BOUNDARY         = 107,              // Proportional valve pressure value is out of bound
    PROPORTIONAL_VALVE_PRESSURE_CRITICAL_STATE_1        = 108,              // Proportional valve pressure above 1.1 bar for more than 15 consecutive seconds
    PROPORTIONAL_VALVE_PRESSURE_CRITICAL_STATE_2        = 109,              // Proportional valve pressure above 1.5 bar for more than 2 consecutive seconds
    /* 106 - 110 reserved for Proportional Valve Pressure errors */
    VACUUM_CHAMBER_SENSOR_TIMEOUT_ERROR                 = 111,              // Vacuum sensor didn't reach 20 mBar before timeout
    /* 10000 - 20000 reserved for warnings*/
    WARNING_MASS_FLOW_SENSOR_FAULT                      = 10001,            // Mass flow sensor is disconnected or I2C has problem
    WARNING_O2_SENSOR_FAULT                             = 10002,            // O2 sensor is disconnected or I2C has problem
    WARNING_CO2_SENSOR_FAULT                            = 10003,            // CO2 sensor is disconnected
}ErrorCodes_t;

void stringifyErrorCode(char* error_stringified, ErrorCodes_t code) {
    switch (code){
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

//Functions
void system_data_init(void);
void read_incoming_system_data(void);
void send_lock_status(void);
void send_unlock_status(void);

UNIT_iot_status_t get_unit_status(sequence_state_t last_unit_state);
SKID_iot_status_t get_skid_status(sequence_state_t last_skid_state);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_DATA_H_ */