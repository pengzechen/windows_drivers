
#include "Timer.h"

KTIMER gTimer;
KDPC  gTimerDpc;
static ULONG tick;

KDPC dpc;

VOID MyDpcRoutine(PKDPC dpc, PVOID deferredCtx, PVOID SysArgu1, PVOID SysArgu2)
{
	UNREFERENCED_PARAMETER(dpc);
	UNREFERENCED_PARAMETER(deferredCtx);
	UNREFERENCED_PARAMETER(SysArgu1);
	UNREFERENCED_PARAMETER(SysArgu2);

	DbgPrintEx(77, 0, "[db]: My Dpc Routine call back\r\n");
}

VOID TimerRoutine(PKDPC _dpc, PVOID deferredCtx, PVOID SysArgu1, PVOID SysArgu2)
{
	UNREFERENCED_PARAMETER(_dpc);
	UNREFERENCED_PARAMETER(deferredCtx);
	UNREFERENCED_PARAMETER(SysArgu1);
	UNREFERENCED_PARAMETER(SysArgu2);
	
	KeInsertQueueDpc(&dpc, NULL, NULL);

	DbgPrintEx(77, 0, "[db]: Timer callback tick: %d ... \r\n", tick++);
}

VOID TimerDelete()
{
	KeCancelTimer(&gTimer);
}

VOID TimerInit()
{
	KeInitializeTimer(&gTimer);

	KeInitializeDpc(&gTimerDpc, TimerRoutine, NULL);
	KeInitializeDpc(&dpc, MyDpcRoutine, NULL);

	LARGE_INTEGER dueTime;
	dueTime.QuadPart = -1 * 1 * 10 * 1000 * 1000;  // 1second
	KeSetTimerEx(&gTimer, dueTime, 1000, &gTimerDpc);
}
