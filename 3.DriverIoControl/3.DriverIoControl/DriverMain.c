
#include <ntddk.h>

#define DEVICE_NAME L"\\Device\\MyDevice"
#define SYM_LINK_NAME L"\\DosDevices\\MySymLink"

PDEVICE_OBJECT gDeviceObj = NULL;

#define IOCTL_MY_OPERATION CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)


VOID MyDriverUnLoad(PDRIVER_OBJECT pDriver)
{
	// ж������
	UNICODE_STRING symLinkName = RTL_CONSTANT_STRING(SYM_LINK_NAME);
	IoDeleteSymbolicLink(&symLinkName);
	// ж���豸
	IoDeleteDevice(pDriver->DeviceObject);
}


typedef struct _INPUT_DATA {
	int someValue;
}INPUT_DATA, *PINPUT_DATA;

typedef struct _OUTPUT_DATA {
	int someValue;
}OUTPUT_DATA, *POUTPUT_DATA;

/*
* Ӧ�ó������ CreateFile ���豸��ʱ�����������
* I/O Request Packet (IRQ)
*/
NTSTATUS MyDriverCreate(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDevice);
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	DbgPrintEx(77, 0, "[db]: create a device\r\n");
	DbgPrintEx(77, 0, "[db]: My Op(hex): %x\r\n", IOCTL_MY_OPERATION);

	return STATUS_SUCCESS;
}

/*
* Ӧ�ó������ CloseHandle �ر��豸��ʱ�����������
*/
NTSTATUS MyDriverClose(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDevice);
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	DbgPrintEx(77, 0, "[db]: delete a device\r\n");
	return STATUS_SUCCESS;
}

NTSTATUS MyDeviceControl(PDEVICE_OBJECT pDevice, PIRP pIrp)
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

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING regPath)
{
	UNREFERENCED_PARAMETER(regPath);

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
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	// ������������
	status = IoCreateSymbolicLink(&symLinkName, &deviceName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(gDeviceObj);
		return status;
	}

	pDriver->MajorFunction[IRP_MJ_CREATE] = MyDriverCreate;
	pDriver->MajorFunction[IRP_MJ_CLOSE] = MyDriverClose;
	pDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MyDeviceControl;
	pDriver->DriverUnload = MyDriverUnLoad;

	return STATUS_SUCCESS;
}