/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef CONTROLLER_DATA_H
#define CONTROLLER_DATA_H

#ifdef __cplusplus
    extern "C" {
#endif

/* Local includes */
#include "iot_communication.h"
#include "gui_comm_api.h"

void ControllerCommunicationTask( void* arg );
void send_lock_status(void);
void send_unlock_status(void);
UNIT_iot_status_t get_unit_status(sequence_state_t last_unit_state);
SKID_iot_status_t get_skid_status(sequence_state_t last_skid_state);

// Helper functions
void stringifyErrorCode(char* error_stringified, ErrorCodes_t code);

#ifdef __cplusplus
    }
#endif

#endif // CONTROLLER_DATA_H