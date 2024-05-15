/* Copyright (c) Skytree B.V. All rights reserved. */

/* Local includes */
#include "errors.h"
#include "watchdog_wrapper.h"

/* HAL includes */
#include "stm32l4xx_hal_iwdg.h"

IWDG_HandleTypeDef hiwdg;
/*-----------------------------------------------------------*/

void MX_IWDG_Init(void) {

    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_64;  
    hiwdg.Init.Reload = 4095;                  

    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        // Initialization Error
        // Error_Handler();
    }
}
