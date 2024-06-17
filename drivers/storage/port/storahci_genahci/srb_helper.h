
/*
 * File: srb_helper.h
 */

#ifndef __SRB_HELPER_H__
#define __SRB_HELPER_H__

#define IOCTL_SCSI_MINIPORT_DSM_GENERAL                 ((FILE_DEVICE_SCSI << 16) + 0x0721)
#define IOCTL_SCSI_MINIPORT_HYBRID            ((FILE_DEVICE_SCSI << 16) + 0x0620)

FORCEINLINE PVOID
SrbGetDataBuffer(
    __in PVOID Srb
    )
{
    PSCSI_REQUEST_BLOCK_EX srb = Srb;
    PVOID DataBuffer;

    if (srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK)
    {
        DataBuffer = srb->DataBuffer;
    }
    else
    {
        DataBuffer = ((PSCSI_REQUEST_BLOCK_EX)srb)->DataBuffer;
    }
    return DataBuffer;
}

FORCEINLINE VOID
SrbSetSrbFlags(
    __in PVOID Srb,
    __in ULONG Flags
    )
{
    PSCSI_REQUEST_BLOCK_EX srb = Srb;
    
    if (srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK)
    {
        srb->SrbFlags |= Flags;
    }
    else
    {
        ((PSCSI_REQUEST_BLOCK_EX)srb)->SrbFlags |= Flags;
    }
}


FORCEINLINE PVOID
SrbGetOriginalRequest(
    __in PVOID Srb
    )
{
    PSCSI_REQUEST_BLOCK_EX srb = Srb;
    
    if (srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK)
    {
        return srb->OriginalRequest;
    }
    else
    {
        return ((PSCSI_REQUEST_BLOCK_EX)srb)->OriginalRequest;
    }
}


//
// If you do not wish to use NT_ASSERT, put "#define SRBHELPER_ASSERT ASSERT"
// before including this file.
//
#if !defined(SRBHELPER_ASSERT)
#define SRBHELPER_ASSERT NT_ASSERT
#endif

FORCEINLINE PCDB
SrbGetScsiData(
    __in PSTORAGE_REQUEST_BLOCK SrbEx,
    __in_opt PUCHAR CdbLength8,
    __in_opt PULONG CdbLength32,
    __in_opt PUCHAR ScsiStatus,
    __in_opt PVOID *SenseInfoBuffer,
    __in_opt PUCHAR SenseInfoBufferLength
    )
/*++

Routine Description:

    Helper function to retrieve SCSI related fields from an extended SRB. If SRB is
    not a SRB_FUNCTION_EXECUTE_SCSI or not an extended SRB, default values will be returned.

Arguments:

    SrbEx - Pointer to extended SRB.

    CdbLength8 - Pointer to buffer to hold CdbLength field value for
                 SRBEX_DATA_SCSI_CDB16 or SRBEX_DATA_SCSI_CDB32

    CdbLength32 - Pointer to buffer to hold CdbLength field value for
                  SRBEX_DATA_SCSI_CDB_VAR

    ScsiStatus - Pointer to buffer to hold ScsiStatus field value.

    SenseInfoBuffer - Pointer to buffer to hold SenseInfoBuffer value.

    SenseInfoBufferLength - Pointer to buffer to hold SenseInfoBufferLength value.

Return Value:

    Pointer to Cdb field or NULL if SRB is not a SRB_FUNCTION_EXECUTE_SCSI.

--*/
{
    PCDB Cdb = NULL;
    ULONG i;
    PSRBEX_DATA SrbExData = NULL;
    BOOLEAN FoundEntry = FALSE;

    if ((SrbEx->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK) &&
        (SrbEx->SrbFunction == SRB_FUNCTION_EXECUTE_SCSI)) {
        SRBHELPER_ASSERT(SrbEx->NumSrbExData > 0);

        for (i = 0; i < SrbEx->NumSrbExData; i++) {

            // Skip any invalid offsets
            if ((SrbEx->SrbExDataOffset[i] < sizeof(STORAGE_REQUEST_BLOCK)) ||
                (SrbEx->SrbExDataOffset[i] >= SrbEx->SrbLength)){
                // Catch any invalid offsets
                SRBHELPER_ASSERT(FALSE);
                continue;
            }

            SrbExData = (PSRBEX_DATA)((PUCHAR)SrbEx + SrbEx->SrbExDataOffset[i]);

            switch (SrbExData->Type) {

                case SrbExDataTypeScsiCdb16:
                    if (SrbEx->SrbExDataOffset[i] + sizeof(SRBEX_DATA_SCSI_CDB16) <= SrbEx->SrbLength) {
                        FoundEntry = TRUE;
                        if (CdbLength8) {
                            *CdbLength8 = ((PSRBEX_DATA_SCSI_CDB16) SrbExData)->CdbLength;
                        }

                        if (((PSRBEX_DATA_SCSI_CDB16) SrbExData)->CdbLength > 0) {
                            Cdb = (PCDB)((PSRBEX_DATA_SCSI_CDB16) SrbExData)->Cdb;
                        }

                        if (ScsiStatus) {
                            *ScsiStatus =
                                ((PSRBEX_DATA_SCSI_CDB16) SrbExData)->ScsiStatus;
                        }

                        if (SenseInfoBuffer) {
                            *SenseInfoBuffer =
                                ((PSRBEX_DATA_SCSI_CDB16) SrbExData)->SenseInfoBuffer;
                        }

                        if (SenseInfoBufferLength) {
                            *SenseInfoBufferLength =
                                ((PSRBEX_DATA_SCSI_CDB16) SrbExData)->SenseInfoBufferLength;
                        }

                    } else {
                        // Catch invalid offset
                        SRBHELPER_ASSERT(FALSE);
                    }
                    break;

                case SrbExDataTypeScsiCdb32:
                    if (SrbEx->SrbExDataOffset[i] + sizeof(SRBEX_DATA_SCSI_CDB32) <= SrbEx->SrbLength) {
                        FoundEntry = TRUE;
                        if (CdbLength8) {
                            *CdbLength8 = ((PSRBEX_DATA_SCSI_CDB32) SrbExData)->CdbLength;
                        }

                        if (((PSRBEX_DATA_SCSI_CDB32) SrbExData)->CdbLength > 0) {
                            Cdb = (PCDB)((PSRBEX_DATA_SCSI_CDB32) SrbExData)->Cdb;
                        }

                        if (ScsiStatus) {
                            *ScsiStatus =
                                ((PSRBEX_DATA_SCSI_CDB32) SrbExData)->ScsiStatus;
                        }

                        if (SenseInfoBuffer) {
                            *SenseInfoBuffer =
                                ((PSRBEX_DATA_SCSI_CDB32) SrbExData)->SenseInfoBuffer;
                        }

                        if (SenseInfoBufferLength) {
                            *SenseInfoBufferLength =
                                ((PSRBEX_DATA_SCSI_CDB32) SrbExData)->SenseInfoBufferLength;
                        }

                    } else {
                        // Catch invalid offset
                        SRBHELPER_ASSERT(FALSE);
                    }
                    break;

                case SrbExDataTypeScsiCdbVar:
                    if (SrbEx->SrbExDataOffset[i] + sizeof(SRBEX_DATA_SCSI_CDB_VAR) <= SrbEx->SrbLength) {
                        FoundEntry = TRUE;
                        if (CdbLength32) {
                            *CdbLength32 = ((PSRBEX_DATA_SCSI_CDB_VAR) SrbExData)->CdbLength;
                        }

                        if (((PSRBEX_DATA_SCSI_CDB_VAR) SrbExData)->CdbLength > 0) {
                            Cdb = (PCDB)((PSRBEX_DATA_SCSI_CDB_VAR) SrbExData)->Cdb;
                        }

                        if (ScsiStatus) {
                            *ScsiStatus =
                                ((PSRBEX_DATA_SCSI_CDB_VAR) SrbExData)->ScsiStatus;
                        }

                        if (SenseInfoBuffer) {
                            *SenseInfoBuffer =
                                ((PSRBEX_DATA_SCSI_CDB_VAR) SrbExData)->SenseInfoBuffer;
                        }

                        if (SenseInfoBufferLength) {
                            *SenseInfoBufferLength =
                                ((PSRBEX_DATA_SCSI_CDB_VAR) SrbExData)->SenseInfoBufferLength;
                        }

                    } else {
                        // Catch invalid offset
                        SRBHELPER_ASSERT(FALSE);
                    }
                    break;
            }

            if (FoundEntry) {
                break;
            }
        }

    } else {

        if (CdbLength8) {
            *CdbLength8 = 0;
        }

        if (CdbLength32) {
            *CdbLength32 = 0;
        }

        if (ScsiStatus) {
            *ScsiStatus = SCSISTAT_GOOD;
        }

        if (SenseInfoBuffer) {
            *SenseInfoBuffer = NULL;
        }

        if (SenseInfoBufferLength) {
            *SenseInfoBufferLength = 0;
        }
    }

    return Cdb;
}

