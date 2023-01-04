/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iomgr/crashdump.c
 * PURPOSE:         Support of memory dump upon system crash
 * PROGRAMMERS:     RigoLigo (rigoligo03@gmail.com)
 */

#include <ntoskrnl.h>

#define DUMP_POOL_TAG 'pmuD'

/*
 * GLOBAL VARIABLES
 */
PROS_DUMP_CONTROL_BLOCK IopDumpControlBlock;

NTSTATUS
NTAPI
IopReadDumpRegistry(OUT PULONG DumpFlags,
                    OUT PULONG ReservePages, // TODO. Not yet reverse engineered
                    OUT PBOOLEAN AutoReboot,
                    OUT PULONG DumpFileSize)
{
    UNICODE_STRING RegistryPath;
    HANDLE ControlKey, CrashControlKey;
    PKEY_VALUE_FULL_INFORMATION ValueInfo;
    NTSTATUS Result;
    PAGED_CODE()

    /* Give the expected variables some sane defaults in case of read failure */
    *DumpFlags = 0;
    *ReservePages = 0;
    *AutoReboot = TRUE;
    *DumpFileSize = 0;

    /* Open Crash Dump registry */
    RtlInitUnicodeString(&RegistryPath, L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control");
    Result = IopOpenRegistryKeyEx(&ControlKey, NULL, &RegistryPath, KEY_READ);
    RtlFreeUnicodeString(&RegistryPath);
    if(!NT_SUCCESS(Result))
    {
        return Result;
    }
    
    RtlInitUnicodeString(&RegistryPath, L"CrashControl");
    Result = IopOpenRegistryKeyEx(&CrashControlKey, ControlKey, &RegistryPath, KEY_READ);
    RtlFreeUnicodeString(&RegistryPath);
    if(!NT_SUCCESS(Result))
    {
        ObCloseHandle(ControlKey, KernelMode);
        return Result;
    }

    Result = IopGetRegistryValue(CrashControlKey, L"CrashDumpEnabled", &ValueInfo);
    if(!NT_SUCCESS(Result))
        goto Cleanup;

    if(ValueInfo->DataLength)
    {
        ULONG dwCrashDumpType = *(PULONG)((PCHAR)ValueInfo + ValueInfo->DataOffset);
        *DumpFlags = IO_DUMP_ENABLED;

        switch (dwCrashDumpType)
        {
            case 1:
                /* Full memory dump */
                *DumpFlags |= IO_DUMP_TYPE_FULL;
                break;

            case 2:
                /* Kernel memory dump */
                *DumpFlags |= IO_DUMP_TYPE_KERNEL;
                /* TODO: Should calculate ReservePages here but I didnt understand original code*/
                break;
            
            case 3:
                /* Triage dump, small memory dump or minidump */
                *DumpFlags |= IO_DUMP_TYPE_TRIAGE;
                break;
            
            default:
            case 0:
                /* Clear enable flag only when it's not enabled */
                *DumpFlags |= ~IO_DUMP_ENABLED;
                break;
        }
    }
    ExFreePool(ValueInfo);

    Result = IopGetRegistryValue(CrashControlKey, L"SendAlert", &ValueInfo);
    if(NT_SUCCESS(Result))
    {
        if(ValueInfo->DataLength)
        {
            LONG doSendAlert = *(PULONG)((PCHAR)ValueInfo + ValueInfo->DataOffset);
            if(doSendAlert)
            {
                *DumpFlags |= IO_DUMP_SEND_ALERT;
            }
        }
        ExFreePool(ValueInfo);
    }

    Result = IopGetRegistryValue(CrashControlKey, L"AutoReboot", &ValueInfo);
    if(NT_SUCCESS(Result))
    {
        if(ValueInfo->DataLength)
        {
            *AutoReboot = *(PULONG)((PCHAR)ValueInfo + ValueInfo->DataOffset);
        }
        ExFreePool(ValueInfo);
    }

    Result = IopGetRegistryValue(CrashControlKey, L"DumpFileSize", &ValueInfo);
    if(NT_SUCCESS(Result))
    {
        if(ValueInfo->DataLength)
        {
            *DumpFileSize = *(PULONG)((PCHAR)ValueInfo + ValueInfo->DataOffset);
        }
        ExFreePool(ValueInfo);
    }
    
Cleanup:
    ObCloseHandle(ControlKey, KernelMode);
    ObCloseHandle(CrashControlKey, KernelMode);
    RtlFreeUnicodeString(&RegistryPath);
    return Result;
}

BOOLEAN
NTAPI
IopInitializeDCB()
{
    PROS_DUMP_CONTROL_BLOCK pDcb = NULL;
    ULONG DumpFlags, ReservePages, DumpFileSize;
    BOOLEAN AutoReboot;

    IopReadDumpRegistry(&DumpFlags, &ReservePages, &AutoReboot, &DumpFileSize);
    if(DumpFlags == 0 && AutoReboot == FALSE)
    {
        return TRUE;
    }

    if(DumpFlags | IO_DUMP_TYPE_TRIAGE)
    {
        /* Triage dump is always 64KB in size */
        DumpFileSize = 65536;
    }

    pDcb = ExAllocatePoolWithTag(NonPagedPool, sizeof(ROS_DUMP_CONTROL_BLOCK), DUMP_POOL_TAG);
    if(pDcb == NULL)
    {
        goto Failure;
    }

    RtlZeroMemory(pDcb, sizeof(ROS_DUMP_CONTROL_BLOCK));



Failure:
    IopDumpControlBlock = pDcb;
    return FALSE;
}
