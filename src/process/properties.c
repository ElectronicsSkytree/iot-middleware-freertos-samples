/* Copyright (c) Skytree B.V. All rights reserved. */

/* Local includes */
#include "properties.h"

/* Azure JSON includes */
#include "azure_iot_json_reader.h"

/**
 * @brief pre-processors for json tokens
*/
#define iotPROPERTIES_DESIRED               ( "desired" )
#define iotPROPERTIES_REPORTED              ( "reported" )
#define iotPROPERTIES_IDENTIFIERS           ( "identifiers" )
#define iotPROPERTIES_VERSION               ( "version" )
#define iotPROPERTIES_TELEMETRY_INTERVAL    ( "interval" )
#define iotPROPERTIES_SERIAL_NUMBER         ( "serial_number" )
#define iotPROPERTIES_CCU                   ( "ccu" )
#define iotPROPERTIES_UNITS                 ( "units" )
#define iotPROPERTIES_TANK                  ( "tank" )
#define iotPROPERTIES_SLOT                  ( "slot" )
#define iotPROPERTIES_CARTRIDGES            ( "cartridges" )

// For getting the length of the telemetry names
#define lengthof( x )                       ( sizeof( x ) - 1 )

/**
 * @brief Macro to validate 
*/
#define validateForError( x, y )                                    \
    if( ( x ) == 0 )                                                \
    {                                                               \
        vLoggingPrintf( "Error (%d) parsing system properties"      \
            "[%s:%d] \r\n", (int32_t)y, __func__, __LINE__ );       \
        return FAILED;                                              \
    }


/**
 * @brief Skip an entire json property section including children
 * 
 * @return 0 for failure and 1 for success
 */