FORCEINLINE PCDB
SrbGetCdb(
    __in PVOID Srb
    )
{
    PSTORAGE_REQUEST_BLOCK srb = Srb;
    PCDB pCdb = NULL;

    if (srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK)
    {
        return SrbGetScsiData(srb,
                              NULL,    // CdbLength8
                              NULL,    // CdbLength32
                              NULL,    // ScsiStatus
                              NULL,    // SenseInfoBuffer
                              NULL);   // SenseInfoBufferLength
    }
    else
    {
        pCdb = (PCDB)((PSCSI_REQUEST_BLOCK_EX)srb)->Cdb;
    }
	
    return pCdb;
}

FORCEINLINE VOID
SrbSetScsiData(
    __in PSTORAGE_REQUEST_BLOCK SrbEx,
    __in_opt PUCHAR CdbLength8,
    __in_opt PULONG CdbLength32,
    __in_opt PUCHAR ScsiStatus,
    __in_opt PVOID *SenseInfoBuffer,
    __in_opt PUCHAR SenseInfoBufferLength
    )
/*++

Routine Description:

    Helper function to set SCSI related fields in an extended SRB. If SRB is
    not a SRB_FUNCTION_EXECUTE_SCSI or not an extended SRB, nothing will happen.

    Only the arguments specified will be set in the extended SRB.

Arguments:

    SrbEx - Pointer to extended SRB.

    CdbLength8 - Pointer to buffer that holds the CdbLength field value for
                 SRBEX_DATA_SCSI_CDB16 or SRBEX_DATA_SCSI_CDB32

    CdbLength32 - Pointer to buffer that holds the CdbLength field value for
                  SRBEX_DATA_SCSI_CDB_VAR

    ScsiStatus - Pointer to buffer that holds the ScsiStatus field value.

    SenseInfoBuffer - Pointer to a SenseInfoBuffer pointer.

    SenseInfoBufferLength - Pointer to buffer that holds the SenseInfoBufferLength value.

--*/
{
    ULONG i;
    PSRBEX_DATA SrbExData = NULL;
    BOOLEAN FoundEntry = FALSE;

    if ((SrbEx->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK) &&
        (SrbEx->SrbFunction == SRB_FUNCTION_EXECUTE_SCSI)) {
        SRBHELPER_ASSERT(SrbEx->NumSrbExData > 0);

        for (i = 0; i < SrbEx->NumSrbExData; i++) {

            // Skip any invalid offsets
            if ((SrbEx->SrbExDataOffset[i] < sizeof(PSTORAGE_REQUEST_BLOCK)) ||
                (SrbEx->SrbExDataOffset[i] >= SrbEx->SrbLength)){
                // Catch any invalid offsets
                SRBHELPER_ASSERT(FALSE);
                continue;
            }

            SrbExData = (PSRBEX_DATA)((PUCHAR)SrbEx + SrbEx->SrbExDataOffset[i]);

            switch (SrbExData->Type) {

                case SrbExDataTypeScsiCdb16:
                    if (SrbEx->SrbExDataOffset[i] + sizeof(SRBEX_DATA_SCSI_CDB16) <= SrbEx->SrbLength) {
                        FoundEntry = TRUE;
                        if (CdbLength8) {
                            ((PSRBEX_DATA_SCSI_CDB16) SrbExData)->CdbLength = *CdbLength8;
                        }

                        if (ScsiStatus) {                            
                            ((PSRBEX_DATA_SCSI_CDB16) SrbExData)->ScsiStatus = *ScsiStatus;
                        }

                        if (SenseInfoBuffer) {
                            ((PSRBEX_DATA_SCSI_CDB16) SrbExData)->SenseInfoBuffer = *SenseInfoBuffer;
                        }

                        if (SenseInfoBufferLength) {
                            ((PSRBEX_DATA_SCSI_CDB16) SrbExData)->SenseInfoBufferLength = *SenseInfoBufferLength;
                        }

                    } else {
                        // Catch invalid offset
                        SRBHELPER_ASSERT(FALSE);
                    }
                    break;

                case SrbExDataTypeScsiCdb32:
                    if (SrbEx->SrbExDataOffset[i] + sizeof(SRBEX_DATA_SCSI_CDB32) <= SrbEx->SrbLength) {
                        FoundEntry = TRUE;
                        if (CdbLength8) {
                            ((PSRBEX_DATA_SCSI_CDB32) SrbExData)->CdbLength = *CdbLength8;
                        }

                        if (ScsiStatus) {                            
                            ((PSRBEX_DATA_SCSI_CDB32) SrbExData)->ScsiStatus = *ScsiStatus;
                        }

                        if (SenseInfoBuffer) {
                            ((PSRBEX_DATA_SCSI_CDB32) SrbExData)->SenseInfoBuffer = *SenseInfoBuffer;
                        }

                        if (SenseInfoBufferLength) {
                            ((PSRBEX_DATA_SCSI_CDB32) SrbExData)->SenseInfoBufferLength = *SenseInfoBufferLength;
                        }

                    } else {
                        // Catch invalid offset
                        SRBHELPER_ASSERT(FALSE);
                    }
                    break;

                case SrbExDataTypeScsiCdbVar:
                    if (SrbEx->SrbExDataOffset[i] + sizeof(SRBEX_DATA_SCSI_CDB_VAR) <= SrbEx->SrbLength) {
                        FoundEntry = TRUE;
                        if (CdbLength32) {
                            ((PSRBEX_DATA_SCSI_CDB_VAR) SrbExData)->CdbLength = *CdbLength32;
                        }

                        if (ScsiStatus) {                            
                            ((PSRBEX_DATA_SCSI_CDB_VAR) SrbExData)->ScsiStatus = *ScsiStatus;
                        }

                        if (SenseInfoBuffer) {
                            ((PSRBEX_DATA_SCSI_CDB_VAR) SrbExData)->SenseInfoBuffer = *SenseInfoBuffer;
                        }

                        if (SenseInfoBufferLength) {
                            ((PSRBEX_DATA_SCSI_CDB_VAR) SrbExData)->SenseInfoBufferLength = *SenseInfoBufferLength;
                        }

                    } else {
                        // Catch invalid offset
                        SRBHELPER_ASSERT(FALSE);
                    }
                    break;
            }

            if (FoundEntry) {
                break;
            }
        }
    }
}

