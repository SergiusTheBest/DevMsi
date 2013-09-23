#pragma once
#include <string>

/**
 * Convert a class name to a GUID.
 *
 * When provided a class name, this function will search the registry
 * to find the GUID associated with the setup class.  The search will
 * be case-insensitive:  "System" is the same as "system".
 *
 * An exception will be thrown on error.
 *
 * @param ClassName The setup class name to be searched for (e.g. "system")
 * @return Returns the matching GUID from the system registry.
 */
GUID ClassName2GUID( __in const std::wstring& ClassName );


/**
 * Convert a GUID to a string.
 *
 * An exception will be thrown on error.
 *
 * @param source The GUID to be converted (e.g. {BDD12CB1-D607-4A2A-82B5-F200B4FFAEF4})
 * @return Returns the data in string format.
 */
std::wstring GUID2Str( __in const GUID&  source );

/**
 * Convert a string to a GUID.
 *
 * An exception will be thrown on error.
 *
 * @param source The string to be converted (e.g. "{BDD12CB1-D607-4A2A-82B5-F200B4FFAEF4}")
 * @return Returns the data in GUID format.
 */
GUID Str2GUID( __in const std::wstring&  source );

/**
 * Extracts the class GUID and string from an INF file.
 *
 * An exception will be thrown on error.
 *
 * @param pathName The path to the INF file to be used.  This path will be modified to
 *                 reflect the full path of the INF file.
 * @param classStr The class string from the INF file.
 * @return Returns the class name in GUID format.
 */
GUID Inf2ClassGUID( __inout std::wstring& pathName, __out std::wstring& classStr );
