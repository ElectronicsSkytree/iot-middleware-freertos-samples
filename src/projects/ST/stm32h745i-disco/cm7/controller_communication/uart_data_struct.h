/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef UART_DATA_STRUCT_H
#define UART_DATA_STRUCT_H

#ifdef __cplusplus
	extern "C" {
#endif

/* Standard includes. */
#include "stdint.h"

/* DEFINITIONS AND MACROS */
typedef void(*tx_callback_t)(void*);

/**
 * @brief uart Tx struct
*/
typedef struct{
	 uint8_t* data_to_send;
	 uint16_t data_size;
	 tx_callback_t pre_trans_callback;
	 tx_callback_t post_trans_callback;
	 void* callback_args;
} uart_tx_struct_t;

/* VARIABLES */

/* FUNCTIONS DECLARATIONS */

#ifdef __cplusplus
	}
#endif

#endif // UART_DATA_STRUCT_H
