/*
 * ergo720                Copyright (c) 2022
 */

#pragma once

#define STATUS_SUCCESS                          ((NTSTATUS)0x00000000L)
#define STATUS_TIMEOUT                          ((NTSTATUS)0x00000102L)
#define STATUS_GUARD_PAGE_VIOLATION             ((NTSTATUS)0x80000001L)
#define STATUS_BREAKPOINT                       ((NTSTATUS)0x80000003L)
#define STATUS_SINGLE_STEP                      ((NTSTATUS)0x80000004L)
#define STATUS_INVALID_HANDLE                   ((NTSTATUS)0xC0000008L)
#define STATUS_INVALID_PARAMETER                ((NTSTATUS)0xC000000DL)
#define STATUS_NO_MEMORY                        ((NTSTATUS)0xC0000017L)
#define STATUS_CONFLICTING_ADDRESSES            ((NTSTATUS)0xC0000018L)
#define STATUS_BUFFER_TOO_SMALL                 ((NTSTATUS)0xC0000023L)
#define STATUS_OBJECT_TYPE_MISMATCH             ((NTSTATUS)0xC0000024L)
#define STATUS_NONCONTINUABLE_EXCEPTION         ((NTSTATUS)0xC0000025L)
#define STATUS_INVALID_DISPOSITION              ((NTSTATUS)0xC0000026L)
#define STATUS_UNWIND                           ((NTSTATUS)0xC0000027L)
#define STATUS_BAD_STACK                        ((NTSTATUS)0xC0000028L)
#define STATUS_INVALID_UNWIND_TARGET            ((NTSTATUS)0xC0000029L)
#define STATUS_PARITY_ERROR                     ((NTSTATUS)0xC000002BL)
#define STATUS_OBJECT_NAME_NOT_FOUND            ((NTSTATUS)0xC0000034L)
#define STATUS_INVALID_PAGE_PROTECTION          ((NTSTATUS)0xC0000045L)
#define STATUS_INVALID_IMAGE_FORMAT             ((NTSTATUS)0xC000007BL)
#define STATUS_ARRAY_BOUNDS_EXCEEDED            ((NTSTATUS)0xC000008CL)
#define STATUS_FLOAT_DENORMAL_OPERAND           ((NTSTATUS)0xC000008DL)
#define STATUS_FLOAT_DIVIDE_BY_ZERO             ((NTSTATUS)0xC000008EL)
#define STATUS_FLOAT_INEXACT_RESULT             ((NTSTATUS)0xC000008FL)
#define STATUS_FLOAT_INVALID_OPERATION          ((NTSTATUS)0xC0000090L)
#define STATUS_FLOAT_OVERFLOW                   ((NTSTATUS)0xC0000091L)
#define STATUS_FLOAT_STACK_CHECK                ((NTSTATUS)0xC0000092L)
#define STATUS_FLOAT_UNDERFLOW                  ((NTSTATUS)0xC0000093L)
#define STATUS_INTEGER_DIVIDE_BY_ZERO           ((NTSTATUS)0xC0000094L)
#define STATUS_PRIVILEGED_INSTRUCTION           ((NTSTATUS)0xC0000096L)
#define STATUS_INSUFFICIENT_RESOURCES           ((NTSTATUS)0xC000009AL)
#define STATUS_IO_DEVICE_ERROR                  ((NTSTATUS)0xC0000185L)

#define NT_SUCCESS(Status)                      ((NTSTATUS)(Status) >= 0)
