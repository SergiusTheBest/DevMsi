#include "stdafx.h"
#include "devmsi.h"
#include "CheckResult.h"
#include "ciwstring.h"
#include <SetupAPI.h>
#include <cfgmgr32.h>
#include "AutoClose.h"
#include "GuidStrHelpers.h"
#include <newdev.h>

/**
* An enumeration of the valid types of class arguments provided to DoCreateDevnode().
*
* @see DoCreateDevnode
* @see GetClassArgType
*/
typedef enum { 
    evClassName,        //*< The argument is a class name (e.g. "System")
    evClassGuidString,  //*< The argument is a class GUID in string form
    evInfPath,          //*< The argument is a path to a relevant INF file
    evInvalidClassArg   //*< The argument type is unknown
} etClassArgType;

/**
* Examine the classArg and return the argument type.
*
* @param classArg the argument to be inspected.
* @return Returns the type of argument parsed.
*/
etClassArgType GetClassArgType( const std::wstring& classArg ) {

    if ( classArg.empty() ) {
        return evInvalidClassArg;
    }

    if ( classArg.length() == 38 
        && L'{' == classArg[0]
    && L'-' == classArg[9] 
    && L'-' == classArg[14] 
    && L'-' == classArg[19] 
    && L'-' == classArg[24] 
    && L'}' == classArg[37]
    ) {
        return evClassGuidString;
    }

    WIN32_FIND_DATA FindFileData;
    HANDLE handle = FindFirstFile(classArg.c_str(), &FindFileData) ;
    if( handle != INVALID_HANDLE_VALUE ) {
        FindClose( handle );
        return evInfPath;
    }
    return evClassName;

} // etClassArgType GetClassArgType( const std::wstring& classArg )

