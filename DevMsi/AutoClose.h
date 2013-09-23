/**
 * Header file for some convenience handle-closing classes.
 *
 * Yes, there are many ways to do this better, including BOOST.
 * But for example code, this should be sufficient to close a few
 * types of handles when the object goes out of scope.
 */
#pragma once
#include <SetupAPI.h>

/**
 * Close HDEVINFO when the object goes out of scope.
 */
class AutoCloseDeviceInfoList {
public:
    AutoCloseDeviceInfoList() : m_handle(INVALID_HANDLE_VALUE) {}
    ~AutoCloseDeviceInfoList() { Close(); }

    bool IsValid() const { return INVALID_HANDLE_VALUE != m_handle; }

    void Close() {
        if ( IsValid() ) {
            SetupDiDestroyDeviceInfoList( m_handle );
            m_handle = INVALID_HANDLE_VALUE;
        }
    }
    const HDEVINFO& operator=( const HDEVINFO that ) {
        Close();
        m_handle = that;
        return m_handle;
    }

    operator HDEVINFO() { return m_handle; }
private:
    HDEVINFO m_handle;
};

/**
 * Close HKEY when the object goes out of scope.
 */
class AutoCloseHKey {
public:
    AutoCloseHKey() : m_hKey((HKEY)INVALID_HANDLE_VALUE) {}
    ~AutoCloseHKey() { Close(); }

    bool IsValid() const { return INVALID_HANDLE_VALUE != (HKEY)m_hKey; }

    void Close() {
        if ( IsValid() && ERROR_SUCCESS == RegCloseKey( m_hKey ) ) {
            m_hKey = (HKEY)INVALID_HANDLE_VALUE;
        }
    }
    operator HKEY() const { return m_hKey; }
    operator HKEY*() { return &m_hKey; }
private:
    HKEY m_hKey;
};

/**
 * Close SC_HANDLE when the object goes out of scope.
 */
class AutoCloseServiceHandle {
public:
    AutoCloseServiceHandle() : m_handle(NULL) {}
    ~AutoCloseServiceHandle() { Close(); }

    bool IsValid() const { return NULL != m_handle; }

    void Close() {
        if ( IsValid() ) {
            SetupDiDestroyDeviceInfoList( m_handle );
            m_handle = NULL;
        }
    }
    const SC_HANDLE& operator=( const SC_HANDLE that ) {
        Close();
        m_handle = that;
        return m_handle;
    }

    operator SC_HANDLE() { return m_handle; }
private:
    SC_HANDLE m_handle;
};
