
#include "Buffer.h"



CHAR gDriverBuffer[BUFFER_SIZE] = "Hello from the driver!";
PVOID gBuffer = NULL;

NTSTATUS DriverBufferInit()
{
	gBuffer = ExAllocatePoolWithTag(NonPagedPool, BUFFER_SIZE, POOL_TAG);
	if (gBuffer == NULL)
	{

		DbgPrintEx(77, 0, "ExAllocatePoolWithTag() error\r\n");
		// TODO: need a proper reason ...
		return STATUS_INVALID_ADDRESS;
	}

	return STATUS_SUCCESS;
}

VOID DriverBufferDelete()
{
	if (gBuffer) {
		ExFreePool(gBuffer);
		gBuffer = NULL;
	}
}