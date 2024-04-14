/* Copyright (c) Skytree B.V. All rights reserved. */

/* Local includes. */
#include "errors.h"
#include "config.h"
#include "time_wrapper.h"

/* Standard includes */
#include <time.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "timers.h"
#include "lwip.h"
#include "lwip/apps/sntp.h"

/* HAL includes */
#include "stm32h7xx_hal.h"

/* Global & static Variables */
RTC_HandleTypeDef xHrtc;
TimerHandle_t xHTimerHandle;

static const char * pTimeServers[] = { "pool.ntp.org", "time.nist.gov" };
const size_t numTimeServers = sizeof( pTimeServers ) / sizeof( char * );

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
void SystemClockConfig( void )
{
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };

    /** Supply configuration update enable
     */
    /*HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY); */

    /** Configure the main internal regulator output voltage
     */
    /*__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1); */

    /*while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {} */

    /** Initializes the CPU, AHB and APB busses clocks
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48 | RCC_OSCILLATORTYPE_LSI
                                       | RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 2;
    RCC_OscInitStruct.PLL.PLLN = 64;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;

    if( HAL_RCC_OscConfig( &RCC_OscInitStruct ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }

    /** Initializes the CPU, AHB and APB busses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                                  | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

    if( HAL_RCC_ClockConfig( &RCC_ClkInitStruct, FLASH_LATENCY_2 ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC | RCC_PERIPHCLK_USART3
                                               | RCC_PERIPHCLK_RNG;
    PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
    PeriphClkInitStruct.RngClockSelection = RCC_RNGCLKSOURCE_HSI48;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;

    if( HAL_RCCEx_PeriphCLKConfig( &PeriphClkInitStruct ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }
}
/*-----------------------------------------------------------*/

void setTimeRTC( uint32_t sec )
{
    struct tm calTime;
    time_t unixTime = sec;
    RTC_TimeTypeDef sTime = { 0 };
    RTC_DateTypeDef sDate = { 0 };

    gmtime_r( &unixTime, &calTime );

    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    sTime.TimeFormat = RTC_HOURFORMAT_24;
    sTime.Seconds = calTime.tm_sec;
    sTime.SecondFraction = 0;
    sTime.SubSeconds = 0;
    sTime.Minutes = calTime.tm_min;
    sTime.Hours = calTime.tm_hour;
    sDate.WeekDay = calTime.tm_wday;
    sDate.Date = calTime.tm_mday;
    sDate.Month = calTime.tm_mon;
    sDate.Year = calTime.tm_year;

    HAL_RTC_SetTime( &xHrtc, &sTime, RTC_FORMAT_BIN );
    HAL_RTC_SetDate( &xHrtc, &sDate, RTC_FORMAT_BIN );
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
    xHrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    xHrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    if( HAL_RTC_Init( &xHrtc ) != HAL_OK )
    {
        Error_Handler(__func__, __LINE__);
    }

    setTimeRTC( 0 ); /* Initializing at epoch. */
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

static void getTimeRTC( uint32_t * pTime )
{
    struct tm calTime;
    RTC_TimeTypeDef sTime = { 0 };
    RTC_DateTypeDef sDate = { 0 };

    HAL_RTC_GetTime( &xHrtc, &sTime, RTC_FORMAT_BIN );
    HAL_RTC_GetDate( &xHrtc, &sDate, RTC_FORMAT_BIN );

    calTime.tm_sec = sTime.Seconds;
    calTime.tm_min = sTime.Minutes;
    calTime.tm_hour = sTime.Hours;
    calTime.tm_isdst = sTime.DayLightSaving;
    calTime.tm_mday = sDate.Date;
    calTime.tm_mon = sDate.Month;
    calTime.tm_year = sDate.Year;

    *pTime = mktime( &calTime );
}
/*-----------------------------------------------------------*/

/**
 * @brief Set RTC with the time from the known ntp servers
 * 
*/
BaseType_t InitializeSNTP( void )
{
    BaseType_t ret = 0;
    uint32_t unixTime = 0;

    sntp_setoperatingmode( SNTP_OPMODE_POLL );

    for( uint8_t i = 0; i < numTimeServers; i++ )
    {
        sntp_setservername( i, pTimeServers[ i ] );
    }

    sntp_init();

    do
    {
        getTimeRTC( &unixTime );

        // @todo seems to be a silly logic
        if( unixTime < configSNTP_INIT_WAIT )
        {
            configPRINTF( ( "SNTP not queried yet. Retrying.\r\n" ) );
            vTaskDelay( configSNTP_INIT_RETRY_DELAY / portTICK_PERIOD_MS );
        }
    } while( unixTime < configSNTP_INIT_WAIT );

    configPRINTF( ( "> SNTP Initialized: %lu\r\n", unixTime ) );

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

    getTimeRTC( &unixTime );
    return ( uint64_t ) unixTime;
}
/*-----------------------------------------------------------*/
