#include <ntddk.h>
#include <wdf.h>

NTSTATUS KmdfHelloWorldEvtDeviceAdd(_In_ WDFDRIVER Driver,
	_Inout_ PWDFDEVICE_INIT DeviceInit) {

	UNREFERENCED_PARAMETER(Driver);

	NTSTATUS status;
	WDFDEVICE hDevice;

	KdPrintEx((DPFLTR_IHVDRIVER_ID,
		DPFLTR_TRACE_LEVEL, "KmdfHelloWorld: kmdfHelloWorldEvtDeviceAdd\n"));

	status =
		WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &hDevice);

	return status;
};

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegsteryPath) {


	NTSTATUS status = STATUS_SUCCESS;

	WDF_DRIVER_CONFIG config;

	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
		"KmdfHelloWorld: DriverEntry\n"));


	WDF_DRIVER_CONFIG_INIT(&config, KmdfHelloWorldEvtDeviceAdd);


	status = WdfDriverCreate(DriverObject, RegsteryPath,
		WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);

	return status;
};