static error_t SkipProperty( AzureIoTJSONReader_t* xReader )
{
    validateForError( xReader != NULL, false );

    AzureIoTResult_t xResult;
    AzureIoTJSONTokenType_t xTokenType;

    xResult = AzureIoTJSONReader_NextToken( xReader );
    validateForError( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONReader_TokenType( xReader, &xTokenType );
    validateForError( xResult == eAzureIoTSuccess, xResult );

    if( xTokenType == eAzureIoTJSONTokenBEGIN_OBJECT )
    {
        xResult = AzureIoTJSONReader_SkipChildren( xReader );
        validateForError( xResult == eAzureIoTSuccess, xResult );
    }

    return SUCCEEDED;
}
/*-----------------------------------------------------------*/

/**
 * @brief Read property and validate as per type
 */
static error_t ReadProperty( AzureIoTJSONReader_t* xReader, AzureIoTJSONTokenType_t type )
{
    validateForError( xReader != NULL, false );

    AzureIoTResult_t xResult;
    AzureIoTJSONTokenType_t xTokenType;

    xResult = AzureIoTJSONReader_NextToken( xReader );
    validateForError( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONReader_TokenType( xReader, &xTokenType );
    validateForError( xResult == eAzureIoTSuccess, xResult );
    validateForError( xTokenType == type, xTokenType );

    return SUCCEEDED;
}

static error_t IsNextEndOfObject( AzureIoTJSONReader_t* xReader )
{
    validateForError( xReader != NULL, false );

    AzureIoTResult_t xResult;
    AzureIoTJSONTokenType_t xTokenType;

    xResult = AzureIoTJSONReader_NextToken( xReader );
    validateForError( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONReader_TokenType( xReader, &xTokenType );
    validateForError( xResult == eAzureIoTSuccess, xResult );

    if( xTokenType == eAzureIoTJSONTokenEND_OBJECT )
    {
        return SUCCEEDED;
    }
    
    return FAILED;
}

static error_t IsNextEndOfArray( AzureIoTJSONReader_t* xReader )
{
    validateForError( xReader != NULL, false );

    AzureIoTResult_t xResult;
    AzureIoTJSONTokenType_t xTokenType;

    xResult = AzureIoTJSONReader_NextToken( xReader );
    validateForError( xResult == eAzureIoTSuccess, xResult );
    xResult = AzureIoTJSONReader_TokenType( xReader, &xTokenType );
    validateForError( xResult == eAzureIoTSuccess, xResult );

    if( xTokenType == eAzureIoTJSONTokenEND_ARRAY )
    {
        return SUCCEEDED;
    }
    
    return FAILED;
}

/**
 * @brief Fill properties for the units
 */
static error_t ReadCartridges( AzureIoTJSONReader_t* xReader, uint32_t unit_index, SYSTEM_Properties_t* system_properties )
{
    validateForError( xReader != NULL, false);
    validateForError( system_properties != NULL, false);

    AzureIoTResult_t xResult;
    uint32_t usBytesCopied = 0;
    uint8_t number_of_cartridges = 0;

    validateForError( ReadProperty( xReader, eAzureIoTJSONTokenBEGIN_ARRAY ) == SUCCEEDED, FAILED );
    validateForError( ReadProperty( xReader, eAzureIoTJSONTokenBEGIN_OBJECT ) == SUCCEEDED, FAILED );

    do
    {
        while( FAILED == IsNextEndOfObject( xReader ) )
        {
            if( AzureIoTJSONReader_TokenIsTextEqual( xReader, ( uint8_t* )iotPROPERTIES_SERIAL_NUMBER, lengthof( iotPROPERTIES_SERIAL_NUMBER ) ) )
            {
                validateForError( ReadProperty( xReader, eAzureIoTJSONTokenSTRING ) == SUCCEEDED, FAILED );
                xResult = AzureIoTJSONReader_GetTokenString( xReader, (uint8_t*)(system_properties->units[unit_index].cartridges[number_of_cartridges].identity.serial_number),
                                                            sizeof(system_properties->units[unit_index].cartridges[number_of_cartridges].identity.serial_number), &usBytesCopied );
                validateForError( xResult == eAzureIoTSuccess, xResult );
            }
            else if( AzureIoTJSONReader_TokenIsTextEqual( xReader, ( uint8_t* )iotPROPERTIES_SLOT, lengthof( iotPROPERTIES_SLOT ) ) )
            {
                validateForError( ReadProperty( xReader, eAzureIoTJSONTokenNUMBER ) == SUCCEEDED, FAILED );
                xResult = AzureIoTJSONReader_GetTokenInt32( xReader, &(system_properties->units[unit_index].cartridges[number_of_cartridges].slot_number) );
                validateForError( xResult == eAzureIoTSuccess, xResult );
            }
            else{
                // Skip as we are not interested in others
                validateForError( SkipProperty(xReader) == SUCCEEDED, FAILED );
            }
        }

        number_of_cartridges++;
    } while( FAILED == IsNextEndOfArray( xReader ) );

    system_properties->units[unit_index].number_of_cartridges = number_of_cartridges;

    return SUCCEEDED;
}

/**
 * @brief Fill properties for the units
 */
static error_t ReadUnits( AzureIoTJSONReader_t* xReader, SYSTEM_Properties_t* system_properties )
{
    validateForError( xReader != NULL, false);
    validateForError( system_properties != NULL, false);

    AzureIoTResult_t xResult;
    uint32_t usBytesCopied = 0;
    uint8_t number_of_units = 0;

    validateForError( ReadProperty( xReader, eAzureIoTJSONTokenBEGIN_ARRAY ) == SUCCEEDED, FAILED );
    validateForError( ReadProperty( xReader, eAzureIoTJSONTokenBEGIN_OBJECT ) == SUCCEEDED, FAILED );

    do
    {
        while( FAILED == IsNextEndOfObject( xReader ) )
        {
            // validateForError( ReadProperty( &xReader, eAzureIoTJSONTokenPROPERTY_NAME ) == SUCCEEDED, FAILED );

            if( AzureIoTJSONReader_TokenIsTextEqual( xReader, ( uint8_t* )iotPROPERTIES_SERIAL_NUMBER, lengthof( iotPROPERTIES_SERIAL_NUMBER ) ) )
            {
                validateForError( ReadProperty( xReader, eAzureIoTJSONTokenSTRING ) == SUCCEEDED, FAILED );
                xResult = AzureIoTJSONReader_GetTokenString( xReader, (uint8_t*)(system_properties->units[number_of_units].identity.serial_number),
                                                            sizeof(system_properties->units[number_of_units].identity.serial_number), &usBytesCopied );
                validateForError( xResult == eAzureIoTSuccess, xResult );
            }
            else if( AzureIoTJSONReader_TokenIsTextEqual( xReader, ( uint8_t* )iotPROPERTIES_CARTRIDGES, lengthof( iotPROPERTIES_CARTRIDGES ) ) )
            {
                validateForError( ReadCartridges( xReader, number_of_units, system_properties ) == SUCCEEDED, FAILED );
            }
            else{
                // Skip as we are not interested in others
                validateForError( SkipProperty(xReader) == SUCCEEDED, FAILED );
            }
        }

        number_of_units++;
    } while( FAILED == IsNextEndOfArray( xReader ) );

    system_properties->number_of_units = number_of_units;

    return SUCCEEDED;
}

/**
 * @brief Read desired properties from the json reader
 * 
 * All the system (CCU, Units, Tank) properties are fetched from
 * the desired prperties payload.
 * 
 * @todo Currently we are interested only in system properties.
 *       Any other details in the desired properties are ignored.
 * 
 * @param[in] xReader json reader handle
 * @param[out] system_properties parsed system properties to be filled
 * 
 * @return 0 for failure and 1 for success
*/
static error_t ReadDesiredProperties( AzureIoTJSONReader_t* xReader, SYSTEM_Properties_t* system_properties )
{
    AzureIoTResult_t xResult;
    uint32_t usBytesCopied = 0;

    validateForError( ReadProperty( xReader, eAzureIoTJSONTokenBEGIN_OBJECT ) == SUCCEEDED, FAILED );

    /**
     * @note This is imp - we loop till we find end of desired properties.
     *       So, any change please take care of it.
    */
    while( FAILED == IsNextEndOfObject( xReader ) )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( xReader, ( uint8_t* )iotPROPERTIES_TELEMETRY_INTERVAL, lengthof( iotPROPERTIES_TELEMETRY_INTERVAL ) ) )
        {
            validateForError( ReadProperty( xReader, eAzureIoTJSONTokenNUMBER ) == SUCCEEDED, FAILED );
            xResult = AzureIoTJSONReader_GetTokenInt32( xReader, &(system_properties->telemetry_interval) );
            validateForError( xResult == eAzureIoTSuccess, xResult );
        }
        else if( AzureIoTJSONReader_TokenIsTextEqual( xReader, (uint8_t*)iotPROPERTIES_IDENTIFIERS, lengthof( iotPROPERTIES_IDENTIFIERS ) ) )
        {                
            // Now work on everything inside "identifiers"
            validateForError( ReadProperty( xReader, eAzureIoTJSONTokenBEGIN_OBJECT ) == SUCCEEDED, FAILED );

            while( FAILED == IsNextEndOfObject( xReader ) )
            {
                if( AzureIoTJSONReader_TokenIsTextEqual( xReader, (uint8_t*)iotPROPERTIES_VERSION, lengthof( iotPROPERTIES_VERSION ) ) )
                {
                    validateForError( ReadProperty( xReader, eAzureIoTJSONTokenNUMBER ) == SUCCEEDED, FAILED );
                    xResult = AzureIoTJSONReader_GetTokenDouble( xReader, &(system_properties->version) );
                    validateForError( xResult == eAzureIoTSuccess, xResult );
                }
                else if( AzureIoTJSONReader_TokenIsTextEqual( xReader, (uint8_t*)iotPROPERTIES_CCU, lengthof( iotPROPERTIES_CCU ) ) )
                {
                    validateForError( ReadProperty( xReader, eAzureIoTJSONTokenBEGIN_OBJECT ) == SUCCEEDED, FAILED );
                    validateForError( ReadProperty( xReader, eAzureIoTJSONTokenPROPERTY_NAME ) == SUCCEEDED, FAILED );

                    if ( AzureIoTJSONReader_TokenIsTextEqual( xReader, (uint8_t*)iotPROPERTIES_SERIAL_NUMBER, lengthof( iotPROPERTIES_SERIAL_NUMBER ) ) )
                    {
                        validateForError( ReadProperty( xReader, eAzureIoTJSONTokenSTRING ) == SUCCEEDED, FAILED );
                        xResult = AzureIoTJSONReader_GetTokenString( xReader, (uint8_t*)(system_properties->ccu.identity.serial_number),
                                                                    sizeof(system_properties->ccu.identity.serial_number), &usBytesCopied );
                        validateForError( xResult == eAzureIoTSuccess, xResult );
                    }
                    else{
                        // Skip as we are not interested in others
                        validateForError( SkipProperty(xReader) == SUCCEEDED, FAILED );
                    }

                    validateForError( ReadProperty( xReader, eAzureIoTJSONTokenEND_OBJECT ) == SUCCEEDED, FAILED );
                }
                else if( AzureIoTJSONReader_TokenIsTextEqual( xReader, (uint8_t*)iotPROPERTIES_TANK, lengthof( iotPROPERTIES_TANK ) ) )
                {
                    validateForError( ReadProperty( xReader, eAzureIoTJSONTokenBEGIN_OBJECT ) == SUCCEEDED, FAILED );
                    validateForError( ReadProperty( xReader, eAzureIoTJSONTokenPROPERTY_NAME ) == SUCCEEDED, FAILED );

                    if ( AzureIoTJSONReader_TokenIsTextEqual( xReader, (uint8_t*)iotPROPERTIES_SERIAL_NUMBER, lengthof( iotPROPERTIES_SERIAL_NUMBER ) ) )
                    {
                        validateForError( ReadProperty( xReader, eAzureIoTJSONTokenSTRING ) == SUCCEEDED, FAILED );
                        xResult = AzureIoTJSONReader_GetTokenString( xReader, (uint8_t*)(system_properties->tank.identity.serial_number),
                                                                    sizeof(system_properties->tank.identity.serial_number), &usBytesCopied );
                        validateForError( xResult == eAzureIoTSuccess, xResult );
                    }
                    else{
                        // Skip as we are not interested in others
                        validateForError( SkipProperty(xReader) == SUCCEEDED, FAILED );
                    }

                    validateForError( ReadProperty( xReader, eAzureIoTJSONTokenEND_OBJECT ) == SUCCEEDED, FAILED );
                }
                else if( AzureIoTJSONReader_TokenIsTextEqual( xReader, (uint8_t*)iotPROPERTIES_UNITS, lengthof( iotPROPERTIES_UNITS ) ) )
                {
                    validateForError( ReadUnits( xReader, system_properties ) == SUCCEEDED, FAILED );
                }
                else{
                    // Skip as we are not interested in others
                    validateForError( SkipProperty(xReader) == SUCCEEDED, FAILED );
                }
            }
        }
        else{
            // Skip as we are not interested in others
            validateForError( SkipProperty(xReader) == SUCCEEDED, FAILED );
        }
    }   // Endof 2nd while for desired properties

    return SUCCEEDED;   // return as we got desired properties
}

/**
 * @brief Parse desired properties
 * 
 * All the system (CCU, Units, Tank) properties are fetched from
 * the desired prperties payload.
 * 
 * @todo Currently we are interested only in system properties.
 *       Any other details in the desired properties are ignored.
 * 
 * @param[in] payload desired properties payload received from IoTHub
 * @param[in] payload_length length of the payload
 * @param[out] system_properties parsed system properties to be filled
 * 
 * @return 0 for failure and 1 for success
*/
error_t ParseDesiredProperties( const uint8_t* payload, uint32_t payload_length, SYSTEM_Properties_t* system_properties )
{
    validateForError( payload != NULL, false);
    validateForError( system_properties != NULL, false);

    AzureIoTResult_t xResult;
    AzureIoTJSONReader_t xReader;

    xResult = AzureIoTJSONReader_Init( &xReader, payload, payload_length );
    validateForError( xResult == eAzureIoTSuccess, xResult );

    return ReadDesiredProperties( &xReader, system_properties );
}

/**
 * @brief Parse system properties
 * 
 * All the system (CCU, Units, Tank) properties are fetched from
 * the prperties payload. The details are mainly present in the
 * desired properties section.
 * 
 * @todo Currently we are interested only in system properties.
 *       Any other details in the desired properties are ignored.
 * 
 * @param[in] payload desired properties payload received from IoTHub
 * @param[in] payload_length length of the payload
 * @param[out] system_properties parsed system properties to be filled
 * 
 * @return 0 for failure and 1 for success
*/
error_t ParseSystemProperties( const uint8_t* payload, uint32_t payload_length, SYSTEM_Properties_t* system_properties )
{
    validateForError( payload != NULL, false);
    validateForError( system_properties != NULL, false);

    AzureIoTResult_t xResult;
    AzureIoTJSONReader_t xReader;

    xResult = AzureIoTJSONReader_Init( &xReader, payload, payload_length );
    validateForError( xResult == eAzureIoTSuccess, xResult );

    validateForError( ReadProperty( &xReader, eAzureIoTJSONTokenBEGIN_OBJECT ) == SUCCEEDED, FAILED );

    // Loop levels: desired, reported etc
    while( FAILED == IsNextEndOfObject( &xReader ) )
    {
        // Desired properties
        if( AzureIoTJSONReader_TokenIsTextEqual( &xReader, ( uint8_t* )iotPROPERTIES_DESIRED, lengthof( iotPROPERTIES_DESIRED ) ) )
        {
            validateForError( ReadDesiredProperties( &xReader, system_properties ) == SUCCEEDED, FAILED );

            return SUCCEEDED;
        }
        else{
            // Skip as it is not desired peoperties
            validateForError( SkipProperty(&xReader) == SUCCEEDED, FAILED );
        }
    }   // Endof main while for all properties

    return FAILED;
}
/*-----------------------------------------------------------*/
