
#include <ntddk.h>

#define DECIVE_NAME L"\\Device\\MyDevice"
#define SYM_LINK_NAME L"\\DosDevices\\MySymLink"

PDEVICE_OBJECT gDeviceObject = NULL;

#define BUFFER_SIZE 256

CHAR gDriverBuffer[BUFFER_SIZE] = "Hello from the driver!";


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

// ReadFile
NTSTATUS MyDriverRead(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDevice);

	DbgBreakPoint();
	
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

// WriteFile
NTSTATUS MyDriverWrite(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDevice);

	DbgBreakPoint();

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



VOID MyDriverUnLoad(PDRIVER_OBJECT pDriver)
{
	UNICODE_STRING symLinkName = RTL_CONSTANT_STRING(SYM_LINK_NAME);
	IoDeleteSymbolicLink(&symLinkName);
	IoDeleteDevice(pDriver->DeviceObject);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath)
{
	UNREFERENCED_PARAMETER(registryPath);

	NTSTATUS status;
	UNICODE_STRING deviceName = RTL_CONSTANT_STRING(DECIVE_NAME);
	UNICODE_STRING symLinkName = RTL_CONSTANT_STRING(SYM_LINK_NAME);
	DbgBreakPoint();

	// �����豸����
	status = IoCreateDevice(
		driverObject,
		0,
		&deviceName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&gDeviceObject
	);

	if (!NT_SUCCESS(status)) {
		return status;
	}

	gDeviceObject->Flags |= DO_BUFFERED_IO;

	// ������������
	status = IoCreateSymbolicLink(&symLinkName, &deviceName);
	if (!NT_SUCCESS(status)) {
		IoDeleteDevice(gDeviceObject);
		return status;
	}

	driverObject->MajorFunction[IRP_MJ_CREATE] = MyDriverCreate;
	driverObject->MajorFunction[IRP_MJ_CLOSE] = MyDriverClose;
	driverObject->MajorFunction[IRP_MJ_READ] = MyDriverRead;
	driverObject->MajorFunction[IRP_MJ_WRITE] = MyDriverWrite;
	driverObject->DriverUnload = MyDriverUnLoad;

	return STATUS_SUCCESS;

}