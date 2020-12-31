/*
 * PROJECT:     ReactOS IDE Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     IDE bus enumeration
 * COPYRIGHT:   Copyright 2022 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

// #define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

CODE_SEG("PAGE")
PCHAR
IdeCopyField(
    _Out_writes_bytes_all_(MaxLength) PCHAR Destination,
    _In_reads_bytes_(MaxLength) PUCHAR Source,
    _In_ ULONG MaxLength,
    _In_ CHAR DefaultCharacter)
{
    PCHAR Dest = Destination;

    PAGED_CODE();

    while (MaxLength)
    {
        UCHAR Char = *Source;

        /* Illegal characters */
        if (Char <= ' ' || Char >= 0x7F || Char == ',')
        {
            *Dest = DefaultCharacter;
        }
        else
        {
            *Dest = Char;
        }

        ++Source;
        ++Dest;
        --MaxLength;
    }

    return Dest;
}

static
CODE_SEG("PAGE")
PCHAR
IdeTrimField(
    _In_ _Post_z_ PCHAR Start,
    _In_ PCHAR End)
{
    PCHAR Current = End - 1;

    PAGED_CODE();

    while (Current >= Start && *Current == ' ')
    {
        --Current;
    }

    Current[1] = ANSI_NULL;

    return (Current + 1);
}

static
CODE_SEG("PAGE")
VOID
IdeSwapField(
    _Inout_updates_bytes_(Length * sizeof(USHORT)) PVOID Buffer,
    _In_range_(>, 0) ULONG Length)
{
    PUSHORT Word = Buffer;

    PAGED_CODE();

    while (Length--)
    {
        *Word = RtlUshortByteSwap(*Word);
        ++Word;
    }
}

static
CODE_SEG("PAGE")
VOID
IdePdoFillIdentificationStrings(
    _In_ PIDEPORT_DEVICE_EXTENSION DeviceExtension)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = &DeviceExtension->IdentifyDeviceData;
    PCHAR End;

    PAGED_CODE();

    /* For ATAPI devices Inquiry Data preferred over Identify Data */
    if (DeviceExtension->Flags & DEVICE_ATAPI)
    {
        End = IdeCopyField(DeviceExtension->FriendlyName,
                           DeviceExtension->InquiryData.VendorId,
                           8,
                           ' ');
        End = IdeTrimField(DeviceExtension->FriendlyName, End);
        *End++ = ' ';
        End = IdeCopyField(End,
                           DeviceExtension->InquiryData.ProductId,
                           16,
                           ' ');
        IdeTrimField(DeviceExtension->FriendlyName, End);

        End = IdeCopyField(DeviceExtension->RevisionNumber,
                           DeviceExtension->InquiryData.ProductRevisionLevel,
                           4,
                           ' ');
        IdeTrimField(DeviceExtension->RevisionNumber, End);
    }
    else
    {
        End = IdeCopyField(DeviceExtension->FriendlyName,
                           IdentifyData->ModelNumber,
                           IDEPORT_FN_FIELD,
                           ' ');
        IdeSwapField(DeviceExtension->FriendlyName, IDEPORT_FN_FIELD / 2);
        IdeTrimField(DeviceExtension->FriendlyName, End);

        End = IdeCopyField(DeviceExtension->RevisionNumber,
                           IdentifyData->FirmwareRevision,
                           IDEPORT_RN_FIELD,
                           ' ');
        IdeSwapField(DeviceExtension->RevisionNumber, IDEPORT_RN_FIELD / 2);
        IdeTrimField(DeviceExtension->RevisionNumber, End);
    }

    if (IdentifyDataSerialNumberValid(IdentifyData))
    {
        ULONG i;
        NTSTATUS Status;
        size_t Remaining;

        End = DeviceExtension->SerialNumber;
        Remaining = sizeof(DeviceExtension->SerialNumber);

        /* Format the serial number */
        for (i = 0; i < sizeof(IdentifyData->SerialNumber); ++i)
        {
            Status = RtlStringCchPrintfExA(End,
                                           Remaining,
                                           &End,
                                           &Remaining,
                                           0,
                                           "%2x",
                                           IdentifyData->SerialNumber[i]);
            if (!NT_VERIFY(NT_SUCCESS(Status)))
            {
                /* This should never happen */
                DeviceExtension->SerialNumber[0] = ANSI_NULL;
                break;
            }
        }
    }
    else
    {
        /* Bogus S/N is ignored */
        DeviceExtension->SerialNumber[0] = ANSI_NULL;
    }

    INFO("FriendlyName: '%s'\n", DeviceExtension->FriendlyName);
    INFO("RevisionNumber: '%s'\n", DeviceExtension->RevisionNumber);
    INFO("SerialNumber: '%s'\n", DeviceExtension->SerialNumber);
}
