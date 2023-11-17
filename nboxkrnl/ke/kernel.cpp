/*
 * ergo720                Copyright (c) 2022
 * PatrickvL              Copyright (c) 2016-2017
 */

#pragma once

#include "..\ki\ki.hpp"
#include "..\kernel.hpp"

#define XBOX_ACPI_FREQUENCY 3375000 // 3.375 MHz


XBOX_KEY_DATA XboxCERTKey;

EXPORTNUM(120) volatile KSYSTEM_TIME KeInterruptTime = { 0, 0, 0 };

EXPORTNUM(154) volatile KSYSTEM_TIME KeSystemTime = { 0, 0, 0 };

EXPORTNUM(156) volatile DWORD KeTickCount = 0;

EXPORTNUM(157) ULONG KeTimeIncrement = CLOCK_TIME_INCREMENT;

EXPORTNUM(321) XBOX_KEY_DATA XboxEEPROMKey;

EXPORTNUM(322) XBOX_HARDWARE_INFO XboxHardwareInfo =
{
	0,     // Flags: 1=INTERNAL_USB, 2=DEVKIT, 4=MACROVISION, 8=CHIHIRO
	0xA2,  // GpuRevision, byte read from NV2A first register, at 0xFD0000000 - see NV_PMC_BOOT_0
	0xD3,  // McpRevision, Retail 1.6
	0,     // unknown
	0      // unknown
};

// Source: Cxbx-Reloaded
EXPORTNUM(125) ULONGLONG XBOXAPI KeQueryInterruptTime()
{
	LARGE_INTEGER InterruptTime;

	while (true) {
		InterruptTime.u.HighPart = KeInterruptTime.HighTime;
		InterruptTime.u.LowPart = KeInterruptTime.LowTime;

		if (InterruptTime.u.HighPart == KeInterruptTime.High2Time) {
			break;
		}
	}

	return (ULONGLONG)InterruptTime.QuadPart;
}

EXPORTNUM(126) ULONGLONG XBOXAPI KeQueryPerformanceCounter()
{
	__asm {
		pushfd
		cli
		mov edx, KE_ACPI_TIME_LOW
		in eax, dx
		mov ecx, eax
		inc edx
		in eax, dx
		mov edx, eax
		mov eax, ecx
		popfd
	}
}

EXPORTNUM(127) ULONGLONG XBOXAPI KeQueryPerformanceFrequency()
{
	return XBOX_ACPI_FREQUENCY;
}

// Source: Cxbx-Reloaded
EXPORTNUM(128) VOID XBOXAPI KeQuerySystemTime
(
	PLARGE_INTEGER CurrentTime
)
{
	LARGE_INTEGER SystemTime;

	while (true) {
		SystemTime.u.HighPart = KeSystemTime.HighTime;
		SystemTime.u.LowPart = KeSystemTime.LowTime;

		if (SystemTime.u.HighPart == KeSystemTime.High2Time) {
			break;
		}
	}

	*CurrentTime = SystemTime;
}

VOID FASTCALL OutputToHost(ULONG Value, USHORT Port)
{
	__asm {
		mov eax, ecx
		out dx, eax
	}
}

ULONG FASTCALL InputFromHost(USHORT Port)
{
	__asm {
		mov edx, ecx
		in eax, dx
	}
}

VOID FASTCALL SubmitIoRequestToHost(IoRequest *Request)
{
	__asm {
		mov dx, IO_START
		mov eax, ecx
		out dx, eax
		mov dx, IO_CHECK_ENQUEUE
		in eax, dx
		test eax, eax
		jz ok
	retry:
		mov dx, IO_RETRY
		out dx, eax
		mov dx, IO_CHECK_ENQUEUE
		in eax, dx
		test eax, eax
		jnz retry
	ok:
	}
}

VOID FASTCALL RetrieveIoRequestFromHost(IoInfoBlock *Info, LONG Id)
{
	// NOTE: ebx is already saved by the compiler
	// TODO: instead of polling the IO like this, the host should signal I/O completion by raising a HDD interrupt, so that we can handle the event in the ISR

	__asm {
		pushfd
		cli
		mov ebx, edx
		mov eax, edx
		mov dx, IO_SET_ID
		out dx, eax
		mov dx, IO_QUERY_STATUS
		in eax, dx
		mov [ecx]IoInfoBlock.Status, eax
		mov dx, IO_QUERY_INFO
		in eax, dx
		mov [ecx]IoInfoBlock.Info, eax
		cmp dword ptr [ecx]IoInfoBlock.Status, Pending
		jnz ok
	retry:
		cli
		mov eax, ebx
		mov dx, IO_SET_ID
		out dx, eax
		mov dx, IO_QUERY_STATUS
		in eax, dx
		mov [ecx]IoInfoBlock.Status, eax
		mov dx, IO_QUERY_INFO
		in eax, dx
		mov [ecx]IoInfoBlock.Info, eax
		sti
		cmp dword ptr [ecx]IoInfoBlock.Status, Pending
		jz retry
	ok:
		dec IoRequestId
		popfd
	}
}

// Source: Cxbx-Reloaded
VOID InitializeListHead(PLIST_ENTRY pListHead)
{
	pListHead->Flink = pListHead->Blink = pListHead;
}

// Source: Cxbx-Reloaded
BOOLEAN IsListEmpty(PLIST_ENTRY pListHead)
{
	return (pListHead->Flink == pListHead);
}

// Source: Cxbx-Reloaded
VOID InsertTailList(PLIST_ENTRY pListHead, PLIST_ENTRY pEntry)
{
	PLIST_ENTRY _EX_ListHead = pListHead;
	PLIST_ENTRY _EX_Blink = _EX_ListHead->Blink;

	pEntry->Flink = _EX_ListHead;
	pEntry->Blink = _EX_Blink;
	_EX_Blink->Flink = pEntry;
	_EX_ListHead->Blink = pEntry;
}

// Source: Cxbx-Reloaded
VOID InsertHeadList(PLIST_ENTRY pListHead, PLIST_ENTRY pEntry)
{
	PLIST_ENTRY _EX_ListHead = pListHead;
	PLIST_ENTRY _EX_Flink = _EX_ListHead->Flink;

	pEntry->Flink = _EX_Flink;
	pEntry->Blink = _EX_ListHead;
	_EX_Flink->Blink = pEntry;
	_EX_ListHead->Flink = pEntry;
}

// Source: Cxbx-Reloaded
VOID RemoveEntryList(PLIST_ENTRY pEntry)
{
	PLIST_ENTRY _EX_Flink = pEntry->Flink;
	PLIST_ENTRY _EX_Blink = pEntry->Blink;
	_EX_Blink->Flink = _EX_Flink;
	_EX_Flink->Blink = _EX_Blink;
}

// Source: Cxbx-Reloaded
PLIST_ENTRY RemoveTailList(PLIST_ENTRY pListHead)
{
	PLIST_ENTRY pList = pListHead->Blink;
	RemoveEntryList(pList);
	return pList;
}

// Source: Cxbx-Reloaded
PLIST_ENTRY RemoveHeadList(PLIST_ENTRY pListHead)
{
	PLIST_ENTRY pList = pListHead->Flink;
	RemoveEntryList(pList);
	return pList;
}
