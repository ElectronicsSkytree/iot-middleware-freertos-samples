/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/* Copyright (c) Skytree B.V. All rights reserved. */

#ifndef CONFIG_H
#define CONFIG_H

/* Local Includes */
#include "log_utility.h"
// #include "time_wrapper.h"

/* FreeRTOS config include. */
#include "FreeRTOSConfig.h"

// Tests on desk: Still point to old iothub
#define HOME_SETUP 1
// #define OFC_SETUP 1

// Pilots
// #define WUR_SETUP 1
// #define GROWY_SETUP 1
// #define KOPPERT_CRESS_SETUP 1
// #define FIELDLESS_SETUP 1

/**************************************************/
/******* DO NOT CHANGE the following order ********/
/**************************************************/

/* Include logging header files and define logging macros in the following order:
 * 1. Include the header file "logging_levels.h".
 * 2. Define the LIBRARY_LOG_NAME and LIBRARY_LOG_LEVEL macros depending on
 * the logging configuration.
 * 3. Include the header file "logging_stack.h", if logging is enabled.
 */

#include "logging_levels.h"

/* Logging configuration. */
#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME    "AzureIoT"
#endif

#ifndef LIBRARY_LOG_LEVEL
    #define LIBRARY_LOG_LEVEL    LOG_INFO
#endif

/*
 * The function prints to the console before the network is connected;
 * then a UDP port after the network has connected. */
extern void vLoggingPrintf( const char * pcFormatString, ... );

/* Map the SdkLog macro to the logging function to enable logging */
#ifndef SdkLog
    #define SdkLog( message )    vLoggingPrintf message
#endif

#include "logging_stack.h"

/************ End of logging configuration ****************/

/**
 * @brief Enable Device Provisioning
 *
 * @note To disable Device Provisioning undef this macro
 *
 */
#define enableDPS

#ifdef enableDPS

/**
 * @brief Provisioning service endpoint.
 *
 * @note https://docs.microsoft.com/azure/iot-dps/concepts-service#service-operations-endpoint
 *
 */
    #define configENDPOINT           "global.azure-devices-provisioning.net"

/**
 * @brief Id scope of provisioning service.
 *
 * @note https://docs.microsoft.com/azure/iot-dps/concepts-service#id-scope
 *
 */
    #if defined(HOME_SETUP) || defined(OFC_SETUP)
        // @todo: Old iot for test devices, move these to new one as well
        #define configID_SCOPE           "0ne00AFD579"
    #else
        #define configID_SCOPE           "0ne00BF2AFD"
    #endif

/**
 * @brief Registration Id of provisioning service
 *
 * @warning If using X509 authentication, this MUST match the Common Name of the cert.
 *
 *  @note https://docs.microsoft.com/azure/iot-dps/concepts-service#registration-id
 */
    #ifdef HOME_SETUP
        #define configREGISTRATION_ID    "skytree_iotkit_home" // home
    #elif defined(OFC_SETUP)
        #define configREGISTRATION_ID    "skytree_iotkit_office" // office
    #elif defined(WUR_SETUP)
        #define configREGISTRATION_ID    "skytree_iotkit_wur" // WUR
    #elif defined(GROWY_SETUP)
        #define configREGISTRATION_ID    "skytree_growy_with_alten" // Growy
    #elif defined(KOPPERT_CRESS_SETUP)
        #define configREGISTRATION_ID    "skytree_koppert_cress_pilot" // Koppert Cress
    #elif defined(FIELDLESS_SETUP)
        #define configREGISTRATION_ID    "skytree_iot_fieldless_pilot" // Fieldless
    #endif  

#endif /* enableDPS */

/**
 * @brief Client's X509 Certificate & key
 * @note not doing X.509 way currently
 */
// #define configCLIENT_CERTIFICATE_PEM    "<YOUR DEVICE CERT HERE>"
// #define configCLIENT_PRIVATE_KEY_PEM    "<YOUR DEVICE PRIVATE KEY HERE>"

/**
 * @brief Device symmetric key
 *
 */
#ifdef HOME_SETUP
    #define deviceSYMMETRIC_KEY    "5Y53mc7C7vwmhumMCgj441h7CvdXo+Kr4xxSFUJzYP67vhCWrPvEzMeHIJISFqEp4hO4r6gJykKfW2OzQkjb/w==" // home device
