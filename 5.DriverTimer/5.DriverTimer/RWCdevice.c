
#include "DriverAbsrtact.h"
#include "RWCdevice.h"

#include "Dto.h"
#include "IoControl.h"
#include "Buffer.h"

static PDEVICE_OBJECT gDeviceObj = NULL;

// ��Ҫ�ϲ㴫��һ�� driver ָ��
static NTSTATUS DeviceInit(PDRIVER_OBJECT pDriver)
{
	NTSTATUS status;
	UNICODE_STRING deviceName = RTL_CONSTANT_STRING(DEVICE_NAME);
	UNICODE_STRING symLinkName = RTL_CONSTANT_STRING(SYM_LINK_NAME);

	// �����豸����
	status = IoCreateDevice(
		pDriver,
		0,
		&deviceName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&gDeviceObj
	);

	if (!NT_SUCCESS(status)) {
		return status;
	}

	gDeviceObj->Flags |= DO_BUFFERED_IO;

	// ������������
	status = IoCreateSymbolicLink(&symLinkName, &deviceName);
	if (!NT_SUCCESS(status)) {
		IoDeleteDevice(gDeviceObj);
		return status;
	}

	DbgPrintEx(77, 0, "[db]: RWCDriver init ok... \r\n");
	return STATUS_SUCCESS;
}

// ��Ҫ�ϲ㴫��һ�� driver ָ��
// ������������ʧ�ܵĻ���device ���ܱ���ȷ�ͷ�
static NTSTATUS DeviceDelete(PDRIVER_OBJECT pDriver)
{
	// ������ɾ���������ӣ���ɾ���豸
	UNICODE_STRING symLinkName = RTL_CONSTANT_STRING(SYM_LINK_NAME);
	NTSTATUS st = IoDeleteSymbolicLink(&symLinkName);
	if (!NT_SUCCESS(st))
		return st;
	IoDeleteDevice(pDriver->DeviceObject);
	DbgPrintEx(77, 0, "[db]: RWCDriver delete ok ...\r\n");
	return STATUS_SUCCESS;
}

// ��Ҫ�ϲ㴫��һ�� driver ָ��
static VOID DeviceSetMajorFunctions(PDRIVER_OBJECT pDriver)
{
	pDriver->MajorFunction[IRP_MJ_CREATE] = MyDriverCreate;
	pDriver->MajorFunction[IRP_MJ_CLOSE] = MyDriverClose;
	pDriver->MajorFunction[IRP_MJ_READ] = MyDriverRead;
	pDriver->MajorFunction[IRP_MJ_WRITE] = MyDriverWrite;
	pDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MyDeviceControl;
}

DriverDescribe RWCDriverDes = {
	.Init = DeviceInit, 
	.Delete = DeviceDelete, 
	.SetMajorFuncs = DeviceSetMajorFunctions 
};

/*
* Ӧ�ó������ CreateFile ���豸��ʱ�����������, I/O Request Packet (IRP)
*/
static NTSTATUS MyDriverCreate(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDevice);
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	DbgPrintEx(77, 0, "[db]: create a device\r\n");
	return STATUS_SUCCESS;
}

/*
* Ӧ�ó������ CloseHandle �ر��豸��ʱ�����������
*/
static NTSTATUS MyDriverClose(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDevice);
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	DbgPrintEx(77, 0, "[db]: delete a device\r\n");
	return STATUS_SUCCESS;
}

/* ReadFile */
static NTSTATUS MyDriverRead(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDevice);

	// DbgBreakPoint();

	// get stack
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG length = stack->Parameters.Read.Length;
	ULONG bytesToRead = (length < BUFFER_SIZE) ? length : BUFFER_SIZE;

	DbgPrintEx(77, 0, "[db]: driver send \"%s\" to user\r\n", gDriverBuffer);
	// �����ݴ� ���������� ���Ƶ� �û�������
	if (pIrp->AssociatedIrp.SystemBuffer != NULL)
		RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, gDriverBuffer, bytesToRead);

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = bytesToRead;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

/* WriteFile */
static NTSTATUS MyDriverWrite(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDevice);

	// DbgBreakPoint();

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG length = stack->Parameters.Write.Length;
	ULONG bytesToWrite = (length < BUFFER_SIZE) ? length : BUFFER_SIZE;

	DbgPrintEx(77, 0, "[db]: user send \"%s\" to driver\r\n", pIrp->AssociatedIrp.SystemBuffer);
	// �� �û������� ���Ƶ� ����������
	if (pIrp->AssociatedIrp.SystemBuffer != NULL)
		RtlCopyMemory(gDriverBuffer, pIrp->AssociatedIrp.SystemBuffer, bytesToWrite);

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = bytesToWrite;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

/* IoControl */
static NTSTATUS MyDeviceControl(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDevice);
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	NTSTATUS status = STATUS_SUCCESS;
	ULONG_PTR info = 0;

	switch (stack->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_MY_OPERATION: {
		// ���� IOCTL_MY_OPERATION �߼�
		// ��黺��������
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(INPUT_DATA))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		// �õ��û����͵�ֵ
		PINPUT_DATA inputData = (PINPUT_DATA)pIrp->AssociatedIrp.SystemBuffer;
		DbgPrintEx(77, 0, "[db]: input data %d\r\n", inputData->someValue);

		RtlCopyMemory(gBuffer, inputData, sizeof(INPUT_DATA));
		DbgPrintEx(77, 0, "[db]: (MemoryBufferWithTag) %d\r\n", *(int*)gBuffer);

		// ���ز������
		// pIrp->IoStatus.Information = sizeof(OUTPUT_DATA);
		POUTPUT_DATA outputData = (POUTPUT_DATA)pIrp->AssociatedIrp.SystemBuffer;
		outputData->someValue = 123;

		info = sizeof(OUTPUT_DATA);

		break;
	}
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = info;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return status;
}

