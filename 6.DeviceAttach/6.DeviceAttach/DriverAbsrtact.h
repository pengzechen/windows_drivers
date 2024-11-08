#pragma once


#include <ntddk.h>

typedef struct _DriverDescribe {
	NTSTATUS (*Init)(PDRIVER_OBJECT);
	NTSTATUS (*Delete)(PDRIVER_OBJECT);
	VOID (*SetMajorFuncs)(PDRIVER_OBJECT);
}DriverDescribe, *PDriverDescribe;