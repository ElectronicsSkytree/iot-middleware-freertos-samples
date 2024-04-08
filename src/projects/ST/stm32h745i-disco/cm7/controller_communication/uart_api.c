/* Copyright (c) Skytree B.V. All rights reserved. */

/* Local includes */
#include "gui_comm_api.h"
#include "uart_api.h"
#include "config.h"

/* Driver includes */
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_uart.h"
#include "stm32h7xx_hal_rcc_ex.h"
#include "stm32h7xx_hal_rcc.h"
#include "stm32h7xx_hal_gpio.h"

/* DEFINITIONS AND MACROS */
typedef enum{
  READY,
  BUSY
}uart_state_t;

/* VARIABLES */
UART_HandleTypeDef uart1;

uart_tx_struct_t uart_current_process_data;

uart_state_t state = READY;

uint8_t uart_message_buf;

/**
 * @brief uart API initializer.
*/
void uart_api_init(void){
    GPIO_InitTypeDef gpioinitstruct = {0};

    /* Enable GPIO clock */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Enable USART clock */
    __HAL_RCC_USART1_CLK_ENABLE();

    /* Configure USART Tx and Rx as alternate function push-pull */
    gpioinitstruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    gpioinitstruct.Mode = GPIO_MODE_AF_PP;
    gpioinitstruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpioinitstruct.Pull = GPIO_PULLUP;
    gpioinitstruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &gpioinitstruct);

    /* USART configuration */
    uart1.Instance = USART1;
    uart1.Init.BaudRate = 57600;
    uart1.Init.WordLength = UART_WORDLENGTH_8B;
    uart1.Init.StopBits = UART_STOPBITS_1;
    uart1.Init.Parity = UART_PARITY_NONE;
    uart1.Init.Mode = UART_MODE_TX_RX;
    uart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart1.Init.OverSampling = UART_OVERSAMPLING_16;
    uart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    uart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    HAL_UART_Init(&uart1);

    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    HAL_UART_Receive_IT(&uart1, &uart_message_buf, 1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
    // configPRINTF( ( "HAL_UART_RxCpltCallback 1" ) );
    gui_comm_rx_buffer_add(uart_message_buf);
    HAL_UART_Receive_IT(&uart1, &uart_message_buf, 1);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
    state = READY;
}

void USART1_IRQHandler(void){
    HAL_UART_IRQHandler(&uart1);
}

error_t uart_send_data(uint8_t* data, uint16_t data_size, tx_callback_t pre_trans_callback, tx_callback_t post_trans_callback, void* callback_args){
    if(state == READY){
      uart_current_process_data.data_to_send = data;
      uart_current_process_data.data_size = data_size;
      uart_current_process_data.pre_trans_callback = pre_trans_callback;
      uart_current_process_data.post_trans_callback = post_trans_callback;
      uart_current_process_data.callback_args = callback_args;

      if(pre_trans_callback != NULL) pre_trans_callback(callback_args);
      if(HAL_UART_Transmit_IT(&uart1, data, data_size) == HAL_ERROR) return FAILED;

      state = BUSY;

      return SUCCEEDED;
    }

    return FAILED;
}
