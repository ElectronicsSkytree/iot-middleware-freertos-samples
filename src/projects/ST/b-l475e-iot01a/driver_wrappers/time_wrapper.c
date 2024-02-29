/* Copyright (c) Skytree B.V. All rights reserved. */

/* Local includes. */
#include "errors.h"
#include "config.h"
#include "time_wrapper.h"
#include "wifi_wrapper.h"

/* Standard includes */
#include <time.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "timers.h"

/* HAL includes */
#include "stm32l4xx_hal.h"

RTC_HandleTypeDef xHrtc;
TimerHandle_t xHTimerHandle;

/**
 * @brief Time sync timer Callback
*/
static void TimeSyncCallback(TimerHandle_t xTimer)
{
    configPRINTF( ( "---------Time sync callback---------\r\n" ) );

    // Set the time again
    if( InitializeSNTP() != 0 )
    {
        Error_Handler(__func__, __LINE__);
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Resets the hour in RTC
 * 
 * Resets the hour and updates the weekday. This function is useful
 * end of day to set the hour back to 00::00 and progress day.
 * 
 * @todo is there a better way of handling this?
*/
static void ResetHour( RTC_TimeTypeDef sTime, RTC_DateTypeDef sDate )
{
    // Reset the hour to 0
    sTime.Hours = 0;

    // Update the weekday (assuming Sunday is 0 and Saturday is 6)
    sDate.WeekDay = (sDate.WeekDay + 1) % 7;

    // Update the RTC time and date with the new hour and weekday
    HAL_RTC_SetTime(&xHrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&xHrtc, &sDate, RTC_FORMAT_BIN);
}
/*-----------------------------------------------------------*/

/**
 * @brief set RTC date and time
 * 
 * @param unix_timestamp utc timestamp
 * 
*/
static void SetUtcTimestamp(uint32_t unix_timestamp)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    // Convert timestamp to struct tm
    time_t timestamp = (time_t)unix_timestamp;
    struct tm *timeInfo = gmtime(&timestamp);

    // Set the time structure
    sTime.Hours = timeInfo->tm_hour;
    sTime.Minutes = timeInfo->tm_min;
    sTime.Seconds = timeInfo->tm_sec;

    // Set the date structure
    sDate.Year = timeInfo->tm_year - 100; // tm_year is the year since 1900 but we need to set since 2000
    sDate.Month = timeInfo->tm_mon + 1; // tm_mon is 0-based (0 for January)
    sDate.Date = timeInfo->tm_mday;

    configPRINTF( ( "ES-WIFI set time: year(%d) month(%d) date(%d)\r\n", sDate.Year, sDate.Month, sDate.Date ) );

    // Set the RTC with the obtained time and date
    HAL_RTC_SetTime(&xHrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&xHrtc, &sDate, RTC_FORMAT_BIN);
}
/*-----------------------------------------------------------*/

/**
 * @brief Initializes the system clock.
 */
void SystemClock_Config( void )
{
    RCC_OscInitTypeDef xRCC_OscInitStruct;
    RCC_ClkInitTypeDef xRCC_ClkInitStruct;
    RCC_PeriphCLKInitTypeDef xPeriphClkInit;

    xRCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_MSI;
    xRCC_OscInitStruct.LSEState = RCC_LSE_ON;
    xRCC_OscInitStruct.MSIState = RCC_MSI_ON;
    xRCC_OscInitStruct.MSICalibrationValue = 0;
    xRCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;
    xRCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    xRCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    xRCC_OscInitStruct.PLL.PLLM = 6;
    xRCC_OscInitStruct.PLL.PLLN = 20;
    xRCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
    xRCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    xRCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;

    if( HAL_RCC_OscConfig( &xRCC_OscInitStruct ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     * clocks dividers. */
    xRCC_ClkInitStruct.ClockType = ( RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 );
    xRCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    xRCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    xRCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    xRCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if( HAL_RCC_ClockConfig( &xRCC_ClkInitStruct, FLASH_LATENCY_4 ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }

    xPeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC
                                          | RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART3 | RCC_PERIPHCLK_I2C2
                                          | RCC_PERIPHCLK_RNG;
    xPeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
    xPeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
    xPeriphClkInit.I2c2ClockSelection = RCC_I2C2CLKSOURCE_PCLK1;
    xPeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    xPeriphClkInit.RngClockSelection = RCC_RNGCLKSOURCE_MSI;

    if( HAL_RCCEx_PeriphCLKConfig( &xPeriphClkInit ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }

    __HAL_RCC_PWR_CLK_ENABLE();

    if( HAL_PWREx_ControlVoltageScaling( PWR_REGULATOR_VOLTAGE_SCALE1 ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }

    /* Enable MSI PLL mode. */
    HAL_RCCEx_EnableMSIPLLMode();
}
/*-----------------------------------------------------------*/

/**
 * @brief RTC initialization.
 */
void RTCInit( void )
{
    xHrtc.Instance = RTC;
    xHrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    xHrtc.Init.AsynchPrediv = 127;
    xHrtc.Init.SynchPrediv = 255;
    xHrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    xHrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    xHrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    xHrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    if( HAL_RTC_Init( &xHrtc ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Get current utc timestamp in a string.
 *
 * @param timestamp_utc timestamp in zulu format.
 * 
 * @note Caller needs to take care of memory.
 */
const char* GetUtcTimeInString( void )
{
    static char timestamp_utc[30];
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    HAL_RTC_GetTime(&xHrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&xHrtc, &sDate, RTC_FORMAT_BIN);

    if( sTime.Hours == 24 )
    {
        ResetHour(sTime, sDate);

        HAL_RTC_GetTime(&xHrtc, &sTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&xHrtc, &sDate, RTC_FORMAT_BIN);
    }

    // We do +2000 as the year returned is since 2000
    sprintf(timestamp_utc, "%04d-%02d-%02dT%02d:%02d:%02dZ",
            sDate.Year + 2000, sDate.Month, sDate.Date,
            sTime.Hours, sTime.Minutes, sTime.Seconds);

    return timestamp_utc;
}
/*-----------------------------------------------------------*/

/**
 * @brief Set RTC with wifi utc
 * 
 * Retrieve utc (gmt) time from WiFi driver and update
 * local RTC with the time and date.
*/
BaseType_t InitializeSNTP( void )
{
    BaseType_t ret = 0;
    uint32_t unixTime = 0;

    configPRINTF( ( "ES-WIFI Initializing Time.\r\n" ) );

    do
    {
        if( FAILED == GetWiFiTime(&unixTime) )
        {
            configPRINTF( ( "!!!ERROR: ES-WIFI Get Time Failed.\r\n" ) );
            ret = -1;
        }

        if( unixTime < configSNTP_INIT_WAIT )
        {
            configPRINTF( ( "ES-WIFI Failed to get time. Retrying.\r\n" ) );
            HAL_Delay( configSNTP_INIT_RETRY_DELAY );
        }
    } while( ( unixTime < configSNTP_INIT_WAIT ) && ( ret == 0 ) );

    if( ret == 0 )
    {
        configPRINTF( ( "> ES-WIFI Time Initialized: %lu\r\n",
                        unixTime ) );

        // Set the RTC
        SetUtcTimestamp(unixTime);
    }

    return ret;
}
/*-----------------------------------------------------------*/

/**
 * @brief Start time sync
 * 
 * Starts a timer to make sure time is synced and sane.
*/
void StartTimeSync()
{
    /* Timer for timestamp sync */
    xHTimerHandle = xTimerCreate("time_sync_timer", TWO_HOURS, pdTRUE, 0, TimeSyncCallback);

    if (xHTimerHandle != NULL) {
        // Start the timer
        xTimerStart(xHTimerHandle, 0);
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Get current utc time
*/
uint64_t GetUnixTime( void )
{
    uint32_t unixTime;

    GetWiFiTime( &unixTime );

    return ( uint64_t ) unixTime;
}
/*-----------------------------------------------------------*/
