/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef TELEMETRY_H
#define TELEMETRY_H

#ifdef __cplusplus
    extern "C" {
#endif

/* Local Includes */
#include "controller_data.h"

/* Standard Includes */
#include <stdint.h>

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
uint32_t CreateBootUpMessage( uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength );

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
    uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength );

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
    uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength );

#ifdef __cplusplus
    }
#endif

#endif // TELEMETRY_H
