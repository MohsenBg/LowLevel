#include <wdm.h>
#include <ntstrsafe.h>
extern "C" {

	// Constants for device type and IOCTL codes
#define FILE_DEVICE_HIDE 0x8000          // Custom device type
#define IOCTL_BASE 0x800                 // Base for IOCTL codes
#define CTL_CODE_HIDE(i)                \
    CTL_CODE(FILE_DEVICE_HIDE, IOCTL_BASE + i, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define DEFAULT_DEVICE_NAME L"\\Device\\STUPID"  // Default device name format
#define DEFAULT_DEVICE_LINK L"\\??\\STUPID"     // Default symbolic link format
#define IOCTL_TEST_START CTL_CODE_HIDE(1)  // IOCTL code for starting the test


	// Function Prototypes
	NTSTATUS DriverAddDevice(_In_ PDRIVER_OBJECT DriverObject, _In_ PDEVICE_OBJECT DeviceObjectInput);
	void WDM_STUPID_DriverUnload(_In_ PDRIVER_OBJECT DriverObject);
	NTSTATUS WDM_STUPID_DeviceCreateRoutine(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp);
	NTSTATUS WDM_STUPID_DeviceCloseRoutine(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp);
	NTSTATUS WDM_STUPID_DeviceShutdownRoutine(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp);
	NTSTATUS WDM_STUPID_DeviceControlRoutine(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp);
	void WDM_STUPID_PrintLn(const char* message);

	// Device extension structure to manage additional device data
	typedef struct _DEVICE_EXTENSION {
		PDEVICE_OBJECT DeviceObject;    // Pointer to the device object
		ULONG DeviceState;              // Device state (e.g., active, inactive)
	} DEVICE_EXTENSION, * PDEVICE_EXTENSION;

	// Max number of devices
	const ULONG MAX_DEVICE = 64;
	static PDEVICE_OBJECT DeviceObjects[MAX_DEVICE];  // Array to store device objects
	static LONG DeviceInstance = 0;  // Variable to keep track of the device instance


	// Driver entry point
	NTSTATUS DriverEntry(
		_In_ PDRIVER_OBJECT DriverObject,
		_In_ PUNICODE_STRING RegistryPath) {

		UNREFERENCED_PARAMETER(RegistryPath);
		WDM_STUPID_PrintLn("Driver Driver Entry Start Loading...");

		NTSTATUS status = STATUS_SUCCESS;

		if (DriverObject->DriverExtension != NULL) {
			// Set up driver and add device functions
			DriverObject->DriverExtension->AddDevice = DriverAddDevice;
		}
		else {
			// Print error if DriverExtension is NULL
			WDM_STUPID_PrintLn("Driver Extension is NULL");
			status = STATUS_UNSUCCESSFUL;
		}

		// Register major functions for device operations
		DriverObject->DriverUnload = WDM_STUPID_DriverUnload;
		DriverObject->MajorFunction[IRP_MJ_CREATE] = WDM_STUPID_DeviceCreateRoutine;
		DriverObject->MajorFunction[IRP_MJ_CLOSE] = WDM_STUPID_DeviceCloseRoutine;
		DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = WDM_STUPID_DeviceShutdownRoutine;
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = WDM_STUPID_DeviceControlRoutine;

		if(NT_SUCCESS(status))
			WDM_STUPID_PrintLn("Driver Entry Load SuccessFully");

		return status;
	}


	// Add a new device
	NTSTATUS DriverAddDevice(
		_In_ PDRIVER_OBJECT DriverObject,
		_In_ PDEVICE_OBJECT DeviceObjectInput
	) {

		UNREFERENCED_PARAMETER(DeviceObjectInput);

		WCHAR deviceNameBuffer[64];
		WCHAR symbolicLinkBuffer[64];
		UNICODE_STRING deviceName;
		UNICODE_STRING symbolicLink;

		// Format the device name with the instance number
		swprintf(deviceNameBuffer, L"%s%d", DEFAULT_DEVICE_NAME, DeviceInstance);
		RtlInitUnicodeString(&deviceName, deviceNameBuffer);

		// Create the device object
		NTSTATUS status = IoCreateDevice(
			DriverObject,
			sizeof(DEVICE_EXTENSION),       // Size of the device extension
			&deviceName,                    // Device name
			FILE_DEVICE_UNKNOWN,            // Device type
			0,                              // Device characteristics
			FALSE,                          // Not exclusive
			&DeviceObjects[DeviceInstance]   // Pointer to the device object to be created
		);

		if (!NT_SUCCESS(status)) {
			// Log error if device creation fails
			WDM_STUPID_PrintLn("IO CreateDevice failed");
			return status;
		}

		// Enable buffered I/O for the device
		DeviceObjects[DeviceInstance]->Flags |= DO_BUFFERED_IO;

		// Format the symbolic link
		swprintf(symbolicLinkBuffer, L"%s%d", DEFAULT_DEVICE_LINK, DeviceInstance);
		RtlInitUnicodeString(&symbolicLink, symbolicLinkBuffer);

		// Create the symbolic link
		status = IoCreateSymbolicLink(&symbolicLink, &deviceName);

		if (!NT_SUCCESS(status)) {
			// Clean up if symbolic link creation fails
			IoDeleteDevice(DeviceObjects[DeviceInstance]);
			WDM_STUPID_PrintLn("IO CreateSymbolicLink failed");
			return status;
		}

		// Increment the device instance counter for the next device
		InterlockedIncrement(&DeviceInstance);

		return status;
	}


	// Unload the driver and clean up resources
	void WDM_STUPID_DriverUnload(_In_ PDRIVER_OBJECT DriverObject) {
		UNREFERENCED_PARAMETER(DriverObject);
		WDM_STUPID_PrintLn("Driver Driver Unload...");

		// Iterate through all devices and clean up their symbolic links and device objects
		for (int i = 0; i < DeviceInstance; i++) {
			WCHAR SymbolicLinkBuffer[64];
			swprintf(SymbolicLinkBuffer, L"%s%d", DEFAULT_DEVICE_LINK, i);
			UNICODE_STRING SymbolicLink;
			RtlInitUnicodeString(&SymbolicLink, SymbolicLinkBuffer);

			// Delete the symbolic link
			IoDeleteSymbolicLink(&SymbolicLink);

			// Delete the device object
			IoDeleteDevice(DeviceObjects[i]);
		}

		// Reset the device instance counter
		DeviceInstance = 0;

		// Log driver unloading message
		WDM_STUPID_PrintLn("Driver Unloaded");
	}

	// Device Create routine (IRP_MJ_CREATE)
	NTSTATUS WDM_STUPID_DeviceCreateRoutine(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp) {
		UNREFERENCED_PARAMETER(DeviceObject);
		UNREFERENCED_PARAMETER(Irp);
		WDM_STUPID_PrintLn("IRP_MJ_CREATE");
		return STATUS_SUCCESS;
	}

	// Device Close routine (IRP_MJ_CLOSE)
	NTSTATUS WDM_STUPID_DeviceCloseRoutine(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp) {
		UNREFERENCED_PARAMETER(DeviceObject);
		UNREFERENCED_PARAMETER(Irp);
		WDM_STUPID_PrintLn("IRP_MJ_CLOSE");
		return STATUS_SUCCESS;
	}

	// Device Shutdown routine (IRP_MJ_SHUTDOWN)
	NTSTATUS WDM_STUPID_DeviceShutdownRoutine(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp) {
		UNREFERENCED_PARAMETER(DeviceObject);
		UNREFERENCED_PARAMETER(Irp);
		WDM_STUPID_PrintLn("IRP_MJ_SHUTDOWN");
		return STATUS_SUCCESS;
	}

	// Device Control routine (IRP_MJ_DEVICE_CONTROL)
	NTSTATUS WDM_STUPID_DeviceControlRoutine(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp) {
		PIO_STACK_LOCATION pIrpStackLocation;
		PDEVICE_EXTENSION deviceExtension;
		NTSTATUS status = STATUS_SUCCESS;
		PVOID inputBuffer;
		ULONG inputBufferLength;
		PVOID outputBuffer;
		ULONG outputBufferLength;
		ULONG ioControlCode;
		PIO_STATUS_BLOCK ioStatus;

		ioStatus = &Irp->IoStatus;
		ioStatus->Status = STATUS_SUCCESS;		// Assume success
		ioStatus->Information = 0;              // Assume nothing returned


		deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
		pIrpStackLocation = IoGetCurrentIrpStackLocation(Irp);

		// Initialize input/output buffers
		inputBuffer = Irp->AssociatedIrp.SystemBuffer;
		outputBuffer = Irp->AssociatedIrp.SystemBuffer;
		inputBufferLength = pIrpStackLocation->Parameters.DeviceIoControl.InputBufferLength;
		outputBufferLength = pIrpStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
		ioControlCode = pIrpStackLocation->Parameters.DeviceIoControl.IoControlCode;



		switch (ioControlCode) {
		case IOCTL_TEST_START:
			// Handle test IOCTL command
			WDM_STUPID_PrintLn("IOCTL called");
			*(PLONG)outputBuffer = (*(PLONG)inputBuffer) * 2;  // Example logic: multiply input by 2
			ioStatus->Information = sizeof(ULONG);  // Return size of output buffer
			break;

		default:
			break;
		}


		Irp->IoStatus.Status = ioStatus->Status;
		Irp->IoStatus.Information = ioStatus->Information;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		return status;
	}
	void WDM_STUPID_PrintLn(const char* message)
	{
		KdPrint(("\n[WDM_STUPID]:%s\n", message));  // Use DbgPrint for debugging
	}
}

