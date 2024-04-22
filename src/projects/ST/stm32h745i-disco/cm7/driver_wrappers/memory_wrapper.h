/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef MEMORY_WRAPPER_H
#define MEMORY_WRAPPER_H

/**
 * @brief Initializes the FreeRTOS heap.
 *
 * Heap_5 is being used because the RAM is not contiguous, therefore the heap
 * needs to be initialized.  See http://www.freertos.org/a00111.html
 */
void InitializeHeap( void );

/**
 * @brief MPU configuration
*/
void MPUConfig( void );

#endif // MEMORY_WRAPPER_H
