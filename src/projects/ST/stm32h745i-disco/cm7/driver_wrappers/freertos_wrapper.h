/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef FREERTOS_WRAPPER_H
#define FREERTOS_WRAPPER_H

#ifdef __cplusplus
    extern "C" {
#endif

/**
 * @brief Initializes the FreeRTOS heap.
 *
 * Heap_5 is being used because the RAM is not contiguous, therefore the heap
 * needs to be initialized.  See http://www.freertos.org/a00111.html
 */
void InitializeHeap( void );

void MPUConfig( void );

#ifdef __cplusplus
    }
#endif

#endif // FREERTOS_WRAPPER_H