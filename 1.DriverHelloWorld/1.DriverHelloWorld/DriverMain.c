


#include <ntifs.h>

VOID DriverUnload(PDRIVER_OBJECT pDriver)
{
	UNREFERENCED_PARAMETER(pDriver);
	DbgPrintEx(77, 0, "[db]: Driver Unload\r\n");
	DbgBreakPoint();
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pReg)
{

	UNREFERENCED_PARAMETER(pReg);

	DbgPrintEx(77, 0, "[db]: Driver Loaded\r\n");
	DbgBreakPoint();

	pDriver->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}