
#pragma once

#include <ntifs.h>
#include <ntstrsafe.h>
#include <ntddkbd.h>

#define KILLRULE_NTDEVICE_NAME L"\\Device\\KillRuleDrv"
PDEVICE_OBJECT deviceObject = NULL;

typedef struct _MY_EXTENSION {
	PDEVICE_OBJECT lowerDeviceObject;
}MY_EXTENSION;


ULONG PendingCount = 0;

VOID DriverUnload(
	_In_ struct _DRIVER_OBJECT* DriverObject
) {
	UNREFERENCED_PARAMETER(DriverObject);
	IoDetachDevice(((MY_EXTENSION*)deviceObject->DeviceExtension)->lowerDeviceObject);
	IoDeleteDevice(deviceObject);
	
	LARGE_INTEGER lDelay = { 0 };
	lDelay.QuadPart = -10 * 1000 * 1000;
	
	KdPrintEx((77, 0, "DriverUnload PendingCount is %d\n", PendingCount));
	while (PendingCount) {
		KdPrintEx((77, 0, "KeDelayExecutionThread PendingCount is %d\n", PendingCount));
		KeDelayExecutionThread(KernelMode, FALSE, &lDelay);
	}

	KdPrintEx((77, 0, "DriverUnload.\n"));
}

NTSTATUS IrpPass(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
) {
	UNREFERENCED_PARAMETER(DeviceObject);
	IoCopyCurrentIrpStackLocationToNext(Irp);
	IoCallDriver(((MY_EXTENSION*)deviceObject->DeviceExtension)->lowerDeviceObject, Irp);
	return STATUS_SUCCESS;

}

NTSTATUS MyCompletionRoutine(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_ PIRP Irp,
	_In_reads_opt_(_Inexpressible_("varies")) PVOID Context
) {
	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(DeviceObject);
	
	IoGetCurrentIrpStackLocation(Irp);
	// https://learn.microsoft.com/en-us/windows/win32/api/ntddkbd/ns-ntddkbd-keyboard_input_data
	PKEYBOARD_INPUT_DATA data = (PKEYBOARD_INPUT_DATA)Irp->AssociatedIrp.SystemBuffer;
	ULONG_PTR structNum = Irp->IoStatus.Information / sizeof(PKEYBOARD_INPUT_DATA);

	if (Irp->IoStatus.Status == STATUS_SUCCESS) {
		for (ULONG_PTR i = 0; i < structNum; i++) {
			// KdPrintEx((77, 0, "[%d]Preesed key is %d\n", i, data[i].MakeCode));
			DbgPrintEx(77, 0, "(i: %d)numkey: %u, sancode: %x, %s\n", i, structNum, data->MakeCode, data->Flags ? "UP" : "DOWN");
			data++;
		}
	}

	if (Irp->PendingReturned) {
		IoMarkIrpPending(Irp);
	}
	PendingCount--;
	// KdPrintEx((77, 0, "PendingCount is %d\n", PendingCount));
	return Irp->IoStatus.Status;
}

NTSTATUS ReadFileDevice(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
) {
	UNREFERENCED_PARAMETER(DeviceObject);
	IoCopyCurrentIrpStackLocationToNext(Irp);

	IoSetCompletionRoutine(Irp, MyCompletionRoutine, NULL, TRUE, TRUE, TRUE);
	PendingCount++;
	// KdPrintEx((77, 0, "PendingCount is %d\n", PendingCount));

	NTSTATUS status = IoCallDriver(((MY_EXTENSION*)deviceObject->DeviceExtension)->lowerDeviceObject, Irp);
	return status;
}

NTSTATUS AttachToDevice(
	_In_ PDEVICE_OBJECT SourceDevice
)
{
	UNREFERENCED_PARAMETER(SourceDevice);
	KdPrintEx((77, 0, "AttachToDevice is start\n"));
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING KbdDeviceName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardClass0");
	status = IoAttachDevice(deviceObject, &KbdDeviceName, &((MY_EXTENSION*)deviceObject->DeviceExtension)->lowerDeviceObject);
	if (NT_SUCCESS(status)) {
		KdPrintEx((77, 0, "AttachToDevice is success\n"));
	}
	else {
		KdPrintEx((77, 0, "AttachToDevice is failed\n"));
	}
	return status;
}

NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
)
{
	//MiProcessLoaderEntry = (pMiProcessLoaderEntry)0xfffff80534b88ee4; // 这是用 dp 来定位的
	KdPrintEx((77, 0, "DriverEntry\n"));
	//KdBreakPoint();

	UNREFERENCED_PARAMETER(RegistryPath);

	UNICODE_STRING deviceName;

	RtlInitUnicodeString(&deviceName, KILLRULE_NTDEVICE_NAME);

	for (int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) 
		DriverObject->MajorFunction[i] = IrpPass; // 其他的照常

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->MajorFunction[IRP_MJ_READ] = ReadFileDevice; // 要读键盘[设备] 端口的值

	NTSTATUS status = IoCreateDevice(
		DriverObject,
		sizeof(MY_EXTENSION), // 因为要用到，所以要给
		&deviceName,
		FILE_DEVICE_KEYBOARD,
		0,
		TRUE,
		&deviceObject);
	if (!NT_SUCCESS(status)) {
		KdPrintEx((77, 0, "Failed to create device (0x%X)\n", status));
		return status;
	}

	status = AttachToDevice(deviceObject);
	if (NT_SUCCESS(status)) 
		KdPrintEx((77, 0, "Success to attach device (0x%X)\n", status));
	else {
		KdPrintEx((77, 0, "Failed to attach device (0x%X)\n", status));
		IoDeleteDevice(deviceObject);
		return status;
	}

	deviceObject->Flags |= DO_BUFFERED_IO;

	status = STATUS_SUCCESS;

	return status;
}