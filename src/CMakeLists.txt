# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

# Copyright (c) Skytree B.V. All rights reserved.

if (NOT BOARD)
    message(FATAL_ERROR "Board is not defined.")
endif()

string(TOLOWER ${BOARD} BOARD_L)
string(TOUPPER ${BOARD} BOARD_U)

# Target for SKYTREE task
if(NOT (TARGET SKYTREE::IOT_PROCESS))
    add_library(SKYTREE::IOT_PROCESS INTERFACE IMPORTED)
    target_sources(SKYTREE::IOT_PROCESS INTERFACE 
      ${CMAKE_CURRENT_SOURCE_DIR}/process/iot_tasks.c
      ${CMAKE_CURRENT_SOURCE_DIR}/process/properties.c
      ${CMAKE_CURRENT_SOURCE_DIR}/process/telemetry.c)
    target_include_directories(SKYTREE::IOT_PROCESS INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/process)
endif()

# Target for freertos tcpip socket
if(NOT (TARGET SKYTREE::SOCKET::FREERTOSTCPIP))
    add_library(SKYTREE::SOCKET::FREERTOSTCPIP INTERFACE IMPORTED)
    target_sources(SKYTREE::SOCKET::FREERTOSTCPIP INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport/sockets_wrapper_freertos_tcpip.c)
    target_include_directories(SKYTREE::SOCKET::FREERTOSTCPIP INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport)
endif()

# Target for lwip based socket
if(NOT (TARGET SKYTREE::SOCKET::LWIP))
    add_library(SKYTREE::SOCKET::LWIP INTERFACE IMPORTED)
    target_sources(SKYTREE::SOCKET::LWIP INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport/sockets_wrapper_lwip.c)
    target_include_directories(SKYTREE::SOCKET::LWIP INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport)
endif()

# Target for transport using sockets
if(NOT (TARGET SKYTREE::TRANSPORT::SOCKET))
    add_library(SKYTREE::TRANSPORT::SOCKET INTERFACE IMPORTED)
    target_sources(SKYTREE::TRANSPORT::SOCKET INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport/transport_socket.c)
    target_include_directories(SKYTREE::TRANSPORT::SOCKET INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport/
        ${CMAKE_CURRENT_SOURCE_DIR}/common/utilities/)
endif()

# Target for transport using Mbedtls
if(NOT (TARGET SKYTREE::TRANSPORT::MBEDTLS))
    add_library(SKYTREE::TRANSPORT::MBEDTLS INTERFACE IMPORTED)
    target_sources(SKYTREE::TRANSPORT::MBEDTLS INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport/transport_tls_socket_using_mbedtls.c
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport/transport_socket.c
        ${CMAKE_CURRENT_SOURCE_DIR}/common/utilities/azure_sample_crypto_mbedtls.c
        ${CMAKE_CURRENT_SOURCE_DIR}/common/utilities/mbedtls_freertos_port.c)
    target_include_directories(SKYTREE::TRANSPORT::MBEDTLS INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport/
        ${CMAKE_CURRENT_SOURCE_DIR}/common/utilities/)
endif()

# Target for SKYTREE connection module
if(NOT (TARGET SKYTREE::COMMON::CONNECTION))
    add_library(SKYTREE::COMMON::CONNECTION INTERFACE IMPORTED)
    target_include_directories(SKYTREE::COMMON::CONNECTION INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/common/connection/)
endif()

# Target for iot communication (with controller)
if(NOT (TARGET SKYTREE::IOT_COMMUNICATION))
    add_library(SKYTREE::IOT_COMMUNICATION INTERFACE IMPORTED)
    target_include_directories(SKYTREE::IOT_COMMUNICATION INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/iot_communication/)
endif()

# Add board specific
if(BOARD_L STREQUAL "stm32h745i-disco")
    set(BOARD_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/projects/${VENDOR}/${BOARD_L}/cm7)
else()
    set(BOARD_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/projects/${VENDOR}/${BOARD_L})
endif()

add_subdirectory(${BOARD_SOURCE_PATH})