FORCEINLINE VOID
SrbSetScsiStatus(
    __in PVOID Srb,
    __in UCHAR ScsiStatus
    )
{
    PSTORAGE_REQUEST_BLOCK srb = Srb;

    if (srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK)
    {
        SrbSetScsiData(srb,
                       NULL,    // CdbLength8
                       NULL,    // CdbLength32
                       &ScsiStatus,    // ScsiStatus
                       NULL,    // SenseInfoBuffer
                       NULL);   // SenseInfoBufferLength
    }
    else
    {
        ((PSCSI_REQUEST_BLOCK_EX)srb)->ScsiStatus = ScsiStatus;
    }
}


FORCEINLINE ULONG
SrbGetDataTransferLength(
    __in PVOID Srb
    )
{
    PSCSI_REQUEST_BLOCK_EX srb = Srb;
    ULONG DataTransferLength;

    if (srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK)
    {
        DataTransferLength = srb->DataTransferLength;
    }
    else
    {
        DataTransferLength = ((PSCSI_REQUEST_BLOCK_EX)srb)->DataTransferLength;
    }
    return DataTransferLength;

}

FORCEINLINE VOID
SrbSetDataTransferLength(
    __in PVOID Srb,
    __in ULONG DataTransferLength
    )
{
    PSCSI_REQUEST_BLOCK_EX srb = Srb;

    if (srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK)
    {
        srb->DataTransferLength = DataTransferLength;
    }
    else
    {
        ((PSCSI_REQUEST_BLOCK_EX)srb)->DataTransferLength = DataTransferLength;
    }
}

FORCEINLINE UCHAR
SrbGetCdbLength(
    __in PVOID Srb
    )
{
    PSTORAGE_REQUEST_BLOCK srb = Srb;
    UCHAR CdbLength = 0;

    if (srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK)
    {
        SrbGetScsiData(srb,
                       &CdbLength,    // CdbLength8
                       NULL,    // CdbLength32
                       NULL,    // ScsiStatus
                       NULL,    // SenseInfoBuffer
                       NULL);   // SenseInfoBufferLength
    }
    else
    {
        CdbLength = ((PSCSI_REQUEST_BLOCK_EX)srb)->CdbLength;
    }
    return CdbLength;
}


