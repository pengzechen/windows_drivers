
#include "DriverAbsrtact.h"
#include "RWCdevice.h"

#include "Dto.h"
#include "IoControl.h"
#include "Buffer.h"

static PDEVICE_OBJECT gDeviceObj = NULL;

// 需要上层传递一个 driver 指针
static NTSTATUS DeviceInit(PDRIVER_OBJECT pDriver)
{
	NTSTATUS status;
	UNICODE_STRING deviceName = RTL_CONSTANT_STRING(DEVICE_NAME);
	UNICODE_STRING symLinkName = RTL_CONSTANT_STRING(SYM_LINK_NAME);

	// 创建设备对象
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

	// 创建符号链接
	status = IoCreateSymbolicLink(&symLinkName, &deviceName);
	if (!NT_SUCCESS(status)) {
		IoDeleteDevice(gDeviceObj);
		return status;
	}

	DbgPrintEx(77, 0, "[db]: RWCDriver init ok... \r\n");
	return STATUS_SUCCESS;
}

// 需要上层传递一个 driver 指针
// ！！符号链接失败的话，device 不能被正确释放
static NTSTATUS DeviceDelete(PDRIVER_OBJECT pDriver)
{
	// 这里先删除符号链接，再删除设备
	UNICODE_STRING symLinkName = RTL_CONSTANT_STRING(SYM_LINK_NAME);
	NTSTATUS st = IoDeleteSymbolicLink(&symLinkName);
	if (!NT_SUCCESS(st))
		return st;
	IoDeleteDevice(pDriver->DeviceObject);
	DbgPrintEx(77, 0, "[db]: RWCDriver delete ok ...\r\n");
	return STATUS_SUCCESS;
}

// 需要上层传递一个 driver 指针
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
* 应用程序调用 CreateFile 打开设备的时候会调这个函数, I/O Request Packet (IRP)
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
* 应用程序调用 CloseHandle 关闭设备的时候会调这个函数
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
	// 将数据从 驱动缓存区 复制到 用户缓存区
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
	// 从 用户缓存区 复制到 驱动缓存区
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
		// 处理 IOCTL_MY_OPERATION 逻辑
		// 检查缓冲区长度
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(INPUT_DATA))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		// 拿到用户发送的值
		PINPUT_DATA inputData = (PINPUT_DATA)pIrp->AssociatedIrp.SystemBuffer;
		DbgPrintEx(77, 0, "[db]: input data %d\r\n", inputData->someValue);

		RtlCopyMemory(gBuffer, inputData, sizeof(INPUT_DATA));
		DbgPrintEx(77, 0, "[db]: (MemoryBufferWithTag) %d\r\n", *(int*)gBuffer);

		// 返回操作结果
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

