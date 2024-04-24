/*
 * PROJECT:     ReactOS nVidia IDE Controller Minidriver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Main file
 * COPYRIGHT:   Copyright 2022 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "nvide.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

IDE_CHANNEL_STATE
NTAPI
NvIdeChannelEnabled(
    _In_ PVOID DeviceExtension,
    _In_ ULONG Channel)
{
    NTSTATUS Status;
    UCHAR EnableControl;

    ASSERT((Channel & ~1) == 0);

    Status = PciIdeXGetBusData(DeviceExtension,
                               &EnableControl,
                               0x50,
                               sizeof(EnableControl));
    if (!NT_SUCCESS(Status))
    {
        return ChannelStateUnknown;
    }

    DPRINT1("EnableControl 0x%lx Channel 0x%lx", EnableControl, Channel);

    if (EnableControl & (0x02 >> Channel))
    {
        return ChannelEnabled;
    }

    return ChannelDisabled;
}

BOOLEAN
NTAPI
NvIdeSyncAccessRequired(
    _In_ PVOID DeviceExtension)
{
    return FALSE;
}

NTSTATUS
NTAPI
NvIdeTransferModeSelect(
    _In_ PVOID DeviceExtension,
    _Inout_ PPCIIDE_TRANSFER_MODE_SELECT XferMode)
{
    ULONG i;

    DPRINT1("Channel 0x%lx\n", XferMode->Channel);

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        DPRINT1("DevicePresent               0x%lx\n", XferMode->DevicePresent[i]);
        DPRINT1("FixedDisk                   0x%lx\n", XferMode->FixedDisk[i]);
        DPRINT1("IoReadySupported            0x%lx\n", XferMode->IoReadySupported[i]);
        DPRINT1("DeviceTransferModeSupported 0x%lx\n", XferMode->DeviceTransferModeSupported[i]);
        DPRINT1("BestPioCycleTime            0x%lx\n", XferMode->BestPioCycleTime[i]);
        DPRINT1("BestSwDmaCycleTime          0x%lx\n", XferMode->BestSwDmaCycleTime[i]);
        DPRINT1("BestMwDmaCycleTime          0x%lx\n", XferMode->BestMwDmaCycleTime[i]);
        DPRINT1("BestUDmaCycleTime           0x%lx\n", XferMode->BestUDmaCycleTime[i]);
        DPRINT1("DeviceTransferModeCurrent   0x%lx\n", XferMode->DeviceTransferModeCurrent[i]);
        DPRINT1("DeviceTransferModeSelected  0x%lx\n", XferMode->DeviceTransferModeSelected[i]);

        if (!XferMode->DevicePresent[i])
            continue;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NvIdeUdmaModesSupported(
    _In_ IDENTIFY_DATA IdentifyData,
    _Out_ PULONG BestXferMode,
    _Out_ PULONG CurrentXferMode)
{
    DPRINT1("NvIdeUdmaModesSupported()\n");

    return STATUS_NOT_IMPLEMENTED;
}

ULONG
NTAPI
NvIdeUseDma(
    _In_ PVOID DeviceExtension,
    _In_ PUCHAR CdbCommand,
    _In_ PUCHAR Slave)
{
    return 1;
}

NTSTATUS
NTAPI
NvIdeGetControllerProperties(
    _In_ PVOID DeviceExtension,
    _Out_ PIDE_CONTROLLER_PROPERTIES ControllerProperties)
{
    DPRINT1("NvIdeGetControllerProperties\n");

    if (ControllerProperties->Size != sizeof(IDE_CONTROLLER_PROPERTIES))
        return STATUS_REVISION_MISMATCH;

    ControllerProperties->PciIdeChannelEnabled = NvIdeChannelEnabled;
    ControllerProperties->PciIdeSyncAccessRequired = NvIdeSyncAccessRequired;
    ControllerProperties->PciIdeTransferModeSelect = NvIdeTransferModeSelect;
    ControllerProperties->PciIdeUdmaModesSupported = NvIdeUdmaModesSupported;
    ControllerProperties->PciIdeUseDma = NvIdeUseDma;
    ControllerProperties->AlignmentRequirement = 1;

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;

    DPRINT1("DriverEntry()\n");

    Status = PciIdeXInitialize(DriverObject,
                               RegistryPath,
                               NvIdeGetControllerProperties,
                               0);

    return Status;
}