FORCEINLINE PVOID
SrbGetNextSrb(
    __in PVOID Srb
    )
{
    PSCSI_REQUEST_BLOCK_EX srb = Srb;
    
    if (srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK)
    {
        return (PVOID)srb->NextSrb;
    }
    else
    {
        return (PVOID)((PSCSI_REQUEST_BLOCK_EX)srb)->NextSrb;
    }
}

FORCEINLINE VOID
SrbSetNextSrb(
    __in PVOID Srb,
    __in_opt PVOID NextSrb
    )
{
    PSCSI_REQUEST_BLOCK_EX srb = Srb;
    
    if (srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK)
    {
        srb->NextSrb = (PSCSI_REQUEST_BLOCK_EX)NextSrb;
    }
    else
    {
        ((PSCSI_REQUEST_BLOCK_EX)srb)->NextSrb = (PSCSI_REQUEST_BLOCK_EX)NextSrb;
    }
}

FORCEINLINE ULONG
SrbGetSrbFunction(
    __in PVOID Srb
    )
{
    PSTORAGE_REQUEST_BLOCK srb = Srb;

    if (srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK)
    {
        return srb->SrbFunction;
    }
    else
    {
        return (ULONG)((PSTORAGE_REQUEST_BLOCK)srb)->Function;
    }
}


//
// If you do not wish to use NT_ASSERT, put "#define SRBHELPER_ASSERT ASSERT"
// before including this file.
//
#if !defined(SRBHELPER_ASSERT)
#define SRBHELPER_ASSERT NT_ASSERT
#endif

FORCEINLINE PSTOR_ADDRESS
SrbGetAddress(
    __in PSTORAGE_REQUEST_BLOCK Srb
    )
{
    PSTOR_ADDRESS storAddr = NULL;

    if (Srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK)
    {
        SRBHELPER_ASSERT(Srb->AddressOffset);
        
        if (Srb->AddressOffset)
        {       
            storAddr = (PSTOR_ADDRESS)((PUCHAR)Srb + Srb->AddressOffset);

            //
            // We currently only support BTL8, so assert if the type is something
            // different.
            //
            SRBHELPER_ASSERT(storAddr->Type == STOR_ADDRESS_TYPE_BTL8);
        }
    }

    return storAddr;
}

FORCEINLINE VOID
SrbGetPathTargetLun(
    __in PVOID Srb,
    __in_opt PUCHAR PathId,
    __in_opt PUCHAR TargetId,
    __in_opt PUCHAR Lun
    )
{
    PSTORAGE_REQUEST_BLOCK srb = Srb;
    PSTOR_ADDRESS storAddr = NULL;

    if (srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK)
    {
        storAddr = (PSTOR_ADDRESS)SrbGetAddress(srb);
        if (storAddr) 
        {
            switch (storAddr->Type) 
            {            
                case STOR_ADDRESS_TYPE_BTL8:
                    if (PathId != NULL) {
                        *PathId = ((PSTOR_ADDR_BTL8)storAddr)->Path;
                    }

                    if (TargetId != NULL) {
                        *TargetId = ((PSTOR_ADDR_BTL8)storAddr)->Target;
                    }

                    if (Lun != NULL) {
                        *Lun = ((PSTOR_ADDR_BTL8)storAddr)->Lun;
                    }

                    break;

                default:
                    SRBHELPER_ASSERT(FALSE);
                    break;
            }
        }
    }
    else
    {
        if (PathId != NULL) {
            *PathId = ((PSCSI_REQUEST_BLOCK_EX)srb)->PathId;
        }

        if (TargetId != NULL) {
            *TargetId = ((PSCSI_REQUEST_BLOCK_EX)srb)->TargetId;
        }

        if (Lun != NULL) {
            *Lun = ((PSCSI_REQUEST_BLOCK_EX)srb)->Lun;
        }
    }

    return;
}

FORCEINLINE UCHAR
SrbGetLun(
    __in PVOID Srb
    )
{
    PSTORAGE_REQUEST_BLOCK srb = Srb;
    UCHAR Lun = 0;
    PSTOR_ADDRESS storAddr = NULL;

    if (srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK)
    {
        storAddr = (PSTOR_ADDRESS)SrbGetAddress(srb);
        if (storAddr) 
        {
            switch (storAddr->Type) 
            {            
                case STOR_ADDRESS_TYPE_BTL8:
                    Lun = ((PSTOR_ADDR_BTL8)storAddr)->Lun;
                    break;

                default:
                    SRBHELPER_ASSERT(FALSE);
                    break;
            }
        }
    }
    else
    {
        Lun = ((PSCSI_REQUEST_BLOCK_EX)srb)->Lun;
    }
    return Lun;
}




FORCEINLINE ULONG
SrbGetSrbFlags(
    __in PVOID Srb
    )
{
    PSCSI_REQUEST_BLOCK_EX srb = Srb;
    ULONG srbFlags;

    if (srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK)
    {
        srbFlags = srb->SrbFlags;
    }
    else
    {
        srbFlags = ((PSCSI_REQUEST_BLOCK_EX)srb)->SrbFlags;
    }
    return srbFlags;
}

FORCEINLINE PSRBEX_DATA
SrbGetSrbExDataByType(
    __in PSTORAGE_REQUEST_BLOCK Srb,
    __in SRBEXDATATYPE Type
    )
{   
    if ((Srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK) &&
        (Srb->NumSrbExData > 0)) 
    {
        PSRBEX_DATA srbExData = NULL;
        UCHAR i = 0;
        
        for (i = 0; i < Srb->NumSrbExData; i++) 
        {
            if (Srb->SrbExDataOffset[i] >= sizeof(PSCSI_REQUEST_BLOCK_EX) &&
                Srb->SrbExDataOffset[i] < Srb->SrbLength) 
            {                
                srbExData = (PSRBEX_DATA)((PUCHAR)Srb + Srb->SrbExDataOffset[i]);
                if (srbExData->Type == Type) 
                {
                    return srbExData;
                }
            }
        }
    }

    return NULL;
}

