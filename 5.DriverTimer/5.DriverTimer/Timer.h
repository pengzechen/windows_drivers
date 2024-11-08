#pragma once


#include <ntddk.h>

extern KTIMER gTimer;
extern KDPC  gDpc;

extern VOID TimerRoutine(PKDPC dpc, PVOID deferredCtx, PVOID SysArgu1, PVOID SysArgu2);

extern VOID TimerDelete();

extern VOID TimerInit();
