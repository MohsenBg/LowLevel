#include <ntddk.h>

// Global variables
PDEVICE_OBJECT MyDevice = NULL;                        // Device object for the driver
UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\MyDevice8586");   // Device name
UNICODE_STRING DeviceSymbolLink = RTL_CONSTANT_STRING(L"\\??\\MyDeviceLink8586"); // Symbolic link for the device
LONG pendingKey = 0;                                    // Tracks the number of pending keyboard events

// Struct for handling keyboard input data
typedef struct _KEYBOARD_INPUT_DATA {
	USHORT UnitId;              // Keyboard unit identifier
	USHORT MakeCode;            // Key make code (scan code)
	USHORT Flags;               // Key flags (e.g., key down, key up)
	USHORT Reserved;            // Reserved for future use
	ULONG  ExtraInformation;    // Additional information (e.g., extended keys)
} KEYBOARD_INPUT_DATA, * PKEYBOARD_INPUT_DATA;

// Device extension structure to store lower device object
typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT LowerDevice;  // Pointer to the lower device object in the device stack
} DEVICE_EXTENSION, * PDEVICE_EXTENSION;

// Driver unload routine
VOID UnloadDriver(PDRIVER_OBJECT DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);

	// If the device exists, detach it and delete resources
	if (MyDevice != NULL) {
		PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)MyDevice->DeviceExtension;
		LARGE_INTEGER interval = { 0 };

		// Detach the lower device from the stack
		IoDetachDevice(deviceExtension->LowerDevice);
		interval.QuadPart = 10 * 1000 * 1000; // 10 seconds delay

		// Wait for pending keyboard events to be processed
		while (pendingKey) {
			KeDelayExecutionThread(KernelMode, FALSE, &interval);
		}

		// Clean up symbolic link and device object
		IoDeleteSymbolicLink(&DeviceSymbolLink);
		IoDeleteDevice(MyDevice);
	}

	// Print driver unload message
	KdPrint(("[KeyboardFilterDriver]: Driver Unloaded.\n"));
}

// Dispatch routine for handling IRPs
NTSTATUS DispatchController(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);

	// Copy the current IRP stack location to the next device in the stack
	PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)MyDevice->DeviceExtension;
	IoCopyCurrentIrpStackLocationToNext(Irp);

	// Call the next driver in the stack
	return IoCallDriver(deviceExtension->LowerDevice, Irp);
}

// Attach the driver to the target device
NTSTATUS AttachDevice(PDRIVER_OBJECT DriverObject) {
	NTSTATUS status;
	PDEVICE_EXTENSION deviceExtension;
	UNICODE_STRING targetDevice = RTL_CONSTANT_STRING(L"\\Device\\KeyboardClass0");

	// Create the device object for the driver
	status = IoCreateDevice(
		DriverObject,
		sizeof(DEVICE_EXTENSION),
		&DeviceName,
		FILE_DEVICE_KEYBOARD,
		0,
		FALSE,
		&MyDevice
	);

	if (!NT_SUCCESS(status)) {
		KdPrint(("[KeyboardFilterDriver]: IoCreateDevice failed (0x%x)\n", status));
		return status;
	}

	// Create a symbolic link to the device for user-mode access
	status = IoCreateSymbolicLink(&DeviceSymbolLink, &DeviceName);
	if (!NT_SUCCESS(status)) {
		KdPrint(("[KeyboardFilterDriver]: IoCreateSymbolicLink failed (0x%x)\n", status));
		IoDeleteDevice(MyDevice);
		return status;
	}

	// Set device flags
	MyDevice->Flags |= DO_BUFFERED_IO;
	MyDevice->Flags &= ~DO_DEVICE_INITIALIZING;

	// Initialize device extension
	deviceExtension = (PDEVICE_EXTENSION)MyDevice->DeviceExtension;
	RtlZeroMemory(deviceExtension, sizeof(DEVICE_EXTENSION));

	// Attach to the target device (keyboard class driver)
	status = IoAttachDevice(MyDevice, &targetDevice, &deviceExtension->LowerDevice);
	if (!NT_SUCCESS(status)) {
		KdPrint(("[KeyboardFilterDriver]: IoAttachDevice failed (0x%x)\n", status));
		IoDeleteSymbolicLink(&DeviceSymbolLink);
		IoDeleteDevice(MyDevice);
		return status;
	}

	// Print success message
	KdPrint(("[KeyboardFilterDriver]: Device attached successfully.\n"));
	return STATUS_SUCCESS;
}

// Completion routine for handling keyboard input data
NTSTATUS ReadComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID context) {
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(context);

	// Define the flags for key states
	CHAR* keyFlags[4] = { "KeyDown", "KeyUp", "E0", "E1" };
	PKEYBOARD_INPUT_DATA keys = (PKEYBOARD_INPUT_DATA)Irp->AssociatedIrp.SystemBuffer;
	int countKeyboardInput = (int)Irp->IoStatus.Information / sizeof(PKEYBOARD_INPUT_DATA);

	// Check if the IRP was successful
	if (NT_SUCCESS(Irp->IoStatus.Status)) {
		// Iterate through the keyboard input data and log the key events
		for (int i = 0; i < countKeyboardInput; i++) {
			KdPrint(("[KeyboardFilterDriver]: The Key Scan Code is %x (%s)\n", keys->MakeCode, keyFlags[keys->Flags]));
		}
	}

	// Mark the IRP as pending if necessary
	if (Irp->PendingReturned) {
		IoMarkIrpPending(Irp);
	}

	// Decrement pending keyboard event counter
	InterlockedDecrement(&pendingKey);
	return Irp->IoStatus.Status;
}

// Dispatch routine for read IRPs
NTSTATUS DispatchRead(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);

	PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)MyDevice->DeviceExtension;

	// Copy the IRP stack location to the next driver
	IoCopyCurrentIrpStackLocationToNext(Irp);

	// Set a completion routine to handle the keyboard input data
	IoSetCompletionRoutine(Irp, ReadComplete, NULL, TRUE, TRUE, TRUE);

	// Increment pending keyboard event counter
	InterlockedIncrement(&pendingKey);

	// Call the next driver in the stack
	return IoCallDriver(deviceExtension->LowerDevice, Irp);
}

// Driver entry point
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS status;

	// Set the driver unload routine
	DriverObject->DriverUnload = UnloadDriver;

	// Set default dispatch routine for all IRP major functions
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		DriverObject->MajorFunction[i] = DispatchController;
	}

	// Set the dispatch routine for read IRPs
	DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;

	// Attach the driver to the target device
	status = AttachDevice(DriverObject);
	if (!NT_SUCCESS(status)) {
		KdPrint(("[KeyboardFilterDriver]: Device Attach failed (0x%x).\n", status));
		return status;
	}

	// Print driver load success message
	KdPrint(("[KeyboardFilterDriver]: Driver loaded successfully.\n"));
	return STATUS_SUCCESS;
}

