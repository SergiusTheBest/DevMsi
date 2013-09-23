// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#include "targetver.h"

#define _DEVMSI_EXPORTS
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <strsafe.h>
#include <msiquery.h>
#include <Shellapi.h>


void LogResult(
    __in HRESULT hr,
    __in_z __format_string PCSTR fmt, ...
    );
