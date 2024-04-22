/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef PROCESS_TELEMETRY_H
#define PROCESS_TELEMETRY_H

/**
 * @brief Create IoT telemetry task.
 * 
 * Sending telemetry to the backend every 30 seconds.
 * 
 * @return pdTRUE if telemetry task created successfully and
 *         pdFALSE if failed to create the telemetry task.
 */
BaseType_t vCreateIoTTelemetryTask( void );

/**
 * @brief Delete IoT telemetry task.
 * 
 * @return pdTRUE if telemetry task deleted successfully and
 *         pdFALSE if failed to delete the telemetry task.
 */
BaseType_t vDeleteIoTTelemetryTask( void );

#endif // PROCESS_TELEMETRY_H
