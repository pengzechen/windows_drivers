#pragma once

#include <ntddk.h>

#define POOL_TAG 'Tag1'
#define BUFFER_SIZE 256

extern PVOID gBuffer;
extern CHAR gDriverBuffer[];

extern NTSTATUS DriverBufferInit();
extern VOID DriverBufferDelete();