FORCEINLINE UCHAR
SrbGetPathId(
    __in PVOID Srb
    )
{
    PSTORAGE_REQUEST_BLOCK srb = Srb;
    UCHAR PathId = 0;
    PSTOR_ADDRESS storAddr = NULL;

    if (srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK)
    {
        storAddr = (PSTOR_ADDRESS)SrbGetAddress(srb);
        if (storAddr) 
        {
            switch (storAddr->Type) 
            {            
                case STOR_ADDRESS_TYPE_BTL8:
                    PathId = ((PSTOR_ADDR_BTL8)storAddr)->Path;
                    break;

                default:
                    SRBHELPER_ASSERT(FALSE);
                    break;
            }
        }
    }
    else
    {
        PathId = ((PSCSI_REQUEST_BLOCK_EX)srb)->PathId;
    }
    return PathId;

}

// From ReactOS common.h

typedef enum _STORPORT_ETW_EVENT_CHANNEL{
    StorportEtwEventDiagnostic = 0,
    StorportEtwEventOperational = 1,
    StorportEtwEventHealth = 2
} STORPORT_ETW_EVENT_CHANNEL, *PSTORPORT_ETW_EVENT_CHANNEL;

ULONG
FORCEINLINE
StorPortEtwChannelEvent2(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ STORPORT_ETW_EVENT_CHANNEL EventChannel,
    _In_ ULONG EventId,
    _In_reads_or_z_(STORPORT_ETW_MAX_DESCRIPTION_LENGTH) PWSTR EventDescription,
    _In_ ULONGLONG EventKeywords,
    _In_ STORPORT_ETW_LEVEL EventLevel,
    _In_ STORPORT_ETW_EVENT_OPCODE EventOpcode,
    _In_opt_ PSCSI_REQUEST_BLOCK Srb,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter1Name,
    _In_ ULONGLONG Parameter1Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter2Name,
    _In_ ULONGLONG Parameter2Value
)
/*
Description:
    A miniport can call this function to log an ETW event to a specific channel with
    two extra general purpose parameters (expressed as name-value pairs).

Parameters:
    HwDeviceExtension - The miniport's device extension.
    Address - NULL if the device is an adapter, otherwise the address specifies
        the unit object.
    EventChannel - ETW channel where event is logged
    EventId - A miniport-specific event ID to uniquely identify the type of event.
    EventDescription - Required.  A short string describing the event.  Must
        not be longer than STORPORT_ETW_MAX_DESCRIPTION_LENGTH characters,
        not including the NULL terminator.
    EventKeywords - Bitmask of STORPORT_ETW_EVENT_KEYWORD_* values to further
        characterize the event.  Can be 0 if no keywords are desired.
    EventLevel - The level of the event (e.g. Informational, Error, etc.).
    EventOpcode - The opcode of the event (e.g. Info, Start, Stop, etc.).
    Srb - Optional pointer to an SRB.  If specified, the SRB pointer and the
        pointer of the associated IRP will be logged.
    Parameter<N>Name - A short string that gives meaning to parameter N's value.
        If NULL or an empty string, parameter N will be ignored.  Must not be
        longer than STORPORT_ETW_MAX_PARAM_NAME_LENGTH characters, not including
        the NULL terminator.
    Parameter<N>Value - Value of parameter N.  If the associated parameter N
        name is NULL or empty, the value will be logged as 0.

Returns:
    STOR_STATUS_SUCCESS if the ETW event was successfully logged.
    STOR_STATUS_INVALID_PARAMETER if there is an invalid parameter. This is
        typically returned if a passed-in string has too many characters.
    STOR_STATUS_UNSUCCESSFUL may also be returned for other, internal reasons.

*/
{
    ULONG status = STOR_STATUS_NOT_IMPLEMENTED;

#if (NTDDI_VERSION >= NTDDI_WIN10_RS5)

    status = StorPortExtendedFunction(ExtFunctionMiniportChannelEtwEvent2,
                                        HwDeviceExtension,
                                        Address,
                                        EventChannel,
                                        EventId,
                                        EventDescription,
                                        EventKeywords,
                                        EventLevel,
                                        EventOpcode,
                                        Srb,
                                        Parameter1Name,
                                        Parameter1Value,
                                        Parameter2Name,
                                        Parameter2Value);
#else
    UNREFERENCED_PARAMETER(HwDeviceExtension);
    UNREFERENCED_PARAMETER(Address);
    UNREFERENCED_PARAMETER(EventChannel);
    UNREFERENCED_PARAMETER(EventId);
    UNREFERENCED_PARAMETER(EventDescription);
    UNREFERENCED_PARAMETER(EventKeywords);
    UNREFERENCED_PARAMETER(EventLevel);
    UNREFERENCED_PARAMETER(EventOpcode);
    UNREFERENCED_PARAMETER(Srb);
    UNREFERENCED_PARAMETER(Parameter1Name);
    UNREFERENCED_PARAMETER(Parameter1Value);
    UNREFERENCED_PARAMETER(Parameter2Name);
    UNREFERENCED_PARAMETER(Parameter2Value);
#endif

    return status;
}

