/*============================
Driver Control Class (SCM way)
============================*/
#include "../header/stdafx.h"  // Precompiled header
#pragma comment(lib,"advapi32.lib")  // Linking to advapi32.lib for Windows API functions like service management

#include "../header/cDrvCtrl.h"  // Include the class definition header file

#define LOG_LAST_ERROR()  // Define a macro for logging the last error (should be implemented)

//--------------------------------------------------------------------------------//
// Function to install a service (driver) in the Service Control Manager (SCM)
BOOL WINAPI InstallService(
	_In_ LPCSTR ServiceName,         // Service name
	_In_ LPCSTR DisplayName,         // Display name for the service
	_In_ LPCSTR szPath)             // Path to the driver executable
{
	// Open the Service Control Manager to manage services
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCManager)  // Error if failed to open SCM
	{
		LOG_LAST_ERROR();
		return FALSE;
	}

	// Create a new service in the SCM
	SC_HANDLE hService = CreateServiceA(
		hSCManager,                     // SCM handle
		ServiceName,                    // Service name
		DisplayName,                    // Display name
		SERVICE_ALL_ACCESS,             // Access rights
		SERVICE_KERNEL_DRIVER,          // Service type (kernel driver)
		SERVICE_DEMAND_START,           // Service start type (demand start)
		SERVICE_ERROR_NORMAL,           // Error control type
		szPath,                         // Path to the driver
		NULL, NULL, NULL, NULL, NULL    // Other parameters (unused)
	);

	if (!hService)  // Error if service creation failed
	{
		LOG_LAST_ERROR();
		return FALSE;
	}

	// Close the service and SCM handles after installation
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);

	return TRUE;  // Return true if service was successfully installed
}
//--------------------------------------------------------------------------------//
// Function to remove a service from the Service Control Manager
BOOL WINAPI RemoveService(
	_In_ LPCSTR ServiceName)  // Service name to be removed
{
	// Open the SCM
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCManager)  // Error if failed to open SCM
	{
		LOG_LAST_ERROR();
		return FALSE;
	}

	// Open the service to be deleted
	SC_HANDLE hService = OpenServiceA(hSCManager, ServiceName, DELETE);
	if (!hService)  // Error if service not found
	{
		LOG_LAST_ERROR();
		CloseServiceHandle(hSCManager);
		return FALSE;
	}

	// Delete the service
	if (!DeleteService(hService))
	{
		LOG_LAST_ERROR();
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);
		return FALSE;
	}

	// Close the service and SCM handles after deletion
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	return TRUE;  // Return true if service was successfully removed
}
//--------------------------------------------------------------------------------//
// Function to start the driver service
BOOL WINAPI StartDrvService(LPCSTR ServiceName)
{
	// Open the SCM
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCManager)  // Error if failed to open SCM
	{
		LOG_LAST_ERROR();
		return FALSE;
	}

	// Open the service to be started
	SC_HANDLE hService = OpenServiceA(hSCManager, ServiceName, SERVICE_START);
	if (!hService)  // Error if service not found
	{
		LOG_LAST_ERROR();
		CloseServiceHandle(hSCManager);
	}

	// Start the service
	if (!StartService(hService, 0, NULL))
	{
		LOG_LAST_ERROR();
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);
		return FALSE;
	}

	// Close the service and SCM handles after starting
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	return TRUE;  // Return true if the service was successfully started
}
//---------------------------------------------------------------------------------//
// Function to stop the driver service
BOOL WINAPI StopService(LPCSTR ServiceName)
{
	// Open the SCM
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	SERVICE_STATUS svcsta = { 0 };  // Initialize the service status structure
	if (!hSCManager)  // Error if failed to open SCM
	{
		LOG_LAST_ERROR();
		return FALSE;
	}

	// Open the service to be stopped
	SC_HANDLE hService = OpenServiceA(hSCManager, ServiceName, SERVICE_STOP);
	if (!hService)  // Error if service not found
	{
		LOG_LAST_ERROR();
		CloseServiceHandle(hSCManager);
		return FALSE;
	}

	// Stop the service
	if (!ControlService(hService, SERVICE_CONTROL_STOP, &svcsta))
	{
		LOG_LAST_ERROR();
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);
		return FALSE;
	}

	// Close the service and SCM handles after stopping
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	return TRUE;  // Return true if the service was successfully stopped
}
//----------------------------------------------------------------------------------//
// Functions implemented in the cDrvCtrl class to interact with driver services
BOOL cDrvCtrl::Install(
	_In_ PCHAR pSysPath,           // Path to the driver executable
	_In_ PCHAR pServiceName,       // Service name
	_In_ PCHAR pDisplayName)       // Display name for the service
{
	// Install the service using the InstallService function
	if (!InstallService(pServiceName, pDisplayName, pSysPath))
	{
		LOG_LAST_ERROR();
		return FALSE;
	}
	return TRUE;
}
//----------------------------------------------------------------------------------//
// Function to start the driver service using the StartDrvService function
BOOL cDrvCtrl::Start(
	_In_ PCHAR pServiceName)       // Service name to be started
{
	if (!StartDrvService(pServiceName))
	{
		LOG_LAST_ERROR();
		return FALSE;
	}
	return TRUE;
}
//----------------------------------------------------------------------------------//
// Function to stop the driver service using the StopService function
BOOL cDrvCtrl::Stop(
	_In_ PCHAR pServiceName)       // Service name to be stopped
{
	if (!StopService(pServiceName))
	{
		LOG_LAST_ERROR();
		return FALSE;
	}
	return TRUE;
}
//----------------------------------------------------------------------------------//
// Function to remove the driver service using the RemoveService function
BOOL cDrvCtrl::Remove(
	_In_ PCHAR pServiceName)       // Service name to be removed
{
	if (!RemoveService(pServiceName))
	{
		LOG_LAST_ERROR();
		return FALSE;
	}
	return TRUE;
}
//----------------------------------------------------------------------------------//
// Function for sending IOCTL commands to the driver
BOOL cDrvCtrl::IoControl(PCHAR SymbolicNames,
	DWORD dwIoCode, PVOID InBuff, DWORD InBuffLen,
	PVOID OutBuff, DWORD OutBuffLen, DWORD* RealRetBytes)
{
	DWORD dw;
	BOOL   b;
	// Open the device to communicate with the driver
	HANDLE hDriver = CreateFileA(SymbolicNames, GENERIC_READ | GENERIC_WRITE, 0, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (!hDriver)
	{
		return FALSE;
	}

	// Send the IOCTL request to the driver
	b = DeviceIoControl(hDriver, dwIoCode, InBuff, InBuffLen, OutBuff, OutBuffLen, &dw, NULL);

	// Return the number of bytes read/written if requested
	if (RealRetBytes)
		*RealRetBytes = dw;

	// Close the handle to the driver
	CloseHandle(hDriver);
	return b;  // Return the result of the IOCTL request
}

//----------------------------------------------------------------------------------//
// Function to generate a control code for the IOCTL request
DWORD cDrvCtrl::CTL_CODE_GEN(DWORD lngFunction)
{
	// Generate a control code based on the function number
	return (FILE_DEVICE_UNKNOWN * 65536) | (FILE_ANY_ACCESS * 16384) | (lngFunction * 4) | METHOD_BUFFERED;
}
