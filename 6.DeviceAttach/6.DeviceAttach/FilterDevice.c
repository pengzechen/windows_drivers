
#include "FilterDevice.h"

ULONG PendingCount = 0;

// 需要上层传递一个 driver 指针
static NTSTATUS DeviceInit(PDRIVER_OBJECT pDriver)
{
	UNREFERENCED_PARAMETER(pDriver);
	return STATUS_SUCCESS;
}

// 需要上层传递一个 driver 指针
static NTSTATUS DeviceDelete(PDRIVER_OBJECT pDriver)
{

	PDEVICE_OBJECT pDevice;
	pDevice = pDriver->DeviceObject;
	while (pDevice)
	{
		DeAttach(pDevice);
		pDevice = pDevice->NextDevice;
	}

	LARGE_INTEGER lDelay = { 0 };
	lDelay.QuadPart = -10 * 1000 * 500;

	KdPrintEx((77, 0, "DriverUnload PendingCount is %d\n", PendingCount));
	while (PendingCount) {
		KdPrintEx((77, 0, "KeDelayExecutionThread PendingCount is %d\n", PendingCount));
		KeDelayExecutionThread(KernelMode, FALSE, &lDelay);
	}

	pDriver->DeviceObject = NULL;

	return STATUS_SUCCESS;
}

// 需要上层传递一个 driver 指针
static VOID DeviceSetMajorFunctions(PDRIVER_OBJECT pDriver)
{
	for (ULONG i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		pDriver->MajorFunction[i] = FilterDriverMajorFuncDefault;

	pDriver->MajorFunction[IRP_MJ_READ] = FilterDeviceRead;

	AttachDevice(pDriver);
}

DriverDescribe FilterKBDDriverDes = {
	.Init = DeviceInit,
	.Delete = DeviceDelete,
	.SetMajorFuncs = DeviceSetMajorFunctions
};


typedef struct _DEV_EXTENSION {
	ULONG_PTR Size;

	PDEVICE_OBJECT LowDevice;
	PDEVICE_OBJECT FilterDevice;
	PDEVICE_OBJECT TargetDevice;

	KSPIN_LOCK IoRequestSpinLock;
	KEVENT	IoInProgressEvent;
	PIRP pIrp;
}DEV_EXTENSION, *PDEV_EXTENSION;

static NTSTATUS DevExtInit(PDEV_EXTENSION devExt, PDEVICE_OBJECT filterDevice, PDEVICE_OBJECT targetDevice, PDEVICE_OBJECT lowDevice)
{
	memset(devExt, 0, sizeof(DEV_EXTENSION));
	devExt->FilterDevice = filterDevice;
	devExt->TargetDevice = targetDevice;
	devExt->LowDevice = lowDevice;
	devExt->Size = sizeof(DEV_EXTENSION);

	KeInitializeSpinLock(&devExt->IoRequestSpinLock);
	KeInitializeEvent(&devExt->IoInProgressEvent, NotificationEvent, FALSE);
	return STATUS_SUCCESS;
}

BOOLEAN CancelIrp(PIRP pIrp)
{
	IoSetCancelRoutine(pIrp, NULL);
	pIrp->IoStatus.Status = STATUS_CANCELLED;
	pIrp->IoStatus.Information = 0;
	return TRUE;
}

static VOID DeAttach(PDEVICE_OBJECT pDevice)
{
	DbgPrintEx(77, 0, "[db]: deattach: %p\r\n", pDevice);
	// DbgBreakPoint();
	PDEV_EXTENSION devExt;
	devExt = (PDEV_EXTENSION)pDevice->DeviceExtension;

	IoDetachDevice(devExt->TargetDevice);
	devExt->TargetDevice = NULL;
	IoDeleteDevice(pDevice);
	devExt->FilterDevice = NULL;

	if (devExt->pIrp != NULL)
	{
		if (CancelIrp(devExt->pIrp))
			DbgPrintEx(77, 0, "[db]: cancel irp\n");
		else
			DbgPrintEx(77, 0, "[db]: cancel irp failed\n");
	}
}

static NTSTATUS AttachDevice(PDRIVER_OBJECT pDriver)
{
	UNICODE_STRING kbdName = RTL_CONSTANT_STRING(L"\\Driver\\kbdclass");
	NTSTATUS status;
	PDEV_EXTENSION devExt;
	PDRIVER_OBJECT kbdDriver;

	PDEVICE_OBJECT filterDevice = NULL;
	PDEVICE_OBJECT targetDevice = NULL;
	PDEVICE_OBJECT lowDevice = NULL;
	// DbgBreakPoint();

	// 获取键盘驱动对象，保存在kbdDriver
	status = ObReferenceObjectByName(&kbdName, OBJ_CASE_INSENSITIVE, NULL, 0,
		*IoDriverObjectType, KernelMode, NULL, &kbdDriver);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("Open KeyBoard Driver Failed!\r\n");
		return status;
	} // 解引用
	else ObDereferenceObject(kbdDriver);

	// 获取键盘驱动的第一个设备
	targetDevice = kbdDriver->DeviceObject;
	while (targetDevice)
	{
		// 创建一个过滤设备
		status = IoCreateDevice(pDriver, sizeof(DEV_EXTENSION), NULL, targetDevice->DeviceType,
			targetDevice->Characteristics, FALSE, &filterDevice);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("Create Filter Driver Failed!\r\n");
			filterDevice = targetDevice = NULL;
			return status;
		}

		// 绑定，lowDevice 是绑定之后得到的下一个设备
		lowDevice = IoAttachDeviceToDeviceStack(filterDevice, targetDevice);
		if (!lowDevice)
		{
			DbgPrint("Attach Failed!\r\n");
			IoDeleteDevice(filterDevice);
			filterDevice = NULL;
			return status;
		}

		// 初始化设备
		devExt = (PDEV_EXTENSION)filterDevice->DeviceExtension;
		DevExtInit(devExt, filterDevice, targetDevice, lowDevice);

		filterDevice->DeviceType = lowDevice->DeviceType;
		filterDevice->Characteristics = lowDevice->Characteristics;
		filterDevice->StackSize = lowDevice->StackSize + 1;
		filterDevice->Flags = lowDevice->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE);

		// 下一个设备
		targetDevice = targetDevice->NextDevice;
	}

	return STATUS_SUCCESS;
}

