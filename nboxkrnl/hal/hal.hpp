/*
 * ergo720                Copyright (c) 2023
 */

#pragma once

#include "..\types.hpp"
#include "..\ke\ke.hpp"


#ifdef __cplusplus
extern "C" {
#endif

EXPORTNUM(43) DLLEXPORT VOID XBOXAPI HalEnableSystemInterrupt
(
	ULONG BusInterruptLevel,
	KINTERRUPT_MODE InterruptMode
);

#ifdef __cplusplus
}
#endif

VOID HalInitSystem();
