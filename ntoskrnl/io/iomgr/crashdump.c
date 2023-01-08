/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iomgr/crashdump.c
 * PURPOSE:         Support of memory dump upon system crash
 * PROGRAMMERS:     RigoLigo (rigoligo03@gmail.com)
 */

#include <ntoskrnl.h>
#include <ntddscsi.h>

#define DUMP_POOL_TAG 'pmuD'

/* GLOBALS *******************************************************************/
PROS_DUMP_CONTROL_BLOCK IopDumpControlBlock;
extern UNICODE_STRING IoArcBootDeviceName; // in ./arcname.c

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
    if(!NT_SUCCESS(Result))
    {
        return Result;
    }
    
    RtlInitUnicodeString(&RegistryPath, L"CrashControl");
    Result = IopOpenRegistryKeyEx(&CrashControlKey, ControlKey, &RegistryPath, KEY_READ);
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
    return Result;
}

VOID
NTAPI
IopFreeDCB(BOOLEAN UnknownParameter)
{
    UNIMPLEMENTED;
}

BOOLEAN
NTAPI
IopInitializeDCB(VOID)
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

    /* Allocate dump control block and clear it with zeros*/
    pDcb = ExAllocatePoolWithTag(NonPagedPool, sizeof(ROS_DUMP_CONTROL_BLOCK), DUMP_POOL_TAG);
    if(pDcb == NULL)
    {
        goto Failure;
    }
    RtlZeroMemory(pDcb, sizeof(ROS_DUMP_CONTROL_BLOCK));

    pDcb->DumpFlags = DumpFlags;

    /* TODO: FILL THEM IN */
    IopDumpControlBlock = pDcb;
    return TRUE;

Failure:
    IopDumpControlBlock = pDcb;
    return FALSE;
}

