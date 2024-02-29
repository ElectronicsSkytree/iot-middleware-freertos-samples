/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef CONNECTION_H
#define CONNECTION_H

/* Standard includes. */
#include <stdbool.h>

#ifdef __cplusplus
    extern "C" {
#endif

/**
 * @brief To check if the MCU is connected to the internet.
 *
 * @todo Does not implement anything currently, returns true.
 */
bool IsConnectedToInternet();

#ifdef __cplusplus
    }
#endif

#endif // CONNECTION_H