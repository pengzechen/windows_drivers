
#include <ntddk.h>

#include "DriverAbsrtact.h"
#include "FilterDevice.h"

static PDriverDescribe gDriverDes = NULL;

/* Driver Unload Defination */
VOID MyDriverUnLoad(PDRIVER_OBJECT pDriver)
{

	gDriverDes->Delete(pDriver);
}

/* Driver Entry Defination */
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING registryPath)
{
	UNREFERENCED_PARAMETER(registryPath);

	NTSTATUS status;

	gDriverDes = &FilterKBDDriverDes;

	// DbgBreakPoint();
	status = gDriverDes->Init(pDriver);
	if (!NT_SUCCESS(status))
		return status;
	
	// �����豸�Ļص�����
	gDriverDes->SetMajorFuncs(pDriver);
	
	pDriver->DriverUnload = MyDriverUnLoad;

	return STATUS_SUCCESS;

}