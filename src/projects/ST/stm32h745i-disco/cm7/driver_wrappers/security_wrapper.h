/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef SECURITY_WRAPPER_H
#define SECURITY_WRAPPER_H

/**
 * @brief RNG Initialization Function
 * 
 * @note On failure we are reseting the MCU.
 * 
 * @param None
 * @retval None
 */
void MXRNGInit( void );

#endif // SECURITY_WRAPPER_H
