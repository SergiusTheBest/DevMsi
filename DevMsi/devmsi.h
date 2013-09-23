/**
 * Function prototypes for external "C" interfaces into the DLL.
 *
 * This project builds a "hybrid" DLL that will work either from
 * a MSI Custom Action environment or from an external C program.
 * The former routes through "C" interface functions defined in 
 * CustomAction.def.  The latter uses the interfaces defined here.
 *
 * This header is suitable for inclusion by a project wanting to
 * call these methods.  Note that _DEVMSI_EXPORTS should not be
 * defined for the accessing application source code.
 */
#pragma once

#ifdef _DEVMSI_EXPORTS
#  define DEVMSI_API __declspec(dllexport)
#else
#  define DEVMSI_API __declspec(dllimport)
#endif

/**
 * Create a new device node in device manager.
 *
 * For this function, the following are valid values for
 * the argv and argc parameters:
 *
 * argc MUST be 2.
 *
 * argv[0] can be any of the following:
 * 1)  The path to the device INF file to be used (e.g. ".\foo.inf")
 * 2)  The class name associated with the device (e.g. "System")
 * 3)  The GUID for the class name associated with the device 
 *     (e.g. "{4D36E97D-E325-11CE-BFC1-08002BE10318}" )
 *
 * argv[1] is the device name to be created, e.g. "\root\foo"
 *
 * @param argc  The count of valid arguments in argv.
 * @param argv  An array of string arguments for the function.
 * @return Returns an HRESULT indicating success or failure.
 */
HRESULT DEVMSI_API DoCreateDevnode( int argc, LPWSTR* argv );


/**
 * Remove device node(s) from device manager.
 *
 * This method will search and remove any matching device nodes
 * from the system (e.g. Device Manager).
 *
 * For this function, the following are valid values for
 * the argv and argc parameters:
 *
 * argc MUST be 1.
 *
 * argv[0] is the device name to be removed, e.g. "\root\foo"
 *
 * @param argc  The count of valid arguments in argv.
 * @param argv  An array of string arguments for the function.
 * @return Returns an HRESULT indicating success or failure.
 */
HRESULT DEVMSI_API DoRemoveDevnode( int argc, LPWSTR* argv );


/**
 * Remove a service from the system.
 *
 * This method will search and remove a matching
 * service from the system, based on name.
 *
 * For this function, the following are valid values for
 * the argv and argc parameters:
 *
 * argc MUST be 1.
 *
 * argv[0] is the service name to be deleted.
 *
 * @param argc  The count of valid arguments in argv.
 * @param argv  An array of string arguments for the function.
 * @return Returns an HRESULT indicating success or failure.
 */
HRESULT DEVMSI_API DoRemoveService( int argc, LPWSTR* argv );

/**
 *  Standardized function prototype for DevMsi.
 *
 *  Functions in DevMsi can be called through the MSI Custom
 *  Action DLL or through an external C program.  Both
 *  methods expect to wrap things into this function prototype.
 *
 *  As a result, all functions defined in this header should
 *  conform to this function prototype.
 */
typedef HRESULT DEVMSI_API (*CUSTOM_ACTION_ARGC_ARGV)(
    int argc, LPWSTR* argv
    );
