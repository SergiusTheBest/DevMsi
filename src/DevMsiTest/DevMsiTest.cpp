// DevMsiTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "..\DevMsi\devmsi.h"


int _tmain(int argc, _TCHAR* argv[])
{
    int result = -1;
    if ( argc < 2 ) {
        return result;
    }
    LPCTSTR opName  = argv[1];
    argc -=2;
    argv +=2;
    if ( !_tcsicmp( opName, TEXT("create") ) ) {
        result = SUCCEEDED( DoCreateDevnode( argc, argv ) )
            ? 0 : -1;
    }
    if ( !_tcsicmp( opName, TEXT("remove") ) ) {
        result = SUCCEEDED( DoRemoveDevnode( argc, argv ) )
            ? 0 : -1;
    }
    if ( !_tcsicmp( opName, TEXT("remService") ) ) {
        result = SUCCEEDED( DoRemoveService( argc, argv ) )
            ? 0 : -1;
    }
	return result;
}

