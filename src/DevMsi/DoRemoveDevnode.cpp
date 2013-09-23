#include "stdafx.h"
#include "devmsi.h"
#include "CheckResult.h"
#include <vector>
#include "ciwstring.h"
#include "AutoClose.h"
#include <cfgmgr32.h>

/**
 * An array of wide strings that do case-insensitive comparisons.
 */
typedef std::vector<ci_wstring> ci_wstringList;

/**
 * Return a device registry property as a list of strings.
 *
 * Provided a device, interrogate it with SetupDiGetDeviceRegistryProperty() and return
 * an array of strings from the registry.  
 *
 * This method will handle both REG_SZ and REG_MULTI_SZ values.  If the data type
 * in the registry is something else, then an error is not returned, but the
 * list of strings is empty.
 *
 * If the registry property does not exist for the device, an error is not returned,
 * but the list of strings is empty.
 *
 * For an unexpected error, an exception will be thrown.
 *
 * @param items The output list of items.  It will be cleared prior to use.
 * @param Devs  The HDEVINFO to be passed to SetupDiGetDeviceRegistryProperty.
 * @param DevInfo the SP_DEVINFO_DATA to be passed to SetupDiGetDeviceRegistryProperty.
 * @param Prop The property to be retrieved from SetupDiGetDeviceRegistryProperty.
 */
void GetDeviceRegistryProperty(__out ci_wstringList& items, __in HDEVINFO Devs, __in SP_DEVINFO_DATA& DevInfo, __in DWORD Prop) {

    DWORD reqSize =0;
    DWORD dataType = REG_NONE;
    items.clear();

    BOOL status = SetupDiGetDeviceRegistryPropertyW( Devs, &DevInfo, Prop, &dataType, NULL, 0, &reqSize );
    if ( !status ) {
        DWORD lastError = GetLastError();
        switch ( lastError ) {
        case ERROR_INSUFFICIENT_BUFFER:
            break;
        case ERROR_INVALID_DATA:
            // According to MSDN,
            // "SetupDiGetDeviceRegistryProperty returns the ERROR_INVALID_DATA 
            // error code if the requested property does not exist for a device 
            // or if the property data is not valid."
            return;
        default:
            LogResult( HRESULT_FROM_WIN32( lastError ), "Size check for SetupDiGetDeviceRegistryProperty() failed" );
            return;
        }
    }

    std::auto_ptr<BYTE> buffer ( new BYTE[++reqSize] );
    wchar_t* ptr = (wchar_t*)buffer.get();

    status = SetupDiGetDeviceRegistryPropertyW( Devs, &DevInfo, Prop, &dataType, buffer.get(), reqSize, &reqSize );
    if ( !status ) {
        HRESULT hr = HRESULT_FROM_WIN32( GetLastError() );
        CheckResult( hr, "Data fetch for SetupDiGetDeviceRegistryProperty() failed" );
    }

    switch ( dataType ) {
    case REG_SZ:
        while ( L'\0' != *ptr ) {
            items.push_back( ptr );
            ptr += items.back().size();
        }
        break;
    case REG_MULTI_SZ:
        while ( L'\0' != *ptr ) {
            items.push_back( ptr );
            ptr += 1 + items.back().size();
        }
        break;
    default:
        // Invalid data type, there's nothing to do here.
        LogResult( S_OK, "Invalid registry property data type %d, ignored.", dataType );
    }

}