NTSTATUS
NTAPI
IopLoadDumpDriver(IN OUT PDUMP_STACK_CONTEXT DumpStack,
                  IN     PCWSTR DriverPath,
                  IN     PCWSTR DriverName)
{
    UNICODE_STRING Prefix, ModulePath, LoadedName;
    PUNICODE_STRING pLoadedName;
    PVOID ImageBase, ModuleObject;
    NTSTATUS Status;

    pLoadedName = NULL;
    RtlInitUnicodeString(&Prefix, DumpStack->ModulePrefix);
    RtlInitUnicodeString(&ModulePath, DriverPath);

    /* If the driver name is specified, then use it with the prefix as loaded name */
    if(DriverName != NULL)
    {
        /* Get the length of driver name string */
        RtlInitUnicodeString(&LoadedName, DriverName);

        /* Prepend the specified prefix ("dump_", etc) to the module name */
        LoadedName.MaximumLength += Prefix.Length;
        LoadedName.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                  LoadedName.MaximumLength,
                                                  DUMP_POOL_TAG);
        LoadedName.Length = 0;
        if(LoadedName.Buffer == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlAppendUnicodeStringToString(&LoadedName, &Prefix);
        RtlAppendUnicodeToString(&LoadedName, DriverName);

        pLoadedName = &LoadedName;
    }
    else
    {
        /* Set this thing to NULL so it's not freed later */
        LoadedName.Buffer = NULL;
    }

    /* Load the driver */
    Status = MmLoadSystemImage(&ModulePath,
                                &Prefix,
                                pLoadedName,
                                0, /* FIXME: Session loading is not supported*/
                                &ModuleObject,
                                &ImageBase);
    
    /* Discard the temporary string */
    if(LoadedName.Buffer != NULL)
    {
        ExFreePool(LoadedName.Buffer);
    }

    if(!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* TODO: Put it onto the DumpStack->DriverList. 
             Have not nailed down the node structure yet, it's not viable right now */
    return Status;
}

NTSTATUS
NTAPI
IopGetDumpStack(IN PCWSTR DriverPrefix,
                OUT PDUMP_STACK_CONTEXT* DumpStack,
                IN PUNICODE_STRING BootDeviceArcName,
                IN PCWSTR DriverPath, /* FIXME: Only used in SCSI path but not ref'd */
                IN DWORD UsageType,
                IN BOOLEAN DryRun)
{
    PCWSTR MiniportDriverName;
    PDUMP_STACK_CONTEXT pDumpStack;
    PDUMP_POINTERS DumpPointers;
    OBJECT_ATTRIBUTES FileObjectAttr;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatus;
    BOOLEAN IsBootDriveScsi;
    PSCSI_ADDRESS ScsiAddress;
    PARTITION_INFORMATION_EX PartitionInformation;
    PDISK_PARTITION_INFO DiskPartitionInfo;
    PIRP Irp;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject, RelatedDeviceObject;
    IO_STACK_LOCATION StackLocation;
    KEVENT Event;
    PVOID DiskIoBuffer;
    NTSTATUS Status;
    
    /* Allocate dump stack context */
    pDumpStack = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(DUMP_STACK_CONTEXT),
                                       DUMP_POOL_TAG);
    if(pDumpStack == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* The dump pointers' memory is included in dump stack */
    pDumpStack->DumpPointers = &pDumpStack->DumpPointerMemory;

    /* Initialize driver list */
    InitializeListHead(&pDumpStack->DriverList);

    /* Allocate disk IO buffer used to determine boot drive geometry */
    DiskIoBuffer = ExAllocatePoolWithTag(PagedPool, 0x1000, DUMP_POOL_TAG);
    if(DiskIoBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Failure1;
    }

    /* Open boot drive */
    InitializeObjectAttributes(&FileObjectAttr,
                               BootDeviceArcName,
                               FILE_ATTRIBUTE_SPARSE_FILE,
                               NULL,
                               NULL);
    Status = ZwOpenFile(&Handle,
                        SYNCHRONIZE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                        &FileObjectAttr,
                        &IoStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_NON_DIRECTORY_FILE);
    if(!NT_SUCCESS(Status))
    {
        IOTRACE(IO_DUMP_DEBUG,
                "%s - Could not open boot drive, Status = %x",
                __FUNCTION__,
                Status);
        goto Failure2;
    }

    /* Try to fetch the SCSI address of the boot drive */
    Status = ZwDeviceIoControlFile(Handle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatus,
                                   IOCTL_SCSI_GET_ADDRESS,
                                   NULL,
                                   0,
                                   DiskIoBuffer,
                                   sizeof(SCSI_ADDRESS));

    /* Wait until the IO is finished*/
    if(Status == STATUS_PENDING)
    {
        ZwWaitForSingleObject(Handle, FALSE, NULL);
        Status = IoStatus.Status;
    }

    /* Determine if the boot drive is on SCSI bus, in order to select proper dump stack drivers*/
    IsBootDriveScsi = NT_SUCCESS(Status);
    if(!IsBootDriveScsi)
    {
        IOTRACE(IO_DUMP_DEBUG,
                "%s - IOCTL_SCSI_GET_ADDRESS failed, Status = %x, not SCSI boot device",
                __FUNCTION__,
                Status);
        pDumpStack->Init.TargetAddress = NULL;
    }
    else
    {
        IOTRACE(IO_DUMP_DEBUG,
                "%s - IOCTL_SCSI_GET_ADDRESS success, booting from SCSI device",
                __FUNCTION__);

        /* Allocate SCSI address structure for the Initialization context */
        ScsiAddress = ExAllocatePoolWithTag(NonPagedPool, sizeof(SCSI_ADDRESS), DUMP_POOL_TAG);
        if(ScsiAddress == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Failure3;
        }

        /* Copy the SCSI address over */
        pDumpStack->Init.TargetAddress = ScsiAddress;
        RtlMoveMemory(ScsiAddress, DiskIoBuffer, sizeof(SCSI_ADDRESS));
    }

    /* Try to fetch the boot drive partition style info. This will be used later */
    Status = ZwDeviceIoControlFile(Handle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatus,
                                   IOCTL_DISK_GET_PARTITION_INFO_EX,
                                   NULL,
                                   0,
                                   &PartitionInformation,
                                   sizeof(PARTITION_INFORMATION_EX));

    /* Wait until the IO is finished*/
    if(Status == STATUS_PENDING)
    {
        ZwWaitForSingleObject(Handle, FALSE, NULL);
    }
    Status = IoStatus.Status;

    /* Try to fetch the boot drive partition info*/
    Status = ZwDeviceIoControlFile(Handle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatus,
                                   IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                                   NULL,
                                   0,
                                   DiskIoBuffer,
                                   sizeof(DISK_GEOMETRY_EX));

    /* Wait until the IO is finished*/
    if(Status == STATUS_PENDING)
    {
        ZwWaitForSingleObject(Handle, FALSE, NULL);
    }
    Status = IoStatus.Status;
    DiskPartitionInfo = DiskGeometryGetPartition((PDISK_GEOMETRY_EX)DiskIoBuffer);

    /* Fill in the partition style and disk info structures */
    pDumpStack->Init.PartitionStyle = DiskPartitionInfo->PartitionStyle;
    if(DiskPartitionInfo->PartitionStyle == PARTITION_STYLE_MBR)
    {
        pDumpStack->Init.DiskInfo.Mbr.Checksum = DiskPartitionInfo->Mbr.CheckSum;
        pDumpStack->Init.DiskInfo.Mbr.Signature = DiskPartitionInfo->Mbr.Signature;
    }
    else
    {
        pDumpStack->Init.DiskInfo.Gpt.Guid = DiskPartitionInfo->Gpt.DiskId;
    }

    /* Obtain file object of the boot drive */
    Status = ObReferenceObjectByHandle(Handle,
                                       0,
                                       IoFileObjectType,
                                       KernelMode,
                                       &FileObject,
                                       NULL);

    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    
    /* Construct IRP for obtaining dump pointers */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_GET_DUMP_POINTERS,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        pDumpStack->DumpPointers,
                                        sizeof(DUMP_POINTERS),
                                        FALSE,
                                        &Event,
                                        &IoStatus);
    if(Irp == NULL)
    {
        ObDereferenceObject(FileObject);
        goto Failure3;
    }

    /* TODO: figure out what this does. Next level in stack should be File system. */
    IoGetNextIrpStackLocation(Irp)->FileObject = FileObject;

    /* Ask the driver for dump pointers */
    Status = IoCallDriver(DeviceObject, Irp);
    if(Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    }
    Status = IoStatus.Status;

    /* Sanity check the results. Size of data is written in IoStatus.Information. See SCSI IOCTL */
    if(!NT_SUCCESS(Status) || IoStatus.Information < sizeof(DUMP_POINTERS))
    {
        ObDereferenceObject(FileObject);
        goto Failure3;
    }

    /* Fill in dump stack context information */
    pDumpStack->PointersLength = IoStatus.Information;
    DumpPointers = (PDUMP_POINTERS)pDumpStack->DumpPointers;
    
    /* If the device object is present, find the name of the associated miniport driver */
    MiniportDriverName = NULL;
    if(DumpPointers->DeviceObject != NULL)
    {
        PCWSTR FoundString;
        
        FoundString = ((PDEVICE_OBJECT)DumpPointers->DeviceObject)->DriverObject->DriverName.Buffer;
        
        /* This buffer taken from DriverName is a full path in the object namespace.
           Should remove the leading paths and leave only the name */
        do
        {
            /* Look for the backslash. If backslash is found, skip one character and pass it to
               name pointer. Otherwise, there's no backslash and search is considered done */
            MiniportDriverName = FoundString;
            FoundString = wcsstr(MiniportDriverName, L"\\");
        }
        while(FoundString++ != L'\0');
    }

    pDumpStack->FileObject = FileObject;

    /* We're done with the boot drive, close file handle now */
    ZwClose(Handle);

    /* Fill in rest of initialization context */
    /* NOTICE: SCSI IOCTL is mostly broken? It nearly didn't fill dump pointers at all. */
    pDumpStack->Init.Length = sizeof(DUMP_INITIALIZATION_CONTEXT);
    pDumpStack->Init.StallRoutine = KeStallExecutionProcessor; /* FIXME: type of function pointer*/
    pDumpStack->Init.AdapterObject = DumpPointers->AdapterObject;
    pDumpStack->Init.MappedRegisterBase = DumpPointers->MappedRegisterBase;
    pDumpStack->Init.PortConfiguration = DumpPointers->DumpData; /* FIXME: Verify this one */

    pDumpStack->ModulePrefix = DriverPrefix;
    pDumpStack->PartitionOffset = PartitionInformation.StartingOffset;
    pDumpStack->UsageType = DeviceUsageTypeUndefined;

    /* TODO: Why 8KB? */
    if(DumpPointers->CommonBufferSize < 8192)
    {
        DumpPointers->CommonBufferSize = 8192;
    }

    /* TODO: Does this mean we're supposed to allocate common buffer in kernel ourselves? */
    if(DumpPointers->AllocateCommonBuffers)
    {
        /* TODO */
    }

    /* Now based on the bus on which the boot drive is connected to, load different drivers */
    if(IsBootDriveScsi)
    {
        UNICODE_STRING DeviceName;
        OBJECT_ATTRIBUTES DeviceObjectAttribute;
        PFILE_OBJECT ScsiDeviceFile;
        PWSTR FoundString;

        /* SCSIport boot drive, use DiskDump */

        Status = IopLoadDumpDriver(pDumpStack,
                                   (PCWSTR)DriverPath,
                                   L"scsiport.sys");
        
        if(!NT_SUCCESS(Status))
        {
            goto Failure2;
        }

        /* Open the SCSI device */
        swprintf((PWSTR)DiskIoBuffer, L"\\Device\\ScsiPort%d", ScsiAddress->PortNumber);
        RtlInitUnicodeString(&DeviceName, (PCWSTR)DiskIoBuffer);
        InitializeObjectAttributes(&DeviceObjectAttribute, &DeviceName, 0, NULL, NULL);

        Status = ZwOpenFile(&Handle,
                            FILE_READ_ATTRIBUTES,
                            &DeviceObjectAttribute,
                            &IoStatus,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_NON_DIRECTORY_FILE);
        
        if(!NT_SUCCESS(Status))
        {
            goto Failure2;
        }

        Status = ObReferenceObjectByHandle(Handle,
                                           0,
                                           IoFileObjectType,
                                           KernelMode,
                                           &ScsiDeviceFile,
                                           NULL);

        if(!NT_SUCCESS(Status))
        {
            goto Failure3;
        }
        
        FoundString = ScsiDeviceFile->DeviceObject->DriverObject->DriverName.Buffer;
        
        /* Same driver name locating routine as before*/
        do
        {
            MiniportDriverName = FoundString;
            FoundString = wcsstr(MiniportDriverName, L"\\");
        }
        while(FoundString++ != L'\0');

        ObDereferenceObject(ScsiDeviceFile);
        ZwClose(Handle);

        /* Falls through */
    }

    /* Not SCSI boot drive */
    if(MiniportDriverName == NULL)
    {
        /* No miniport driver? Fail the load */
        ObDereferenceObject(FileObject);
        goto Failure2;
    }

    /* Create the driver path string */
    /* TODO: Find safe routines? */
    swprintf((PWSTR)DiskIoBuffer,
                L"\\SystemRoot\\System32\\Drivers\\%s.sys",
                MiniportDriverName);

    /* Call load dump driver routine */
    Status = IopLoadDumpDriver(pDumpStack, (PWCHAR)DiskIoBuffer, NULL);

    if(!NT_SUCCESS(Status))
    {
        IOTRACE(IO_DUMP_DEBUG,
            "%s - Load dump driver for non SCSI system failed, Status = %x",
            __FUNCTION__,
            Status);
        ObDereferenceObject(FileObject);
        goto Failure2;
    }

    /* Get the device's related device object */
    RelatedDeviceObject = IoGetRelatedDeviceObject(pDumpStack->FileObject);

    /* Directly notify the device of its usage */
    RtlZeroMemory(&StackLocation, sizeof(IO_STACK_LOCATION));
    StackLocation.MajorFunction = IRP_MJ_PNP;
    StackLocation.MinorFunction = IRP_MN_DEVICE_USAGE_NOTIFICATION;
    StackLocation.Parameters.UsageNotification.Type = UsageType; /*FIXME:VERIFY*/
    StackLocation.Parameters.UsageNotification.InPath = TRUE; /*FIXME:EXPLAIN*/

    Status = IopSynchronousCall(RelatedDeviceObject, &StackLocation, NULL);

    if(DryRun)
    {
        Status = STATUS_SUCCESS;
    }
    if(!NT_SUCCESS(Status))
    {
        ObDereferenceObject(FileObject);
        goto Failure2;
    }

    pDumpStack->UsageType = UsageType;
    
    goto Cleanup;

Failure3:
    ZwClose(Handle);
Cleanup:
    if(NT_SUCCESS(Status))
    {
        /* Output the dump stack */
        *DumpStack = pDumpStack;

        ExFreePool(DiskIoBuffer);
        return Status;
    }
Failure2:
    ExFreePool(DiskIoBuffer);
Failure1:
    ExFreePool(pDumpStack);
    return Status;
}

