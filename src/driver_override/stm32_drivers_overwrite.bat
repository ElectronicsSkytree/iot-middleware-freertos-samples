@echo off

REM Get the full path of the directory containing the batch file
set BATCH_DIR=%~dp0

REM Source directory containing modified driver files
set SOURCE_DIR=%BATCH_DIR%

REM Destination directory within the BSP
REM todo: Add command line input instead of hardcoding Debug
set BSP_DEPS_DIR_UART=%BATCH_DIR%..\..\b-l475e-iot01a\Debug\_deps\stm32cubel4-src\Drivers\STM32L4xx_HAL_Driver\Src
set BSP_DEPS_DIR_LED=%BATCH_DIR%..\..\b-l475e-iot01a\Debug\_deps\stm32cubel4-src\Drivers\BSP\B-L475E-IOT01

REM Copy modified LED driver files to BSP _deps folder
xcopy /E /Y "%SOURCE_DIR%\stm32l475e_iot01*" "%BSP_DEPS_DIR_LED%"

REM Check if LED driver files copy operation was successful
if %errorlevel% equ 0 (
    echo Modified LED driver files copied successfully.
) else (
    echo Error: Failed to copy modified LED driver files.
    exit /b 1
)

REM Copy modified UART driver files _deps folder
xcopy /E /Y "%SOURCE_DIR%\stm32l4xx_hal_uart.c" "%BSP_DEPS_DIR_UART%"

REM Check if copy operation was successful
if %errorlevel% equ 0 (
    echo Modified UART driver files copied successfully.
) else (
    echo Error: Failed to copy modified UART driver files.
    exit /b 1
)