#ifndef __REACTOS__
ULONG
FORCEINLINE
StorPortEtwEvent2(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ ULONG EventId,
    _In_reads_or_z_(STORPORT_ETW_MAX_DESCRIPTION_LENGTH) PWSTR EventDescription,
    _In_ ULONGLONG EventKeywords,
    _In_ STORPORT_ETW_LEVEL EventLevel,
    _In_ STORPORT_ETW_EVENT_OPCODE EventOpcode,
    _In_opt_ PSCSI_REQUEST_BLOCK Srb,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter1Name,
    _In_ ULONGLONG Parameter1Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter2Name,
    _In_ ULONGLONG Parameter2Value
)
/*
Description:
    A miniport can call this function to log an ETW event with two extra
    general purpose parameters (expressed as name-value pairs).

Parameters:
    HwDeviceExtension - The miniport's device extension.
    Address - NULL if the device is an adapter, otherwise the address specifies
        the unit object.
    EventId - A miniport-specific event ID to uniquely identify the type of event.
    EventDescription - Required.  A short string describing the event.  Must
        not be longer than STORPORT_ETW_MAX_DESCRIPTION_LENGTH characters,
        not including the NULL terminator.
    EventKeywords - Bitmask of STORPORT_ETW_EVENT_KEYWORD_* values to further
        characterize the event.  Can be 0 if no keywords are desired.
    EventLevel - The level of the event (e.g. Informational, Error, etc.).
    EventOpcode - The opcode of the event (e.g. Info, Start, Stop, etc.).
    Srb - Optional pointer to an SRB.  If specified, the SRB pointer and the
        pointer of the associated IRP will be logged.
    Parameter<N>Name - A short string that gives meaning to parameter N's value.
        If NULL or an empty string, parameter N will be ignored.  Must not be
        longer than STORPORT_ETW_MAX_PARAM_NAME_LENGTH characters, not including
        the NULL terminator.
    Parameter<N>Value - Value of parameter N.  If the associated parameter N
        name is NULL or empty, the value will be logged as 0.

Returns:
    STOR_STATUS_SUCCESS if the ETW event was successfully logged.
    STOR_STATUS_INVALID_PARAMETER if there is an invalid parameter. This is
        typically returned if a passed-in string has too many characters.
    STOR_STATUS_UNSUCCESSFUL may also be returned for other, internal reasons.

*/
{
    ULONG status = STOR_STATUS_NOT_IMPLEMENTED;

#if (NTDDI_VERSION >= NTDDI_WIN10_RS5)

    status = StorPortExtendedFunction(ExtFunctionMiniportChannelEtwEvent2,
                                        HwDeviceExtension,
                                        Address,
                                        StorportEtwEventDiagnostic,
                                        EventId,
                                        EventDescription,
                                        EventKeywords,
                                        EventLevel,
                                        EventOpcode,
                                        Srb,
                                        Parameter1Name,
                                        Parameter1Value,
                                        Parameter2Name,
                                        Parameter2Value);
#elif (NTDDI_VERSION >= NTDDI_WINBLUE)

    status = StorPortExtendedFunction(ExtFunctionMiniportEtwEvent2,
                                        HwDeviceExtension,
                                        Address,
                                        EventId,
                                        EventDescription,
                                        EventKeywords,
                                        EventLevel,
                                        EventOpcode,
                                        Srb,
                                        Parameter1Name,
                                        Parameter1Value,
                                        Parameter2Name,
                                        Parameter2Value);
#else
    UNREFERENCED_PARAMETER(HwDeviceExtension);
    UNREFERENCED_PARAMETER(Address);
    UNREFERENCED_PARAMETER(EventId);
    UNREFERENCED_PARAMETER(EventDescription);
    UNREFERENCED_PARAMETER(EventKeywords);
    UNREFERENCED_PARAMETER(EventLevel);
    UNREFERENCED_PARAMETER(EventOpcode);
    UNREFERENCED_PARAMETER(Srb);
    UNREFERENCED_PARAMETER(Parameter1Name);
    UNREFERENCED_PARAMETER(Parameter1Value);
    UNREFERENCED_PARAMETER(Parameter2Name);
    UNREFERENCED_PARAMETER(Parameter2Value);
#endif

    return status;
}
#endif