// NTSTATUS
// NTAPI
// IoGetDumpStack()

BOOLEAN
NTAPI
IoInitializeCrashDump(IN HANDLE PageFileHandle)
{
    PFILE_OBJECT PageFileObject;
    NTSTATUS Status;

    IopFreeDCB(TRUE);
    if(!IopInitializeDCB())
    {
        return FALSE;
    }

    if(IopDumpControlBlock == NULL)
    {
        return FALSE;
    }

    if((IopDumpControlBlock->DumpFlags & (IO_DUMP_ENABLED | IO_DUMP_SEND_ALERT)) == 0)
    {
        return FALSE;
    }

    /* Try to open pagefile */
    Status = ObReferenceObjectByHandle(PageFileHandle,
                                       0,
                                       IoFileObjectType,
                                       KernelMode,
                                       &PageFileObject,
                                       NULL);
    if(!NT_SUCCESS(Status))
    {
        // IO_DUMP_PAGE_CONFIG_FAILED
        return FALSE;
    }
    ObDereferenceObject(PageFileObject);

    Status = IopGetDumpStack(L"dump_",
                             &IopDumpControlBlock->DumpStack,
                             &IoArcBootDeviceName,
                             L"\\SystemRoot\\System32\\Drivers\\diskdump.sys",
                             DeviceUsageTypeDumpFile,
                             FALSE);

    return FALSE;
}
