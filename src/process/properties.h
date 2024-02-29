/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef PROPERTIES_H
#define PROPERTIES_H

#ifdef __cplusplus
    extern "C" {
#endif

/* Local Includes */
#include "errors.h"

/* Standard Includes */
#include <stdint.h>

#define SERIAL_NUMBER_MAX_LENGTH    30
#define MAX_NUMBER_OF_UNITS         3
#define MAX_NUMBER_OF_CARTRIDGES    3

typedef struct{
    char serial_number[SERIAL_NUMBER_MAX_LENGTH];
}Identity_t;

typedef struct{
    Identity_t identity;
}CCU_Properties_t;

typedef struct{
    Identity_t identity;
}Tank_Properties_t;

typedef struct{
    Identity_t identity;
    int32_t slot_number;
}CARTRIDGE_Properties_t;

typedef struct{
    Identity_t identity;
    uint8_t number_of_cartridges;
    CARTRIDGE_Properties_t cartridges[MAX_NUMBER_OF_CARTRIDGES];
}UNIT_Properties_t;

typedef struct{
    double version;
    int32_t telemetry_interval;
    uint8_t number_of_units;
    CCU_Properties_t ccu;
    Tank_Properties_t tank;
    UNIT_Properties_t units[MAX_NUMBER_OF_UNITS];
}SYSTEM_Properties_t;


/**
 * @brief Parse desired properties
 * 
 * All the system (CCU, Units, Tank) properties are fetched from
 * the desired prperties payload.
 * 
 * @todo Currently we are interested only in system properties.
 *       Any other details in the desired properties are ignored.
 * 
 * @param[in] payload desired properties payload received from IoTHub
 * @param[in] payload_length length of the payload
 * @param[out] system_properties parsed system properties to be filled
 * 
 * @return 0 for failure and 1 for success
*/
error_t ParseDesiredProperties( const uint8_t* payload, uint32_t payload_length, SYSTEM_Properties_t* system_properties );

/**
 * @brief Parse system properties
 * 
 * All the system (CCU, Units, Tank) properties are fetched from
 * the prperties payload. The details are mainly present in the
 * desired properties section.
 * 
 * @todo Currently we are interested only in system properties.
 *       Any other details in the desired properties are ignored.
 * 
 * @param[in] payload desired properties payload received from IoTHub
 * @param[in] payload_length length of the payload
 * @param[out] system_properties parsed system properties to be filled
 * 
 * @return 0 for failure and 1 for success
*/
error_t ParseSystemProperties(const uint8_t* payload, uint32_t payload_length, SYSTEM_Properties_t* system_properties);

#ifdef __cplusplus
    }
#endif

#endif // PROPERTIES_H
