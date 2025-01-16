#include <ntddk.h>


#define DEVICE_SEND CTL_CODE(FILE_DEVICE_UNKNOWN,0x901,METHOD_BUFFERED,FILE_WRITE_DATA)
#define DEVICE_RECEIVE CTL_CODE(FILE_DEVICE_UNKNOWN,0x902,METHOD_BUFFERED,FILE_READ_DATA)

UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\myDevice8585");
UNICODE_STRING DeviceSysmbolLink = RTL_CONSTANT_STRING(L"\\??\\myDeviceLink8585");
PDEVICE_OBJECT MyDeviceObject = NULL;


NTSTATUS DispatchIoCTLControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	PIO_STACK_LOCATION irpStackLocation;
	PVOID buffer;
	ULONG inputBufferLength, outputBufferLength, ioControlCode;
	ULONG informationLength = 0;
	WCHAR* message = L"Data Came From Driver\n";

	// Validate IRP and Device Object
	if (!Irp || !DeviceObject) {
		KdPrint(("\n[WDM_IRP]: Invalid IRP or Device Object.\r\n"));
		return STATUS_INVALID_PARAMETER;
	}

	// Retrieve current stack location
	irpStackLocation = IoGetCurrentIrpStackLocation(Irp);
	buffer = Irp->AssociatedIrp.SystemBuffer;
	inputBufferLength = irpStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	outputBufferLength = irpStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
	ioControlCode = irpStackLocation->Parameters.DeviceIoControl.IoControlCode;

	switch (ioControlCode) {
	case DEVICE_SEND:
		// Handle DEVICE_SEND logic
		if (buffer && inputBufferLength >= sizeof(WCHAR)) {
			ULONG maxBufferLength = (outputBufferLength / sizeof(WCHAR)) - 1;
			KdPrint(("\n[WDM_IRP]: DEVICE_SEND data: %ws.\r\n", (WCHAR*)buffer));
			informationLength = ((ULONG)wcsnlen((WCHAR*)buffer, maxBufferLength - 1) + 1) * sizeof(WCHAR);
		}
		else {
			KdPrint(("\n[WDM_IRP]: Invalid input buffer for DEVICE_SEND.\r\n"));
			status = STATUS_INVALID_PARAMETER;
		}
		break;

	case DEVICE_RECEIVE:
		// Handle DEVICE_RECEIVE logic
		if (outputBufferLength >= sizeof(WCHAR)) {
			if (buffer) {
				ULONG maxMessageLength = (outputBufferLength / sizeof(WCHAR)) - 1;
				wcsncpy((WCHAR*)buffer, message, maxMessageLength);
				((WCHAR*)buffer)[maxMessageLength - 1] = L'\0';
				informationLength = ((ULONG)wcsnlen((WCHAR*)buffer, maxMessageLength - 1) + 1) * sizeof(WCHAR);
				KdPrint(("\n[WDM_IRP]: DEVICE_RECEIVE responded with: %ws.\r\n", (WCHAR*)buffer));
			}
			else {
				KdPrint(("\n[WDM_IRP]: No valid buffer for DEVICE_RECEIVE.\r\n"));
				status = STATUS_INVALID_PARAMETER;
			}
		}
		else {
			KdPrint(("\n[WDM_IRP]: Output buffer too small for DEVICE_RECEIVE.\r\n"));
			status = STATUS_BUFFER_TOO_SMALL;
		}
		break;

	default:
		KdPrint(("\n[WDM_IRP]: Unsupported IoControlCode (0x%x).\r\n", ioControlCode));
		status = STATUS_INVALID_PARAMETER;
		break;
	}

	// Complete the IRP
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = NT_SUCCESS(status) ? informationLength : 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}



NTSTATUS DispatchController(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);

	NTSTATUS status = STATUS_SUCCESS;
	PIO_STACK_LOCATION irpStackLocation = IoGetCurrentIrpStackLocation(Irp);

	if (!Irp) {
		KdPrint(("\n[WDM_IRP]: Invalid IRP.\r\n"));
		return STATUS_INVALID_PARAMETER;
	}

	switch (irpStackLocation->MajorFunction) {
	case IRP_MJ_CREATE:
		KdPrint(("\n[WDM_IRP]: IRP_MJ_CREATE Dispatch.\r\n"));
		break;

	case IRP_MJ_CLOSE:
		KdPrint(("\n[WDM_IRP]: IRP_MJ_CLOSE Dispatch.\r\n"));
		break;

	case IRP_MJ_READ:
		KdPrint(("\n[WDM_IRP]: IRP_MJ_READ Dispatch.\r\n"));
		break;

	default:
		KdPrint(("\n[WDM_IRP]: Unsupported MajorFunction (0x%x).\r\n", irpStackLocation->MajorFunction));
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}


VOID Unload(PDRIVER_OBJECT DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);

	IoDeleteSymbolicLink(&DeviceSysmbolLink);
	IoDeleteDevice(MyDeviceObject);
	KdPrint(("\n[WDM_IRP]: Driver Unload Successfully \r\n"));
}


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS status;
	DriverObject->DriverUnload = Unload;


	status = IoCreateDevice(
		DriverObject,				// Pointer To Driver Object
		0,							// Size of Device Extension
		&DeviceName,				// Pointer To Device Name
		FILE_DEVICE_UNKNOWN,		// Device Type
		FILE_DEVICE_SECURE_OPEN,	// DeviceCharacteristics
		FALSE,						// Exclusive
		&MyDeviceObject				// Pointer To Device Object
	);

	if (!NT_SUCCESS(status))
		KdPrint(("\n[WDM_IRP]: IoCreateDevice Faild: (0x%x) \r\n", status));

	status = IoCreateSymbolicLink(&DeviceSysmbolLink, &DeviceName);

	if (!NT_SUCCESS(status)) {
		KdPrint(("[WDM_IRP]: IoCreateSymbolicLink Faild: (0x%x) \r\n", status));

		if (MyDeviceObject != NULL)
			IoDeleteDevice(MyDeviceObject);

		return status;
	}

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchController;
	}


	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoCTLControl;
	KdPrint(("\n[WDM_IRP]: Driver Load Successfully\r\n"));
	return status;
}