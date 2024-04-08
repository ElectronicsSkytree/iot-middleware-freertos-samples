/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
    extern "C" {
#endif

/* HAL includes */
#include "stm32h7xx_hal.h"

/*
 * Get board specific unix time.
 */
uint64_t GetUnixTime( void );

#ifdef __cplusplus
    }
#endif

#endif // MAIN_H