#elif defined(OFC_SETUP)
    #define deviceSYMMETRIC_KEY    "PLbvN5Tlmu0pViNPBlUDioXvwAzjzeGTCJOVb36Z5QZ6GDZzDRKehzqtERJlIAnl8QFbAUB0PtkMO7y11tIiUw==" // Office device
#elif defined(WUR_SETUP)
    #define deviceSYMMETRIC_KEY    "xpEawa5WwBPLLKKZmqbFea9m3mcyLxFDETZpPqT/w9LReyhGprneLbrnmd3xnn5tURkW2eaxUDkcE1eDOwdMtw==" // WUR
#elif defined(GROWY_SETUP)
    #define deviceSYMMETRIC_KEY "hqJEvY1bC93mIksjXPR4kdnx0jx/Yv5h/JsxzDBICKMniegZI07f+aiIuz/Ol49LwFxvCTiOdGKpUJKN6A2XWw=="   // Growy
#elif defined(KOPPERT_CRESS_SETUP)
    #define deviceSYMMETRIC_KEY "be3aTTmc8ZsVD3Dntl2Fbxr2H3isAZ056/rKE95BNrHhFp8dGQ8mPLTk8plfBwweCaYIAu8aMY1qZNn6gaZQ9w=="   // Koppert Cress 
#elif defined(FIELDLESS_SETUP)
    #define deviceSYMMETRIC_KEY    "lCz8fl+XuEy/higmXJIMuPn1mTY9Ultv99Le+UrcfkE1oBFz9HaEHpMqw9+LOFC95AGyBVHLocb9LNwZXoeyzg==" // Fieldless Alten
#endif

/**
 * @brief Load the required certificates:
 *  - Baltimore Trusted Root CA
 *  - DigiCert Global Root G2
 *  - Microsoft RSA Root Certificate Authority 2017
 *
 * @warning Hard coding certificates is not recommended by Microsoft as a best
 * practice for production scenarios. Please see our document here for notes on best practices.
 * https://github.com/Azure-Samples/iot-middleware-freertos-samples/blob/main/docs/certificate-notice.md
 *
 */
#define configROOT_CA_PEM                                              \
    "-----BEGIN CERTIFICATE-----\r\n"                                      \
    "MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\r\n" \
    "RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\r\n" \
    "VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\r\n" \
    "DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\r\n" \
    "ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\r\n" \
    "VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\r\n" \
    "mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\r\n" \
    "IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\r\n" \
    "mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\r\n" \
    "XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\r\n" \
    "dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\r\n" \
    "jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\r\n" \
    "BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\r\n" \
    "DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\r\n" \
    "9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\r\n" \
    "jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\r\n" \
    "Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\r\n" \
    "ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\r\n" \
    "R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\r\n"                             \
    "-----END CERTIFICATE-----\r\n"                                        \
    "-----BEGIN CERTIFICATE-----\r\n"                                      \
    "MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\r\n" \
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n" \
    "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\r\n" \
    "MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\r\n" \
    "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\r\n" \
    "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\r\n" \
    "9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\r\n" \
    "2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\r\n" \
    "1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\r\n" \
    "q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\r\n" \
    "tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\r\n" \
    "vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\r\n" \
    "BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\r\n" \
    "5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\r\n" \
    "1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\r\n" \
    "NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\r\n" \
    "Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\r\n" \
    "8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\r\n" \
    "pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\r\n" \
    "MrY=\r\n"                                                             \
    "-----END CERTIFICATE-----\r\n"                                        \
    "-----BEGIN CERTIFICATE-----\r\n"                                      \
    "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\r\n" \
    "RA+GsCyRxj3qrg+E\r\n"                                                 \
    "-----END CERTIFICATE-----\r\n"

/**
 * @brief Size of the network buffer for MQTT packets.
 */
#define configNETWORK_BUFFER_SIZE        ( 5 * 1024U )

/**
 * @brief IoTHub endpoint (MQTT) port.
 * 
 * @todo websocket to be enabled later.
 */
#define configIOTHUB_PORT                ( 8883 )

#define configCHUNK_DOWNLOAD_SIZE        1024

#define configSNTP_INIT_WAIT             1672531200U /* January 1, 2023 */
#define configSNTP_INIT_RETRY_DELAY      5000

#endif // CONFIG_H