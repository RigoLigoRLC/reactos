/*
 * PROJECT:     ReactOS IDE Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WMI support
 * COPYRIGHT:   Copyright 2022 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

static WMIGUIDREGINFO IdePortWmiGuidList[] =
{
    {&MSIde_PortDeviceInfo_GUID, 1, 0}
};

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
IdePortQueryWmiRegInfo(
    PDEVICE_OBJECT DeviceObject,
    PULONG RegFlags,
    PUNICODE_STRING InstanceName,
    PUNICODE_STRING *RegistryPath,
    PUNICODE_STRING MofResourceName,
    PDEVICE_OBJECT *Pdo)
{
    PIDEPORT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    UNREFERENCED_PARAMETER(InstanceName);
    UNREFERENCED_PARAMETER(MofResourceName);

    PAGED_CODE();

    INFO("\n");

    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *RegistryPath = &DriverRegistryPath;
    *Pdo = DeviceExtension->Common.Self;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
IdePortQueryWmiDataBlock(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    ULONG GuidIndex,
    ULONG InstanceIndex,
    ULONG InstanceCount,
    PULONG InstanceLengthArray,
    ULONG BufferAvail,
    PUCHAR Buffer)
{
    NTSTATUS Status;
    PMSIde_PortDeviceInfo DeviceInfo;
    PIDEPORT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    INFO("\n");

    if (GuidIndex > RTL_NUMBER_OF(IdePortWmiGuidList))
    {
        Status = STATUS_WMI_GUID_NOT_FOUND;
        goto Complete;
    }

    /* Only ever register 1 instance per GUID */
    if (InstanceIndex != 0 || InstanceCount != 1)
    {
        Status = STATUS_WMI_INSTANCE_NOT_FOUND;
        goto Complete;
    }

    if (!InstanceLengthArray || BufferAvail < sizeof(MSIde_PortDeviceInfo))
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto Complete;
    }

    DeviceInfo = (PMSIde_PortDeviceInfo)Buffer;
    DeviceInfo->Bus = DeviceExtension->PathId;
    DeviceInfo->Target = DeviceExtension->TargetId;
    DeviceInfo->Lun = DeviceExtension->Lun;
    *InstanceLengthArray = sizeof(MSIde_PortDeviceInfo);

    Status = STATUS_SUCCESS;

Complete:
    return WmiCompleteRequest(DeviceObject,
                              Irp,
                              Status,
                              sizeof(MSIde_PortDeviceInfo),
                              IO_NO_INCREMENT);
}

CODE_SEG("PAGE")
NTSTATUS
IdePortWmiRegistration(
    _In_ PIDEPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ BOOLEAN Register)
{
    PAGED_CODE();

    if (!Register)
    {
        return IoWMIRegistrationControl(DeviceExtension->Common.Self,
                                        WMIREG_ACTION_DEREGISTER);
    }

    DeviceExtension->WmiLibInfo.GuidCount = RTL_NUMBER_OF(IdePortWmiGuidList);
    DeviceExtension->WmiLibInfo.GuidList = IdePortWmiGuidList;

    DeviceExtension->WmiLibInfo.QueryWmiRegInfo = IdePortQueryWmiRegInfo;
    DeviceExtension->WmiLibInfo.QueryWmiDataBlock = IdePortQueryWmiDataBlock;
    DeviceExtension->WmiLibInfo.SetWmiDataBlock = NULL;
    DeviceExtension->WmiLibInfo.SetWmiDataItem = NULL;
    DeviceExtension->WmiLibInfo.ExecuteWmiMethod = NULL;
    DeviceExtension->WmiLibInfo.WmiFunctionControl = NULL;

    return IoWMIRegistrationControl(DeviceExtension->Common.Self,
                                    WMIREG_ACTION_REGISTER);
}

static
NTSTATUS
IdePdoDispatchWmi(
    _In_ PIDEPORT_DEVICE_EXTENSION DeviceExtension,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    SYSCTL_IRP_DISPOSITION Disposition;

    PAGED_CODE();

    DPRINT("%s(%p, %p) %lx\n", __FUNCTION__, DeviceExtension, Irp,
           IoGetCurrentIrpStackLocation(Irp)->MinorFunction);

    Status = IdePortDeviceAcquireRemoveLock(DeviceExtension, Irp);
    if (!NT_SUCCESS(Status))
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return Status;
    }

    Status = WmiSystemControl(&DeviceExtension->WmiLibInfo,
                              DeviceExtension->Common.Self,
                              Irp,
                              &Disposition);
    switch (Disposition)
    {
        case IrpProcessed:
            break;

        case IrpNotCompleted:
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        case IrpForward:
        case IrpNotWmi:
            Status = STATUS_NOT_SUPPORTED;
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        default:
            ASSERT(FALSE);
            UNREACHABLE;
            break;
    }

    IdePortDeviceReleaseRemoveLock(DeviceExtension);

    return Status;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
IdePortDispatchWmi(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PAGED_CODE();

    if (IS_FDO(DeviceObject->DeviceExtension))
    {
        PIDEPORT_CHANNEL_EXTENSION ChannelExtension = DeviceObject->DeviceExtension;

        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(ChannelExtension->Ldo, Irp);
    }
    else
    {
        return IdePdoDispatchWmi(DeviceObject->DeviceExtension, Irp);
    }
}
