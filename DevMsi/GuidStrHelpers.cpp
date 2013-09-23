#include "stdafx.h"
#include "AutoClose.h"
#include "CheckResult.h"
#include <string>
#include <Objbase.h>
#include <vector>
#include <SetupAPI.h>
#include <cfgmgr32.h>


std::wstring GUID2Str( __in const GUID&  source )
{
    const int bufferSize(40);
    wchar_t buffer[bufferSize]; // should be more than large enough.
    ZeroMemory(buffer, sizeof(buffer) );

    int len = StringFromGUID2( source, buffer, bufferSize );
    if ( 0 == len ) {
        CheckResult( E_FAIL, "GUID2Str() reported buffer too small.");
    }
    return buffer;
}

GUID Str2GUID( __in const std::wstring&  source )
{
    GUID dest;
    static_assert( sizeof( GUID ) == sizeof( CLSID ), "GUID is not the samesize as a CLSID" );
    HRESULT hr = CLSIDFromString(source.c_str(), (LPCLSID)&dest);
    if ( FAILED( hr ) ) {
        LogResult(hr, "CLSIDFromString(%ls) Failed.", source );
        throw hr;
    }
    return dest;
}

GUID ClassName2GUID( __in const std::wstring& ClassName ) {

    GUID ClassGUID;
    HRESULT hr = E_FAIL;

    if ( ClassName.empty() ) {
        throw std::runtime_error( "Unable to convert empty string from class name to GUID" );
    }

    AutoCloseHKey hKeyClass, hKey;
    LONG lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\Class"), 
        0, KEY_ENUMERATE_SUB_KEYS, hKeyClass );
    if ( ERROR_SUCCESS != lResult ) {
        // Couldn't open the Class key to find key names underneath.
        hr = HRESULT_FROM_WIN32( lResult );
        LogResult( hr, "Unable to open Registry Key HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class" );
        throw hr;
    }

    BOOL found = FALSE;
    wchar_t buffer[MAX_PATH];
    DWORD bufferSize = 0;
    for ( DWORD i = 0; FAILED(hr); ++i ) {

        bufferSize = MAX_PATH; // size of subkeyName in characters (not bytes)
        lResult = RegEnumKeyExW( hKeyClass, i, buffer, &bufferSize, NULL, NULL, NULL, NULL );
        if ( ERROR_SUCCESS != lResult ) {
            hr = HRESULT_FROM_WIN32( lResult );
            if ( ERROR_NO_MORE_ITEMS == lResult ) {
                LogResult( hr, "Class name not found in registry." );
                throw hr;
            } else {
                LogResult( hr, "RegEnumKeyEx() Failed." );
                throw hr;
            }
        }

        lResult = RegOpenKeyExW( hKeyClass, buffer, 0, KEY_QUERY_VALUE, hKey );
        if ( ERROR_SUCCESS != lResult ) {
            hr = HRESULT_FROM_WIN32( lResult );
            LogResult( hr, "Registry key '%ls' could not be opened.", buffer );
        } else {
            wchar_t classBuffer[MAX_PATH];
            DWORD classSize = sizeof(classBuffer); // size in bytes (not characters)
            DWORD dataType = REG_SZ;

            lResult = RegQueryValueExW( hKey, L"Class", NULL, &dataType,
                (LPBYTE)classBuffer, &classSize );
            if ( ERROR_SUCCESS == lResult 
                && REG_SZ == dataType
                && 0 == _wcsicmp( classBuffer, ClassName.c_str() ) 
                ) {
                    try
                    {
                        ClassGUID = Str2GUID( buffer );
                        hr = S_OK;
                    }
                    catch( HRESULT err)
                    {
                        hr = err;
                    }
            }
            hKey.Close();
        }

    } // FOR loop incrementing i.

    if ( FAILED( hr ) ) {
        LogResult( hr, "Unable to find class GUID by name" );
    }
    return ClassGUID;
} // ClassName2GUID

GUID Inf2ClassGUID( __inout std::wstring& pathName, __out std::wstring& classStr ) {

    GUID ClassGUID;
    wchar_t ClassName[1+MAX_CLASS_NAME_LEN];
    std::vector< wchar_t > InfPath( 0 );
    DWORD pathSize = MAX_PATH;
    ClassName[MAX_CLASS_NAME_LEN] = L'\0';

    while ( pathSize > InfPath.size() ) {

        InfPath.resize( 1 + pathSize );

        pathSize = GetFullPathName(pathName.c_str(), static_cast<DWORD>(InfPath.size()) ,&InfPath[0], NULL);

        if ( 0 == pathSize ) {
            HRESULT hr = HRESULT_FROM_WIN32( GetLastError() );
            LogResult( hr, "GetFullPathName('%ls') Failed", pathName.c_str() );
            throw hr;
        }
    }

    //
    // Use the INF File to extract the Class GUID.
    //
    if (!SetupDiGetINFClassW(&InfPath[0],&ClassGUID,ClassName,sizeof(ClassName)/sizeof(ClassName[0]),0))
    {
        HRESULT hr = HRESULT_FROM_WIN32( GetLastError() );
        LogResult( hr, "SetupDiGetINFClass('%ls') Failed", &InfPath[0] );
        throw hr;
    }

    pathName = &InfPath[0];
    classStr = ClassName;
    return ClassGUID;

} //GUID Inf2ClassGUID( __in const std::wstring& pathName )