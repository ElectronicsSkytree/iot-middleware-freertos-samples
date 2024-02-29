/* Copyright (c) Skytree B.V. All rights reserved. */

/* Local Includes */
#include "errors.h"
#include "security_wrapper.h"

/* Standard Includes */
#include <stddef.h>
#include <string.h>

/* FreeRTOS Includes */
#include "FreeRTOS.h"

/* HAL includes */
#include "stm32l4xx_hal.h"

RNG_HandleTypeDef xHrng;

/**
 * @brief RNG init.
 * 
 * @note On failure we are reseting the MCU.
*/
void RNGInit( void )
{
    /* RNG init function. */
    xHrng.Instance = RNG;

    if( HAL_RNG_Init( &xHrng ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }
}
/*-----------------------------------------------------------*/


/**
 * @brief fptr for the entropy source
*/
int mbedtls_platform_entropy_poll( void * data,
                                   unsigned char * output,
                                   size_t len,
                                   size_t * olen )
{
    HAL_StatusTypeDef status = HAL_OK;
    uint32_t random_number = 0;

    status = HAL_RNG_GenerateRandomNumber( &xHrng, &random_number );
    ( ( void ) data );
    *olen = 0;

    if( ( len < sizeof( uint32_t ) ) || ( HAL_OK != status ) )
    {
        return 0;
    }

    memcpy( output, &random_number, sizeof( uint32_t ) );
    *olen = sizeof( uint32_t );

    return 0;
}
/*-----------------------------------------------------------*/

/**
 * @brief Psuedo random number generator.
 * 
 * @note Do not use the standard C library rand() function as it can cause
 * unexpected behaviour, such as calls to malloc().
 * 
 * @todo Revisit on the security aptness
*/
int GetRand32( void )
{
    static UBaseType_t uxlNextRand; /*_RB_ Not seeded. */
    const uint32_t ulMultiplier = 0x015a4e35UL, ulIncrement = 1UL;

    /* Utility function to generate a pseudo random number. */

    uxlNextRand = ( ulMultiplier * uxlNextRand ) + ulIncrement;

    return( ( int ) ( uxlNextRand >> 16UL ) & 0x7fffUL );
}
/*-----------------------------------------------------------*/