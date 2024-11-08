#pragma once

#include "DriverAbsrtact.h"

#define FILTER_DEVICE_NAME L"\\Device\\RWCDevice"
#define FILTER_SYM_LINK_NAME L"\\DosDevices\\RWCSymLinks"

extern DriverDescribe FilterKBDDriverDes;


static NTSTATUS FilterDriverMajorFuncDefault(PDEVICE_OBJECT pDevice, PIRP pIrp);

static NTSTATUS FilterDeviceRead(PDEVICE_OBJECT pDevice, PIRP pIrp);

static NTSTATUS PnPDispatch(PDEVICE_OBJECT pDevice, PIRP pIrp);

static NTSTATUS PowerDispatch(PDEVICE_OBJECT pDevice, PIRP pIrp);



static NTSTATUS AttachDevice(PDRIVER_OBJECT pDriver);
static VOID DeAttach(PDEVICE_OBJECT pDevice);


/* 未文档化的对象 */

extern NTKERNELAPI NTSTATUS ObReferenceObjectByName(
	IN PUNICODE_STRING ObjectName,
	IN ULONG Attributes,
	IN PACCESS_STATE PassedAccessState OPTIONAL,
	IN ACCESS_MASK DesiredAccess OPTIONAL,
	IN POBJECT_TYPE ObjectType,
	IN KPROCESSOR_MODE AccessMode,
	IN OUT PVOID ParseContext OPTIONAL,
	OUT PVOID *Object
);

extern POBJECT_TYPE *IoDriverObjectType;

typedef struct _KEYBOARD_INPUT_DATA {
	USHORT UnitId;
	USHORT MakeCode;
	USHORT Flags;
	USHORT Reserved;
	ULONG  ExtraInformation;
} KEYBOARD_INPUT_DATA, *PKEYBOARD_INPUT_DATA;