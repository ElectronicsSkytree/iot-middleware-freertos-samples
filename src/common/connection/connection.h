/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef CONNECTION_H
#define CONNECTION_H

#include <stdbool.h>

/**
 * @brief Indicates if connected to the internet or not.
 *
 * @return A #bool result where true indicates the sample is connected to the internet,
 *         and false indicates it is not.
 */
bool IsConnectedToInternet( void );

#endif // CONNECTION_H