static NTSTATUS ReadComp(PDEVICE_OBJECT DeviceObject, PIRP pIrp, PVOID Context)
{
	// NTSTATUS status;
	UNREFERENCED_PARAMETER(Context);

	ULONG kerNumber;
	PKEYBOARD_INPUT_DATA myData;

	// KAPC_STATE kApcState = { 0 };
	// PEPROCESS Process = NULL;
	// DbgBreakPoint();
	if (DeviceObject == NULL)
		return STATUS_KEY_DELETED;

	PIO_STACK_LOCATION stack;
	stack = IoGetCurrentIrpStackLocation(pIrp);

	if (NT_SUCCESS(pIrp->IoStatus.Status))
	{
		// get keyboard data
		do
		{
			myData = pIrp->AssociatedIrp.SystemBuffer;
			kerNumber = (ULONG)(pIrp->IoStatus.Information) / sizeof(PKEYBOARD_INPUT_DATA);
			for (ULONG i = 0; i < kerNumber; i++) {
				DbgPrintEx(77, 0, "(i: %d)numkey: %u, sancode: %x, %s\n", i, kerNumber, myData->MakeCode, myData->Flags ? "UP" : "DOWN");
				myData++;
			}
		} while (0);
	}
	if (pIrp->PendingReturned)
	{
		IoMarkIrpPending(pIrp);
	}

	PendingCount--;
	return pIrp->IoStatus.Status;
}



static NTSTATUS FilterDeviceRead(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEV_EXTENSION devExt;
	PDEVICE_OBJECT lowDevice;
	PIO_STACK_LOCATION stack;

	if (pIrp->CurrentLocation == 1)
	{
		DbgPrint("irp send error...\r\n");
		status = STATUS_INVALID_DEVICE_REQUEST;
		pIrp->IoStatus.Status = status;
		pIrp->IoStatus.Information = 0;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return status;
	}
	// 得到设备拓展，目的是之后获得下一个设备的指针
	devExt = pDevice->DeviceExtension;
	lowDevice = devExt->LowDevice;
	//stack = IoGetCurrentIrpStackLocation(pIrp);

	// 复制irp栈
	IoCopyCurrentIrpStackLocationToNext(pIrp);
	// 设置irp完成回调
	IoSetCompletionRoutine(pIrp, ReadComp, pDevice, TRUE, TRUE, TRUE);
	PendingCount++;
	// 调用下一层的函数
	return IoCallDriver(lowDevice, pIrp);
}

static NTSTATUS PnPDispatch(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	PDEV_EXTENSION devExt;
	PIO_STACK_LOCATION stack;
	NTSTATUS status = STATUS_SUCCESS;

	devExt = (PDEV_EXTENSION)pDevice->DeviceExtension;
	stack = IoGetCurrentIrpStackLocation(pIrp);

	switch (stack->MinorFunction)
	{
	case IRP_MN_REMOVE_DEVICE:
		//首先把请求发下去
		IoSkipCurrentIrpStackLocation(pIrp);
		IoCallDriver(devExt->LowDevice, pIrp);
		//解除绑定
		IoDetachDevice(devExt->LowDevice);
		//删除自己的虚拟设备
		IoDeleteDevice(pDevice);
		status = STATUS_SUCCESS;
		break;
	default:
		// 对于其他类型的irp直接下发
		IoSkipCurrentIrpStackLocation(pIrp);
		status = IoCallDriver(devExt->LowDevice, pIrp);
		break;
	}
	return status;
}

static NTSTATUS PowerDispatch(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	PDEV_EXTENSION devExt;
	devExt = (PDEV_EXTENSION)pDevice->DeviceExtension;

	PoStartNextPowerIrp(pIrp);
	IoSkipCurrentIrpStackLocation(pIrp);
	return PoCallDriver(devExt->TargetDevice, pIrp);
}

static NTSTATUS FilterDriverMajorFuncDefault(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDevice);
	NTSTATUS status;
	// 拿到底层设备
	PDEV_EXTENSION devExt = (PDEV_EXTENSION)pDevice->DeviceExtension;
	PDEVICE_OBJECT lowDevice = devExt->LowDevice;

	// 跳过当前设备
	IoSkipCurrentIrpStackLocation(pIrp);
	// 调用底层设备的相应处理函数
	status = IoCallDriver(lowDevice, pIrp);

	return status;
}
