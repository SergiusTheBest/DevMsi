#include "stdafx.h"
#include "devmsi.h"
#include "CheckResult.h"
#include "AutoClose.h"
#include "ciwstring.h"


HRESULT DEVMSI_API DoRemoveService( int argc, LPWSTR* argv )
{
    HRESULT hr = S_OK;

    try
    {
        ci_wstring serviceName;
        AutoCloseServiceHandle schSCManager, schService;
        switch( argc )
        {
        case 1:
            serviceName = ( NULL == argv[0] ? L"" : argv[0] );
            break;
        case 0:
            throw std::runtime_error( "DoRemoveService() requires one parameter, zero provided" );
        default:
            throw std::runtime_error( "DoRemoveService() requires one parameter, too many provided" );
        }
        LogResult( S_OK, "Entered DoRemoveService('%ls').", serviceName.c_str() );

        // This code largely taken from the MSDN article "Deleting a Service"
        // http://msdn.microsoft.com/en-us/library/windows/desktop/ms682571(v=vs.85).aspx

        // Get a handle to the SCM database. 

        schSCManager = OpenSCManager( 
            NULL,                    // local computer
            NULL,                    // ServicesActive database 
            SC_MANAGER_ALL_ACCESS);  // full access rights 

        if (NULL == schSCManager) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CheckResult(hr, "OpenSCManager() failed.");
        }

        // Get a handle to the service.

        schService = OpenServiceW( 
            schSCManager,           // SCM database 
            serviceName.c_str(),    // name of service 
            DELETE);                // need delete access 

        if (schService == NULL)
        { 
            DWORD lastError = ::GetLastError();
            hr = HRESULT_FROM_WIN32(lastError);
            if ( ERROR_SERVICE_DOES_NOT_EXIST == lastError ) {

                LogResult( hr, "Service '%ls' does not exist, so it will not be deleted.", serviceName.c_str());
                LogResult( S_OK, "DoRemoveService() Complete.");
                return S_OK;
            }
            LogResult(hr, "OpenService('%ls') failed.", serviceName.c_str());
            throw hr;
        }

        // Delete the service.

        LogResult( hr, "Service '%ls' opened.", serviceName.c_str());
        if (! DeleteService(schService) ) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CheckResult(hr, "DeleteService() failed.");
        }

        LogResult( hr, "Service '%ls' deleted.", serviceName.c_str());
        LogResult( hr, "DoRemoveService() Complete.");
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
} // HRESULT DEVMSI_API DoRemoveService( int argc, LPWSTR* argv )