HRESULT DEVMSI_API DoCreateDevnode( int argc, LPWSTR* argv )
{
    HRESULT hr = E_FAIL;

    try
    {
        wchar_t hwIdList[LINE_LEN+4];
        wchar_t ClassName[MAX_CLASS_NAME_LEN+1];
        AutoCloseDeviceInfoList DeviceInfoList;
        SP_DEVINFO_DATA DeviceInfoData;
        GUID ClassGUID;
        std::wstring classArg, hwidArg, classStr;
        bool classNameValid = false;

        switch( argc )
        {
        case 2:
            classArg = (NULL == argv[0] ? L"" : argv[0]);
            hwidArg = (NULL == argv[1] ? L"" : argv[1]);
            break;
        case 1:
            throw std::runtime_error( "CreateDevnode() requires two parameters, only one provided" );
        case 0:
            throw std::runtime_error( "CreateDevnode() requires two parameters, zero provided" );
        default:
            throw std::runtime_error( "CreateDevnode() requires two parameters, too many provided" );
        }
        LogResult( S_OK, "hwid = '%ls', class = '%ls'.", hwidArg.c_str(), classArg.c_str() );

        etClassArgType classArgType = GetClassArgType( classArg );

        switch ( classArgType ) {
        case evClassName:
            LogResult( S_OK, "Converting '%ls' from Class Name to GUID.", classArg.c_str() );
            ClassGUID = ClassName2GUID( classArg );
            break;
        case evClassGuidString:
            LogResult( S_OK, "Converting '%ls' from String to GUID.", classArg.c_str() );
            ClassGUID = Str2GUID( classArg );
            break;
        case evInfPath:
            LogResult( S_OK, "Converting '%ls' from INF Path to Class GUID.", classArg.c_str() );
            ClassGUID = Inf2ClassGUID( classArg, classStr );
            if ( MAX_CLASS_NAME_LEN < classStr.size() ) {
                hr = E_FAIL;
                LogResult( hr, "Class string '%ls' too long.", classStr.c_str() );
                throw hr;
            }
            wcscpy_s( ClassName, classStr.c_str() );
            classNameValid = true;
            break;
        case evInvalidClassArg:
            throw std::runtime_error( "Unable to determine classArg type." );
            break;
        }

        LogResult( S_OK, "Class GUID %ls will be used.", GUID2Str( ClassGUID ).c_str() );

        // At this point, we have hwidArg and ClassGUID, so we are ready
        // to start working on creating the appropriate device.  This code
        // will be very familiar to those who look at devcon in the WDK.

        //
        // List of hardware ID's must be double zero-terminated
        //
        ZeroMemory(hwIdList,sizeof(hwIdList));
        hr = StringCchCopyW(hwIdList, LINE_LEN, hwidArg.c_str() );
        CheckResult( hr,  "Failed StringCchCopy(hwIdList,hwidArg)");

        if ( !classNameValid ) {
            //
            // Get the class name from the class GUID.
            //
            ClassName[MAX_CLASS_NAME_LEN] = L'\0';
            if (!(SetupDiClassNameFromGuidW( &ClassGUID, ClassName, MAX_CLASS_NAME_LEN, NULL ) ) ) {
                hr = HRESULT_FROM_WIN32( ::GetLastError() );
                CheckResult(hr, "Failed to retrieve class name from provided class GUID");
            }
            LogResult(hr, "SetupDiClassNameFromGuid() returned %ls.", ClassName);
        }

        //
        // Create the container for the to-be-created Device Information Element.
        //
        DeviceInfoList = SetupDiCreateDeviceInfoList(&ClassGUID,0);
        if(DeviceInfoList == INVALID_HANDLE_VALUE)
        {
            hr = HRESULT_FROM_WIN32( ::GetLastError() );
            CheckResult( hr, "Unable to create DeviceInfoList for new device" );
        }
        LogResult(hr, "SetupDiCreateDeviceInfoList() succeeded.");

        //
        // Now create the element.  Unlike DevCon, no INF file was needed
        // since the user provided the class GUID or name.
        //
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        if (!SetupDiCreateDeviceInfo(DeviceInfoList,
            ClassName,
            &ClassGUID,
            NULL,
            0,
            DICD_GENERATE_ID,
            &DeviceInfoData))
        {
            hr = HRESULT_FROM_WIN32( ::GetLastError() );
            CheckResult( hr, "Unable to create DeviceInfoData element" );
        }
        LogResult(hr, "SetupDiCreateDeviceInfo() succeeded.");

        //
        // Add the HardwareID to the Device's HardwareID property.
        //
        if(!SetupDiSetDeviceRegistryProperty(DeviceInfoList,
            &DeviceInfoData,
            SPDRP_HARDWAREID,
            (LPBYTE)hwIdList,
            (lstrlen(hwIdList)+1+1)*sizeof(TCHAR)))
        {
            hr = HRESULT_FROM_WIN32( ::GetLastError() );
            CheckResult( hr, "Unable to set HardwareID Property" );
        }
        LogResult(hr, "SetupDiSetDeviceRegistryProperty() succeeded.");

        //
        // Transform the registry element into an actual devnode
        // in the PnP HW tree.
        //
        if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
            DeviceInfoList,
            &DeviceInfoData))
        {
            hr = HRESULT_FROM_WIN32( ::GetLastError() );
            CheckResult( hr, "Unable to call the class installer to create the devnode" );
        }
        LogResult(hr, "SetupDiCallClassInstaller() succeeded.");

        // We should now have a device in Device Manager
        // that doesn't have a driver.  

        if ( evInfPath == classArgType ) {

            // In this case, we have the path to the INF file.  So
            // we can "do things the DevCon way" and just install
            // the INF driver for our created device.
            BOOL rebootRequired = FALSE;

            BOOL success = UpdateDriverForPlugAndPlayDevices(
                NULL,
                hwidArg.c_str(),
                classArg.c_str(),
                0,
                &rebootRequired
                );
            if ( !success ) {
                hr = HRESULT_FROM_WIN32( ::GetLastError() );
                CheckResult( hr, "Unable to UpdateDriverForPlugAndPlayDevices()" );
            }
        } else {
            // Go and simulate "Scan For Hardware Changes"
            // ( http://support.microsoft.com/kb/259697?wa=wsignin1.0 )
            DEVINST     devInst;
            CONFIGRET   status;

            status = CM_Locate_DevNode(&devInst, NULL, CM_LOCATE_DEVNODE_NORMAL);

            if (status != CR_SUCCESS) {
                CheckResult( E_FAIL, "CM_Locate_DevNode() failed" );
            }

            status = CM_Reenumerate_DevNode(devInst, 0);

            if (status != CR_SUCCESS) {
                CheckResult( E_FAIL, "CM_Reenumerate_DevNode() failed" );
            }

        }

        LogResult( hr, "DoCreateDevnode() Complete.");
    }
    catch( HRESULT& _error )
    {
        hr = _error;
    }
    catch( const std::exception& _error )
    {
        hr = E_FAIL;
        LogResult( hr, _error.what() );
    }
    catch( ... )
    {
        hr = E_FAIL;
        LogResult( hr, "Unhandled C++ exception" );
    }

    return hr;
} // HRESULT DEVMSI_API DoCreateDevnode( int argc, LPWSTR* argv )
