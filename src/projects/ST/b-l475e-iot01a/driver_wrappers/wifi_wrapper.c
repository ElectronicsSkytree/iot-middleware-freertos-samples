/* Copyright (c) Skytree B.V. All rights reserved. */

/* Local includes. */
#include "config.h"
#include "connection.h"
#include "wifi_wrapper.h"

/* WiFi driver related */
#include "es_wifi.h"
#include "wifi.h"

/* HAL includes */
#include "stm32l475e_iot01.h"

/* Define the default wifi ssid and password.
 * User must override this in config.h
 */
#ifndef WIFI_SSID
    #error "Symbol WIFI_SSID must be defined."
#endif /* WIFI_SSID  */

#ifndef WIFI_PASSWORD
    #error "Symbol WIFI_PASSWORD must be defined."
#endif /* WIFI_PASSWORD  */

#ifndef WIFI_SECURITY_TYPE
    #error "Symbol WIFI_SECURITY_TYPE must be defined."
#endif /* WIFI_SECURITY_TYPE  */

uint8_t MAC_Addr[ 6 ];
uint8_t IP_Addr[ 4 ];
uint8_t Gateway_Addr[ 4 ];
uint8_t DNS1_Addr[ 4 ];
uint8_t DNS2_Addr[ 4 ];

/* Defined in es_wifi_io.c. */
extern SPI_HandleTypeDef hspi;

/*-----------------------------------------------------------*/

/**
 * @brief SPI Interrupt Handler.
 *
 * @note Inventek module is configured to use SPI3.
 */
void SPI3_IRQHandler( void )
{
    HAL_SPI_IRQHandler( &( hspi ) );
}
/*-----------------------------------------------------------*/

/**
 * @brief Check if WiFi driver received time
 * 
 * @param unixTime pointer to utc timestamp to be filled
*/
error_t GetWiFiTime( uint32_t* unixTime )
{
    return (WIFI_GetTime( unixTime ) == WIFI_STATUS_OK) ? true : false;
}

/**
 * @brief Initialize wifi module and connect to WiFi
*/
BaseType_t InitializeWifi( void )
{
    uint32_t hal_version = HAL_GetHalVersion();
    uint32_t bsp_version = BSP_GetVersion();
    WIFI_Status_t xWifiStatus;
    BaseType_t ret;

    /* Turn on the WiFi before key provisioning. This is needed because
     * if we want to use offload SSL, device certificate and key is stored
     * on the WiFi module during key provisioning which requires the WiFi
     * module to be initialized. */
    xWifiStatus = WIFI_Init();

    if( xWifiStatus == WIFI_STATUS_OK )
    {
        configPRINTF( ( "WiFi module initialized.\r\n" ) );

        configPRINTF( ( "STM32L4XX Lib:\r\n" ) );
        configPRINTF( ( "\t> CMSIS Device Version: %d.%d.%d.%d.\r\n",
                        __STM32L4_CMSIS_VERSION_MAIN, __STM32L4_CMSIS_VERSION_SUB1,
                        __STM32L4_CMSIS_VERSION_SUB2, __STM32L4_CMSIS_VERSION_RC ) );
        configPRINTF( ( "\t> HAL Driver Version: %ld.%ld.%ld.%ld.\r\n",
                        ( ( hal_version >> 24U ) & 0xFF ), ( ( hal_version >> 16U ) & 0xFF ),
                        ( ( hal_version >> 8U ) & 0xFF ), ( hal_version & 0xFF ) ) );
        configPRINTF( ( "\t> BSP Driver Version: %ld.%ld.%ld.%ld.\r\n",
                        ( ( bsp_version >> 24U ) & 0xFF ), ( ( bsp_version >> 16U ) & 0xFF ),
                        ( ( bsp_version >> 8U ) & 0xFF ), ( bsp_version & 0xFF ) ) );

        if( WIFI_GetMAC_Address( MAC_Addr ) != WIFI_STATUS_OK )
        {
            configPRINTF( ( "!!!ERROR: ES-WIFI Get MAC Address Failed.\r\n" ) );
            ret = -1;
        }
        else if( WIFI_Connect( WIFI_SSID, WIFI_PASSWORD, WIFI_SECURITY_TYPE ) != WIFI_STATUS_OK )
        {
            configPRINTF( ( "!!!ERROR: ES-WIFI NOT connected.\r\n" ) );
            ret = -1;
        }
        else
        {
            configPRINTF( ( "ES-WIFI MAC Address: %X:%X:%X:%X:%X:%X\r\n",
                            MAC_Addr[ 0 ], MAC_Addr[ 1 ], MAC_Addr[ 2 ],
                            MAC_Addr[ 3 ], MAC_Addr[ 4 ], MAC_Addr[ 5 ] ) );

            configPRINTF( ( "ES-WIFI Connected.\r\n" ) );

            if( WIFI_GetIP_Address( IP_Addr ) == WIFI_STATUS_OK )
            {
                configPRINTF( ( "\t> ES-WIFI IP Address: %d.%d.%d.%d\r\n",
                                IP_Addr[ 0 ],
                                IP_Addr[ 1 ],
                                IP_Addr[ 2 ],
                                IP_Addr[ 3 ] ) );
            }

            if( ( IP_Addr[ 0 ] == 0 ) &&
                ( IP_Addr[ 1 ] == 0 ) &&
                ( IP_Addr[ 2 ] == 0 ) &&
                ( IP_Addr[ 3 ] == 0 ) )
            {
                configPRINTF( ( "!!!ERROR: ES-WIFI Get IP Address Failed.\r\n" ) );
                ret = -1;
            }
            else
            {
                ret = 0;
            }
        }
    }
    else
    {
        configPRINTF( ( "!!!ERROR: WIFI initialization Failed.\r\n" ) );
        ret = -1;
    }

    return ret;
}
/*-----------------------------------------------------------*/
