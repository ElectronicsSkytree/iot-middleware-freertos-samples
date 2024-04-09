/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef _UART_API
#define _UART_API

#ifdef __cplusplus
    extern "C" {
#endif

/* Local includes */
#include "errors.h"
#include "uart_data_struct.h"

/**
 * @brief Initialize uart
*/
void uart_api_init(void);

/**
 * @brief Send data over uart
*/
error_t uart_send_data(uint8_t* data, uint16_t data_size, tx_callback_t pre_trans_callback,
                       tx_callback_t post_trans_callback, void* callback_args);

#ifdef __cplusplus
    }
#endif

#endif //_UART_API