ULONG
FORCEINLINE
StorPortEtwEvent8(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ ULONG EventId,
    _In_reads_or_z_(STORPORT_ETW_MAX_DESCRIPTION_LENGTH) PWSTR EventDescription,
    _In_ ULONGLONG EventKeywords,
    _In_ STORPORT_ETW_LEVEL EventLevel,
    _In_ STORPORT_ETW_EVENT_OPCODE EventOpcode,
    _In_opt_ PSCSI_REQUEST_BLOCK Srb,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter1Name,
    _In_ ULONGLONG Parameter1Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter2Name,
    _In_ ULONGLONG Parameter2Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter3Name,
    _In_ ULONGLONG Parameter3Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter4Name,
    _In_ ULONGLONG Parameter4Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter5Name,
    _In_ ULONGLONG Parameter5Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter6Name,
    _In_ ULONGLONG Parameter6Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter7Name,
    _In_ ULONGLONG Parameter7Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter8Name,
    _In_ ULONGLONG Parameter8Value
)
/*
Description:
    A miniport can call this function to log an ETW event with eight extra
    general purpose parameters (expressed as name-value pairs).

Parameters:
    HwDeviceExtension - The miniport's device extension.
    Address - NULL if the device is an adapter, otherwise the address specifies
        the unit object.
    EventId - A miniport-specific event ID to uniquely identify the type of event.
    EventDescription - Required.  A short string describing the event.  Must
        not be longer than STORPORT_ETW_MAX_DESCRIPTION_LENGTH characters,
        not including the NULL terminator.
    EventKeywords - Bitmask of STORPORT_ETW_EVENT_KEYWORD_* values to further
        characterize the event.  Can be 0 if no keywords are desired.
    EventLevel - The level of the event (e.g. Informational, Error, etc.).
    EventOpcode - The opcode of the event (e.g. Info, Start, Stop, etc.).
    Srb - Optional pointer to an SRB.  If specified, the SRB pointer and the
        pointer of the associated IRP will be logged.
    Parameter<N>Name - A short string that gives meaning to parameter N's value.
        If NULL or an empty string, parameter N will be ignored.  Must not be
        longer than STORPORT_ETW_MAX_PARAM_NAME_LENGTH characters, not including
        the NULL terminator.
    Parameter<N>Value - Value of parameter N.  If the associated parameter N
        name is NULL or empty, the value will be logged as 0.

Returns:
    STOR_STATUS_SUCCESS if the ETW event was successfully logged.
    STOR_STATUS_INVALID_PARAMETER if there is an invalid parameter. This is
        typically returned if a passed-in string has too many characters.
    STOR_STATUS_UNSUCCESSFUL may also be returned for other, internal reasons.

*/
{
    ULONG status = STOR_STATUS_NOT_IMPLEMENTED;

#if (NTDDI_VERSION >= NTDDI_WIN10_RS5)

    status = StorPortExtendedFunction(ExtFunctionMiniportChannelEtwEvent8,
                                    HwDeviceExtension,
                                    Address,
                                    StorportEtwEventDiagnostic,
                                    EventId,
                                    EventDescription,
                                    EventKeywords,
                                    EventLevel,
                                    EventOpcode,
                                    Srb,
                                    Parameter1Name,
                                    Parameter1Value,
                                    Parameter2Name,
                                    Parameter2Value,
                                    Parameter3Name,
                                    Parameter3Value,
                                    Parameter4Name,
                                    Parameter4Value,
                                    Parameter5Name,
                                    Parameter5Value,
                                    Parameter6Name,
                                    Parameter6Value,
                                    Parameter7Name,
                                    Parameter7Value,
                                    Parameter8Name,
                                    Parameter8Value);
#elif (NTDDI_VERSION >= NTDDI_WINBLUE)

    status = StorPortExtendedFunction(ExtFunctionMiniportEtwEvent8,
                                    HwDeviceExtension,
                                    Address,
                                    EventId,
                                    EventDescription,
                                    EventKeywords,
                                    EventLevel,
                                    EventOpcode,
                                    Srb,
                                    Parameter1Name,
                                    Parameter1Value,
                                    Parameter2Name,
                                    Parameter2Value,
                                    Parameter3Name,
                                    Parameter3Value,
                                    Parameter4Name,
                                    Parameter4Value,
                                    Parameter5Name,
                                    Parameter5Value,
                                    Parameter6Name,
                                    Parameter6Value,
                                    Parameter7Name,
                                    Parameter7Value,
                                    Parameter8Name,
                                    Parameter8Value);
#else
    UNREFERENCED_PARAMETER(HwDeviceExtension);
    UNREFERENCED_PARAMETER(Address);
    UNREFERENCED_PARAMETER(EventId);
    UNREFERENCED_PARAMETER(EventDescription);
    UNREFERENCED_PARAMETER(EventKeywords);
    UNREFERENCED_PARAMETER(EventLevel);
    UNREFERENCED_PARAMETER(EventOpcode);
    UNREFERENCED_PARAMETER(Srb);
    UNREFERENCED_PARAMETER(Parameter1Name);
    UNREFERENCED_PARAMETER(Parameter1Value);
    UNREFERENCED_PARAMETER(Parameter2Name);
    UNREFERENCED_PARAMETER(Parameter2Value);
    UNREFERENCED_PARAMETER(Parameter3Name);
    UNREFERENCED_PARAMETER(Parameter3Value);
    UNREFERENCED_PARAMETER(Parameter4Name);
    UNREFERENCED_PARAMETER(Parameter4Value);
    UNREFERENCED_PARAMETER(Parameter5Name);
    UNREFERENCED_PARAMETER(Parameter5Value);
    UNREFERENCED_PARAMETER(Parameter6Name);
    UNREFERENCED_PARAMETER(Parameter6Value);
    UNREFERENCED_PARAMETER(Parameter7Name);
    UNREFERENCED_PARAMETER(Parameter7Value);
    UNREFERENCED_PARAMETER(Parameter8Name);
    UNREFERENCED_PARAMETER(Parameter8Value);
#endif

    return status;
}




