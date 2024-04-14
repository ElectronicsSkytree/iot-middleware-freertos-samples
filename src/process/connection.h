/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef CONNECTION_H
#define CONNECTION_H

/* Standard includes. */
#include <stdbool.h>

#ifdef __cplusplus
    extern "C" {
#endif

/**
 * @brief Is connected to the internet
 * 
 * @todo TBD, retruns true for now
*/
bool IsConnectedToInternet()
{
    return true;
}

#ifdef __cplusplus
    }
#endif

#endif // CONNECTION_H