HRESULT DEVMSI_API DoRemoveDevnode( int argc, LPWSTR* argv )
{
    HRESULT hr = E_FAIL;

    try
    {
        AutoCloseDeviceInfoList devs;    
        SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail = { sizeof(SP_DEVINFO_LIST_DETAIL_DATA)};
        SP_DEVINFO_DATA devInfo = { sizeof(SP_DEVINFO_DATA) };
        DWORD devIndex = 0;
        DWORD lastError= ERROR_SUCCESS;
        ci_wstring devNodeName;

        switch( argc )
        {
        case 1:
            devNodeName = ( NULL == argv[0] ? L"" : argv[0] );
            break;
        case 0:
            throw std::runtime_error( "DoRemoveDevnode() requires one parameter, zero provided" );
        default:
            throw std::runtime_error( "DoRemoveDevnode() requires one parameter, too many provided" );
        }
        LogResult( S_OK, "Entered RemoveDevnode('%ls').", devNodeName.c_str() );

        devs = SetupDiGetClassDevsEx(
            NULL, NULL, NULL, 
            DIGCF_ALLCLASSES | DIGCF_PRESENT,
            NULL, NULL, NULL);
        if ( INVALID_HANDLE_VALUE == devs ) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CheckResult(hr, "SetupDiGetClassDevsEx(DIGCF_ALLCLASSES | DIGCF_PRESENT) failed.");
        }

        if(!SetupDiGetDeviceInfoListDetail(devs,&devInfoListDetail)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CheckResult(hr, "SetupDiGetDeviceInfoListDetail() failed.");
        }

        for ( devIndex = 0; SetupDiEnumDeviceInfo( devs, devIndex, &devInfo ); ++devIndex ) {
            TCHAR devID[MAX_DEVICE_ID_LEN];
            ci_wstringList hwIds, compatIds;
            bool found = false;
            //
            // determine instance ID
            //
            if(CR_SUCCESS == CM_Get_Device_ID_Ex(devInfo.DevInst, devID, 
                MAX_DEVICE_ID_LEN, 0, devInfoListDetail.RemoteMachineHandle) ) {
                     GetDeviceRegistryProperty(hwIds, devs,devInfo,SPDRP_HARDWAREID);
                     GetDeviceRegistryProperty(compatIds, devs,devInfo,SPDRP_COMPATIBLEIDS);

                    if ( !hwIds.empty() ) {
                        for ( auto iter = hwIds.begin(); iter != hwIds.end(); ++iter ) {
                            if ( *iter == devNodeName ) {
                                LogResult( S_OK, "Device '%ls' matched SPDRP_HARDWAREID '%ls'."
                                    , devNodeName.c_str(), iter->c_str() );
                                found = true;
                            }
                        }
                    }
                    if ( !compatIds.empty() ) {
                        for ( auto iter = compatIds.begin(); iter != compatIds.end(); ++iter ) {
                            if ( *iter == devNodeName ) {
                                LogResult( S_OK, "Device '%ls' matched SPDRP_COMPATIBLEID '%ls'."
                                    , devNodeName.c_str(), iter->c_str() );
                                found = true;
                            }
                        }
                    }

                    if ( found ) {
                        SP_REMOVEDEVICE_PARAMS rmdParams;
                        rmdParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
                        rmdParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
                        rmdParams.Scope = DI_REMOVEDEVICE_GLOBAL;
                        rmdParams.HwProfile = 0;

                        if(!SetupDiSetClassInstallParams(devs,&devInfo,&rmdParams.ClassInstallHeader,sizeof(rmdParams)) ) {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                            CheckResult(hr, "SetupDiSetClassInstallParams() failed.");
                        }
                        if(!SetupDiCallClassInstaller(DIF_REMOVE,devs,&devInfo)) {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                            CheckResult(hr, "SetupDiCallClassInstaller(DIF_REMOVE) failed.");
                        }

                        hr = S_OK;
                        LogResult(hr, "Device '%ls' removed.", devNodeName.c_str() );
                    }
            }
        } // for loop on devIndex
        lastError = GetLastError();
        if ( ERROR_NO_MORE_ITEMS == lastError ) {
            LogResult ( HRESULT_FROM_WIN32( lastError ), "Matching device not found, no device(s) removed." );
            hr = S_OK;
        } else {
            hr = HRESULT_FROM_WIN32(lastError);
            CheckResult(hr, "SetupDiEnumDeviceInfo() failed.");
        }

        LogResult( hr, "DoRemoveDevnode() Complete.");
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
} // HRESULT DEVMSI_API DoRemoveDevnode( int argc, LPWSTR* argv )