ULONG
FORCEINLINE
StorPortEtwChannelEvent8(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ STORPORT_ETW_EVENT_CHANNEL EventChannel,
    _In_ ULONG EventId,
    _In_reads_or_z_(STORPORT_ETW_MAX_DESCRIPTION_LENGTH) PWSTR EventDescription,
    _In_ ULONGLONG EventKeywords,
    _In_ STORPORT_ETW_LEVEL EventLevel,
    _In_ STORPORT_ETW_EVENT_OPCODE EventOpcode,
    _In_opt_ PSCSI_REQUEST_BLOCK Srb,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter1Name,
    _In_ ULONGLONG Parameter1Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter2Name,
    _In_ ULONGLONG Parameter2Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter3Name,
    _In_ ULONGLONG Parameter3Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter4Name,
    _In_ ULONGLONG Parameter4Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter5Name,
    _In_ ULONGLONG Parameter5Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter6Name,
    _In_ ULONGLONG Parameter6Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter7Name,
    _In_ ULONGLONG Parameter7Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter8Name,
    _In_ ULONGLONG Parameter8Value
)
/*
Description:
    A miniport can call this function to log an ETW event to a specific channel
    with eight extra general purpose parameters (expressed as name-value pairs).

Parameters:
    HwDeviceExtension - The miniport's device extension.
    Address - NULL if the device is an adapter, otherwise the address specifies
        the unit object.
    EventChannel - ETW channel where event is logged
    EventId - A miniport-specific event ID to uniquely identify the type of event.
    EventDescription - Required.  A short string describing the event.  Must
        not be longer than STORPORT_ETW_MAX_DESCRIPTION_LENGTH characters,
        not including the NULL terminator.
    EventKeywords - Bitmask of STORPORT_ETW_EVENT_KEYWORD_* values to further
        characterize the event.  Can be 0 if no keywords are desired.
    EventLevel - The level of the event (e.g. Informational, Error, etc.).
    EventOpcode - The opcode of the event (e.g. Info, Start, Stop, etc.).
    Srb - Optional pointer to an SRB.  If specified, the SRB pointer and the
        pointer of the associated IRP will be logged.
    Parameter<N>Name - A short string that gives meaning to parameter N's value.
        If NULL or an empty string, parameter N will be ignored.  Must not be
        longer than STORPORT_ETW_MAX_PARAM_NAME_LENGTH characters, not including
        the NULL terminator.
    Parameter<N>Value - Value of parameter N.  If the associated parameter N
        name is NULL or empty, the value will be logged as 0.

Returns:
    STOR_STATUS_SUCCESS if the ETW event was successfully logged.
    STOR_STATUS_INVALID_PARAMETER if there is an invalid parameter. This is
        typically returned if a passed-in string has too many characters.
    STOR_STATUS_UNSUCCESSFUL may also be returned for other, internal reasons.

*/
{
    ULONG status = STOR_STATUS_NOT_IMPLEMENTED;

#if (NTDDI_VERSION >= NTDDI_WIN10_RS5)

    status = StorPortExtendedFunction(ExtFunctionMiniportChannelEtwEvent8,
                                    HwDeviceExtension,
                                    Address,
                                    EventChannel,
                                    EventId,
                                    EventDescription,
                                    EventKeywords,
                                    EventLevel,
                                    EventOpcode,
                                    Srb,
                                    Parameter1Name,
                                    Parameter1Value,
                                    Parameter2Name,
                                    Parameter2Value,
                                    Parameter3Name,
                                    Parameter3Value,
                                    Parameter4Name,
                                    Parameter4Value,
                                    Parameter5Name,
                                    Parameter5Value,
                                    Parameter6Name,
                                    Parameter6Value,
                                    Parameter7Name,
                                    Parameter7Value,
                                    Parameter8Name,
                                    Parameter8Value);
#else
    UNREFERENCED_PARAMETER(HwDeviceExtension);
    UNREFERENCED_PARAMETER(Address);
    UNREFERENCED_PARAMETER(EventChannel);
    UNREFERENCED_PARAMETER(EventId);
    UNREFERENCED_PARAMETER(EventDescription);
    UNREFERENCED_PARAMETER(EventKeywords);
    UNREFERENCED_PARAMETER(EventLevel);
    UNREFERENCED_PARAMETER(EventOpcode);
    UNREFERENCED_PARAMETER(Srb);
    UNREFERENCED_PARAMETER(Parameter1Name);
    UNREFERENCED_PARAMETER(Parameter1Value);
    UNREFERENCED_PARAMETER(Parameter2Name);
    UNREFERENCED_PARAMETER(Parameter2Value);
    UNREFERENCED_PARAMETER(Parameter3Name);
    UNREFERENCED_PARAMETER(Parameter3Value);
    UNREFERENCED_PARAMETER(Parameter4Name);
    UNREFERENCED_PARAMETER(Parameter4Value);
    UNREFERENCED_PARAMETER(Parameter5Name);
    UNREFERENCED_PARAMETER(Parameter5Value);
    UNREFERENCED_PARAMETER(Parameter6Name);
    UNREFERENCED_PARAMETER(Parameter6Value);
    UNREFERENCED_PARAMETER(Parameter7Name);
    UNREFERENCED_PARAMETER(Parameter7Value);
    UNREFERENCED_PARAMETER(Parameter8Name);
    UNREFERENCED_PARAMETER(Parameter8Value);
#endif

    return status;
}


__inline
PCDB
RequestGetSrbScsiData (
    __in PSCSI_REQUEST_BLOCK_EX Srb,
    __in_opt PULONG             CdbLength,
    __in_opt PUCHAR             ScsiStatus,
    __in_opt PVOID*             SenseInfoBuffer,
    __in_opt PUCHAR             SenseInfoBufferLength
    )
/*++

Routine Description:

    Helper function to retrieve SCSI related fields from a SRB.

Arguments:

    Srb - Pointer to Srb or SrbEx.

    CdbLength - Pointer to buffer to hold CdbLength field value for CDB

    ScsiStatus - Buffer to hold address ScsiStatus field value.

    SenseInfoBuffer - Pointer to SenseInfoBuffer buffer.

    SenseInfoBufferLength - Pointer to buffer to hold SenseInfoBufferLength value.

Return Value:

    Pointer to Cdb field

--*/
{
    PCDB                    cdb = NULL;

    if (Srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK) {
        //
        // This is SrbEx - STORAGE_REQUEST_BLOCK
        //
        cdb = SrbGetScsiData((PSTORAGE_REQUEST_BLOCK)Srb, (PUCHAR)CdbLength, CdbLength, ScsiStatus, SenseInfoBuffer, SenseInfoBufferLength);

    }  else if (Srb->Function == SRB_FUNCTION_EXECUTE_SCSI) {
        //
        // This is legacy SCSI_REQUEST_BLOCK_EX
        //
        PSCSI_REQUEST_BLOCK_EX  srb = (PSCSI_REQUEST_BLOCK_EX)Srb;

        if (CdbLength) {
            *CdbLength = srb->CdbLength;
        }

        if (srb->CdbLength > 0) {
            cdb = (PCDB)srb->Cdb;
        }

        if (ScsiStatus) {
            *ScsiStatus = srb->ScsiStatus;
        }

        if (SenseInfoBuffer) {
            *SenseInfoBuffer = srb->SenseInfoBuffer;
        }

        if (SenseInfoBufferLength) {
            *SenseInfoBufferLength = srb->SenseInfoBufferLength;
        }

    } else {
        if (CdbLength) {
            *CdbLength = 0;
        }

        if (ScsiStatus) {
            *ScsiStatus = SCSISTAT_GOOD;
        }

        if (SenseInfoBuffer) {
            *SenseInfoBuffer = NULL;
        }

        if (SenseInfoBufferLength) {
            *SenseInfoBufferLength = 0;
        }
    }

    return cdb;
}



#endif /* __SRB_HELPER_H__ */

