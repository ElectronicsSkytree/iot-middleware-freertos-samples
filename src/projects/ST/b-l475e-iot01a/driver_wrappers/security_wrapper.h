/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef SECURITY_WRAPPER_H
#define SECURITY_WRAPPER_H

#ifdef __cplusplus
    extern "C" {
#endif

/**
 * @brief RNG init.
 * 
 * @note On failure we are reseting the MCU.
*/
void RNGInit( void );


#ifdef __cplusplus
    }
#endif

#endif // SECURITY_WRAPPER_H