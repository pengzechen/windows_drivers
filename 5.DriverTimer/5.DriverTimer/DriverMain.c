
#include <ntddk.h>
#include "Buffer.h"
#include "Timer.h"

#include "RWCdevice.h"
#include "FilterDevice.h"
#include "DriverAbsrtact.h"

static PDriverDescribe gDriverDes = NULL;

/* Driver Unload Defination */
VOID MyDriverUnLoad(PDRIVER_OBJECT pDriver)
{
	DriverBufferDelete();
	TimerDelete();

	gDriverDes->Delete(pDriver);
}

/* Driver Entry Defination */
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING registryPath)
{
	UNREFERENCED_PARAMETER(registryPath);

	NTSTATUS status;
	
	status = DriverBufferInit();
	if (!NT_SUCCESS(status))
		return status;
	
	TimerInit();

	gDriverDes = &RWCDriverDes;
	// gDriverDes = &FilterKBDDriverDes;

	// DbgBreakPoint();
	status = gDriverDes->Init(pDriver);
	if (!NT_SUCCESS(status))
		return status;
	
	// �����豸�Ļص�����
	gDriverDes->SetMajorFuncs(pDriver);
	
	pDriver->DriverUnload = MyDriverUnLoad;

	return STATUS_SUCCESS;

}