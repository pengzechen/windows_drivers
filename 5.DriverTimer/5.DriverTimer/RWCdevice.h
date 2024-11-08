#pragma once


#include "DriverAbsrtact.h"

#define DEVICE_NAME L"\\Device\\RWCDevice"
#define SYM_LINK_NAME L"\\DosDevices\\RWCSymLink"

// extern PDEVICE_OBJECT gDeviceObj;

extern DriverDescribe RWCDriverDes;



static NTSTATUS DeviceInit(PDRIVER_OBJECT);

// 需要上层传递一个 driver 指针
// ！！符号链接失败的话，device 不能被正确释放
static NTSTATUS DeviceDelete(PDRIVER_OBJECT);

static VOID DeviceSetMajorFunctions(PDRIVER_OBJECT pDriver);

static NTSTATUS MyDriverCreate(PDEVICE_OBJECT pDevice, PIRP pIrp);

static NTSTATUS MyDriverClose(PDEVICE_OBJECT pDevice, PIRP pIrp);

/* ReadFile */
static NTSTATUS MyDriverRead(PDEVICE_OBJECT pDevice, PIRP pIrp);

/* WriteFile */
static NTSTATUS MyDriverWrite(PDEVICE_OBJECT pDevice, PIRP pIrp);

/* IoControl */
static NTSTATUS MyDeviceControl(PDEVICE_OBJECT pDevice, PIRP pIrp);

