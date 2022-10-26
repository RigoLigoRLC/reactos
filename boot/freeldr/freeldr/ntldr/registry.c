/*
 *  FreeLoader
 *
 *  Copyright (C) 2014  Timo Kreuzer <timo.kreuzer@reactos.org>
 *                2022  George Bișoc <george.bisoc@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>
#include <cmlib.h>
#include "registry.h"
#include <internal/cmboot.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(REGISTRY);

static PCMHIVE CmSystemHive;
static HCELL_INDEX SystemRootCell;
BOOLEAN CmpSelfHeal = TRUE;

PHHIVE SystemHive = NULL;
HKEY CurrentControlSetKey = NULL;

#define HCI_TO_HKEY(CellIndex)          ((HKEY)(ULONG_PTR)(CellIndex))
#ifndef HKEY_TO_HCI // See also registry.h
#define HKEY_TO_HCI(hKey)               ((HCELL_INDEX)(ULONG_PTR)(hKey))
#endif

#define GET_HHIVE(CmHive)               (&((CmHive)->Hive))
#define GET_HHIVE_FROM_HKEY(hKey)       GET_HHIVE(CmSystemHive)
#define GET_CM_KEY_NODE(hHive, hKey)    ((PCM_KEY_NODE)HvGetCell(hHive, HKEY_TO_HCI(hKey)))

#define GET_HBASE_BLOCK(ChunkBase)      ((PHBASE_BLOCK)ChunkBase)
#define GET_REG_ADDRESS(Reg)            ((PVOID)Reg)

PVOID
NTAPI
CmpAllocate(
    IN SIZE_T Size,
    IN BOOLEAN Paged,
    IN ULONG Tag)
{
    UNREFERENCED_PARAMETER(Paged);
    return FrLdrHeapAlloc(Size, Tag);
}

VOID
NTAPI
CmpFree(
    IN PVOID Ptr,
    IN ULONG Quota)
{
    UNREFERENCED_PARAMETER(Quota);
    FrLdrHeapFree(Ptr, 0);
}

/**
 * @brief
 * Initializes a flat hive descriptor for the
 * SYSTEM hive and validates the registry hive.
 * Volatile data is purged during this procedure
 * for initialization.
 *
 * @param[in] CmHive
 * A pointer to a CM (in-memory) hive descriptor
 * containing the hive descriptor to be initialized.
 *
 * @param[in] ChunkBase
 * A an arbitrary pointer that points to the registry
 * chunk base. This pointer serves as the base block
 * containing the hive file header data.
 *
 * @return
 * Returns TRUE if the SYSTEM hive has been initialized
 * and registry data inside the hive is valid, FALSE
 * otherwise.
 */
BOOLEAN
RegInitializeSystemHive(
    _In_ PCMHIVE CmHive,
    _In_ PVOID ChunkBase)
{
    NTSTATUS Status;
/*
 * FIXME: Disable compilation of some parts of code for AMD64 for now,
 * since it makes the FreeLdr binary size so large that it prevents
 * x64 ROS from booting.
 */
#if !defined(_M_AMD64)
    CM_CHECK_REGISTRY_STATUS CmStatusCode;
#endif

    /* Initialize the hive */
    Status = HvInitialize(GET_HHIVE(CmHive),
                          HINIT_FLAT, // HINIT_MEMORY_INPLACE
                          0,
                          HFILE_TYPE_PRIMARY,
                          ChunkBase,
                          CmpAllocate,
                          CmpFree,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          1,
                          NULL);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

/* FIXME: See the comment above */
#if !defined(_M_AMD64)
    /* Now check the hive and purge volatile data */
    CmStatusCode = CmCheckRegistry(CmHive, CM_CHECK_REGISTRY_BOOTLOADER_PURGE_VOLATILES | CM_CHECK_REGISTRY_VALIDATE_HIVE);
    if (!CM_CHECK_REGISTRY_SUCCESS(CmStatusCode))
    {
        return FALSE;
    }
#endif

    return TRUE;
}

/* FIXME: See the comment above */
#if !defined(_M_AMD64)
/**
 * @brief
 * Loads the system log data for SYSTEM
 * hive recovery.
 *
 * @param[out] SystemLogData
 * An arbitrary pointer returned by the
 * function that points to the system
 * log data.
 *
 * @return
 * Returns TRUE if the log data could
 * be loaded, FALSE otherwise.
 */
BOOLEAN
RegLoadSystemLogForRecovery(
    _Out_ PVOID SystemLogData)
{
    ARC_STATUS Status;
    ULONG FileId;
    CHAR SystemLogPath[MAX_PATH];
    ULONG LogFileSize;
    FILEINFORMATION FileInfo;
    ULONG BytesRead;
    PVOID LogDataVirtual;
    PVOID LogDataPhysical;

    /* Build the full path to the system log */
    RtlStringCbCopyA(SystemLogPath, sizeof(SystemLogPath), "system32\\config\\SYSTEM.LOG");

    /* Open the file */
    Status = ArcOpen(SystemLogPath, OpenReadOnly, &FileId);
    if (Status != ESUCCESS)
    {
        return FALSE;
    }

    /* Get the file length */
    Status = ArcGetFileInformation(FileId, &FileInfo);
    if (Status != ESUCCESS)
    {
        ArcClose(FileId);
        return FALSE;
    }

    /* Capture the size of the system log file */
    LogFileSize = FileInfo.EndingAddress.LowPart;
    if (LogFileSize == 0)
    {
        ArcClose(FileId);
        return FALSE;
    }

    /* Allocate memory blocks for our log data */
    LogDataPhysical = MmAllocateMemoryWithType(
        MM_SIZE_TO_PAGES(LogFileSize + MM_PAGE_SIZE - 1) << MM_PAGE_SHIFT,
        LoaderRegistryData);
    if (LogDataPhysical == NULL)
    {
        ArcClose(FileId);
        return FALSE;
    }

    /* Convert address to virtual and assign it to caller's parameter */
    LogDataVirtual = PaToVa(LogDataPhysical);
    SystemLogData = LogDataVirtual;

    /* Read the system log now */
    Status = ArcRead(FileId, LogDataPhysical, LogFileSize, &BytesRead);
    if (Status != ESUCCESS)
    {
        ArcClose(FileId);
        return FALSE;
    }

    ArcClose(FileId);
    return TRUE;
}

/**
 * @brief
 * Copies the registry data from the log
 * into the primary flat SYSTEM hive. This
 * is done during recovery process of the
 * hive.
 *
 * @param[in] LogBaseBlock
 * A pointer to a base block of the log.
 * This parameter is used to gather the
 * clustering factor of the log.
 *
 * @param[in] HiveLength
 * The hive space length of the primary
 * SYSTEM hive. This length takes into
 * account the whole data present in hive
 * minus the header. This parameter is used
 * to calculate the necessary bitmap size
 * for data copying into the hive.
 *
 * @param[in] SystemHive
 * A pointer to the SYSTEM hive. This parameter
 * serves as the address of the registry hive
 * which is used for data copying into the
 * right location within the hive.
 *
 * @param[in] SystemLog
 * A pointer to the SYSTEM log. This parameter
 * serves as the address of the hive log where
 * contiguous blocks of data is to be copied from.
 *
 * @return
 * Returns TRUE if data copying has succeeded without
 * problems, FALSE otherwise.
 */
BOOLEAN
RegCopyDataFromSystemLog(
    _In_ PHBASE_BLOCK LogBaseBlock,
    _In_ ULONG HiveLength,
    _In_ PVOID SystemHive,
    _In_ PVOID SystemLog)
{
    RTL_BITMAP Bitmap;
    ULONG BitmapBuffer;
    ULONG BitmapSize;
    ULONG Offset;
    ULONG HeaderSize;
    ULONG BlockIndex;
    ULONG LastIndex;
    ULONG Block, BlockDest;
    ULONG DirtyVector;

    /*
     * Compute the file offset so that
     * we can read the dirty vector
     * and write up data to the hive after
     * the header.
     */
    HeaderSize = ROUND_UP(HV_LOG_HEADER_SIZE, LogBaseBlock->Cluster * HSECTOR_SIZE);
    Offset = LogBaseBlock->Cluster * HSECTOR_SIZE;
    Offset = ROUND_UP(Offset, HeaderSize);

    /* Now check the dirty vector signature */
    DirtyVector = (ULONG_PTR)SystemLog + Offset;
    Offset += sizeof(HV_LOG_DIRTY_SIGNATURE);
    if (DirtyVector != HV_LOG_DIRTY_SIGNATURE)
    {
        return FALSE;
    }

    /* Build the bitmap */
    BitmapSize = HiveLength / HSECTOR_SIZE;
    BitmapBuffer = (ULONG_PTR)SystemLog + Offset;
    RtlInitializeBitMap(&Bitmap, &BitmapBuffer, BitmapSize);

    /*
     * Now read each data block individually from the
     * log and write it back to the SYSTEM hive.
     */
    BlockIndex = 0;
    Offset += BitmapSize / 8;
    Offset = ROUND_UP(Offset, LogBaseBlock->Cluster * HSECTOR_SIZE);
    while (BlockIndex < BitmapSize)
    {
        /* Check if the block is clean or we're past the last block */
        LastIndex = BlockIndex;
        BlockIndex = RtlFindSetBits(&Bitmap, 1, BlockIndex);
        if (BlockIndex == ~HV_CLEAN_BLOCK || BlockIndex < LastIndex)
        {
            break;
        }

        /* Get the block and copy it to hive */
        Block = (ULONG_PTR)SystemLog + Offset;
        BlockDest = (ULONG_PTR)SystemHive + HBLOCK_SIZE + BlockIndex * HBLOCK_SIZE;
        RtlCopyMemory(&BlockDest, &Block, HiveLength);

        /* Advance to the next block to copy */
        Offset += HiveLength;
        BlockIndex++;
    }

    return TRUE;
}

/**
 * @brief
 * Recovers the SYSTEM hive from
 * corruption.
 *
 * @param[in] ChunkBase
 * An arbitrary pointer that points to the
 * registry chunk base of the SYSTEM hive
 * that is to be recovered in question.
 *
 * @param[in] SystemLog
 * The address of the SYSTEM hive log where
 * recovery data is to be copied from. This
 * parameter is assumed that it has a valid
 * base block header.
 *
 * @return
 * Returns TRUE if hive recovery has succeeded,
 * FALSE otherwise.
 *
 * @remarks
 * FreeLdr only performs SYSTEM hive recovery in
 * memory, the healed hive data is still not present
 * physically on the disk yet. The kernel is responsible
 * to sync all the dirty data from memory to the backing
 * storage in order to make the recovery data visible.
 */
BOOLEAN
RegRecoverSystemHive(
    _Inout_ PVOID ChunkBase,
    _In_ ULONG_PTR SystemLog)
{
    PHBASE_BLOCK SystemHiveBlock, SystemLogBlock;
    PVOID SystemFlatHive, SystemFlatLog;

    /* Capture the base blocks */
    SystemHiveBlock = GET_HBASE_BLOCK(ChunkBase);
    SystemLogBlock = GET_HBASE_BLOCK(SystemLog);

    /* Validate the base block */
    if (!HvpVerifyHiveHeader(SystemHiveBlock, HFILE_TYPE_PRIMARY))
    {
        /* Recover it */
        RtlCopyMemory(SystemHiveBlock,
                      SystemLogBlock,
                      SystemLogBlock->Cluster * HSECTOR_SIZE);
        SystemHiveBlock->Type = HFILE_TYPE_PRIMARY;
    }

    /* Copy data from the SYSTEM log */
    SystemFlatHive = GET_REG_ADDRESS(ChunkBase);
    SystemFlatLog = GET_REG_ADDRESS(SystemLog);
    if (!RegCopyDataFromSystemLog(SystemLogBlock,
                                  SystemHiveBlock->Length,
                                  SystemFlatHive,
                                  SystemFlatLog))
    {
        return FALSE;
    }

    /* Fix the secondary sequence and set up a new checksum */
    SystemHiveBlock->Sequence2 = SystemHiveBlock->Sequence1;
    SystemHiveBlock->CheckSum = HvpHiveHeaderChecksum(SystemHiveBlock);
    return TRUE;
}
#endif

/**
 * @brief
 * Imports the SYSTEM binary hive from
 * the registry base chunk that's been
 * provided by the loader block.
 *
 * @param[in] ChunkBase
 * A pointer to the registry base chunk
 * that serves for SYSTEM hive initialization.
 *
 * @param[in] ChunkSize
 * The size of the registry base chunk. This
 * parameter refers to the actual size of
 * the SYSTEM hive. This parameter is currently
 * unused.
 *
 * @return
 * Returns TRUE if hive importing and initialization
 * have succeeded, FALSE otherwise.
 */
BOOLEAN
RegImportBinaryHive(
    _In_ PVOID ChunkBase,
    _In_ ULONG ChunkSize)
{
    BOOLEAN Success;
/* FIXME: See the comment above */
#if !defined(_M_AMD64)
    ULONG_PTR SystemLog;
#endif
    PCM_KEY_NODE KeyNode;

    TRACE("RegImportBinaryHive(%p, 0x%lx)\n", ChunkBase, ChunkSize);

    /* Assume that we don't need boot recover, unless we have to */
    ((PHBASE_BLOCK)ChunkBase)->BootRecover = HBOOT_NO_BOOT_RECOVER;

    /* Allocate and initialize the hive */
    CmSystemHive = FrLdrTempAlloc(sizeof(CMHIVE), 'eviH');
    Success = RegInitializeSystemHive(CmSystemHive, ChunkBase);
    if (!Success)
/* FIXME: See the comment above */
#if defined(_M_AMD64)
    {
        ERR("Corrupted hive %p!\n", ChunkBase);
        FrLdrTempFree(CmSystemHive, 'eviH');
        return FALSE;
    }
#else
    {
        /* Free the buffer and retry again */
        FrLdrTempFree(CmSystemHive, 'eviH');
        CmSystemHive = NULL;

        /* Get the SYSTEM log address */
        if (!RegLoadSystemLogForRecovery(&SystemLog))
        {
            ERR("Couldn't get the SYSTEM log data\n");
            return FALSE;
        }

        /* Now recover the SYSTEM hive */
        if (!RegRecoverSystemHive(ChunkBase, SystemLog))
        {
            ERR("SYSTEM hive recovery failed\n");
            return FALSE;
        }

        /* Now retry initializing the hive again */
        CmSystemHive = FrLdrTempAlloc(sizeof(CMHIVE), 'eviH');
        Success = RegInitializeSystemHive(CmSystemHive, ChunkBase);
        if (!Success)
        {
            ERR("Corrupted hive (despite recovery) %p\n", ChunkBase);
            FrLdrTempFree(CmSystemHive, 'eviH');
            return FALSE;
        }

        /*
         * Acknowledge the kernel we recovered
         * the SYSTEM hive on our side.
         */
        ((PHBASE_BLOCK)ChunkBase)->BootRecover = HBOOT_BOOT_RECOVERED;
    }
#endif

    /* Save the root key node */
    SystemHive = GET_HHIVE(CmSystemHive);
    SystemRootCell = SystemHive->BaseBlock->RootCell;
    ASSERT(SystemRootCell != HCELL_NIL);

    /* Verify it is accessible */
    KeyNode = (PCM_KEY_NODE)HvGetCell(SystemHive, SystemRootCell);
    ASSERT(KeyNode);
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);
    HvReleaseCell(SystemHive, SystemRootCell);

    return TRUE;
}

BOOLEAN
RegInitCurrentControlSet(
    _In_ BOOLEAN LastKnownGood)
{
    UNICODE_STRING ControlSetName;
    HCELL_INDEX ControlCell;
    PCM_KEY_NODE KeyNode;
    BOOLEAN AutoSelect;

    TRACE("RegInitCurrentControlSet\n");

    /* Choose which control set to open and set it as the new "Current" */
    RtlInitUnicodeString(&ControlSetName,
                         LastKnownGood ? L"LastKnownGood"
                                       : L"Default");

    ControlCell = CmpFindControlSet(SystemHive,
                                    SystemRootCell,
                                    &ControlSetName,
                                    &AutoSelect);
    if (ControlCell == HCELL_NIL)
    {
        ERR("CmpFindControlSet('%wZ') failed\n", &ControlSetName);
        return FALSE;
    }

    CurrentControlSetKey = HCI_TO_HKEY(ControlCell);

    /* Verify it is accessible */
    KeyNode = (PCM_KEY_NODE)HvGetCell(SystemHive, ControlCell);
    ASSERT(KeyNode);
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);
    HvReleaseCell(SystemHive, ControlCell);

    return TRUE;
}

static
BOOLEAN
GetNextPathElement(
    _Out_ PUNICODE_STRING NextElement,
    _Inout_ PUNICODE_STRING RemainingPath)
{
    /* Check if there are any characters left */
    if (RemainingPath->Length < sizeof(WCHAR))
    {
        /* Nothing left, bail out early */
        return FALSE;
    }

    /* The next path elements starts with the remaining path */
    NextElement->Buffer = RemainingPath->Buffer;

    /* Loop until the path element ends */
    while ((RemainingPath->Length >= sizeof(WCHAR)) &&
           (RemainingPath->Buffer[0] != '\\'))
    {
        /* Skip this character */
        RemainingPath->Buffer++;
        RemainingPath->Length -= sizeof(WCHAR);
    }

    NextElement->Length = (USHORT)(RemainingPath->Buffer - NextElement->Buffer) * sizeof(WCHAR);
    NextElement->MaximumLength = NextElement->Length;

    /* Check if the path element ended with a path separator */
    if (RemainingPath->Length >= sizeof(WCHAR))
    {
        /* Skip the path separator */
        ASSERT(RemainingPath->Buffer[0] == '\\');
        RemainingPath->Buffer++;
        RemainingPath->Length -= sizeof(WCHAR);
    }

    /* Return whether we got any characters */
    return TRUE;
}

#if 0
LONG
RegEnumKey(
    _In_ HKEY Key,
    _In_ ULONG Index,
    _Out_ PWCHAR Name,
    _Inout_ PULONG NameSize,
    _Out_opt_ PHKEY SubKey)
{
    PHHIVE Hive = GET_HHIVE_FROM_HKEY(Key);
    PCM_KEY_NODE KeyNode, SubKeyNode;
    HCELL_INDEX CellIndex;
    USHORT NameLength;

    TRACE("RegEnumKey(%p, %lu, %p, %p->%u)\n",
          Key, Index, Name, NameSize, NameSize ? *NameSize : 0);

    /* Get the key node */
    KeyNode = GET_CM_KEY_NODE(Hive, Key);
    ASSERT(KeyNode);
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    CellIndex = CmpFindSubKeyByNumber(Hive, KeyNode, Index);
    if (CellIndex == HCELL_NIL)
    {
        TRACE("RegEnumKey index out of bounds (%d) in key (%.*s)\n",
              Index, KeyNode->NameLength, KeyNode->Name);
        HvReleaseCell(Hive, HKEY_TO_HCI(Key));
        return ERROR_NO_MORE_ITEMS;
    }
    HvReleaseCell(Hive, HKEY_TO_HCI(Key));

    /* Get the value cell */
    SubKeyNode = (PCM_KEY_NODE)HvGetCell(Hive, CellIndex);
    ASSERT(SubKeyNode != NULL);
    ASSERT(SubKeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    if (SubKeyNode->Flags & KEY_COMP_NAME)
    {
        NameLength = CmpCompressedNameSize(SubKeyNode->Name, SubKeyNode->NameLength);

        /* Compressed name */
        CmpCopyCompressedName(Name,
                              *NameSize,
                              SubKeyNode->Name,
                              SubKeyNode->NameLength);
    }
    else
    {
        NameLength = SubKeyNode->NameLength;

        /* Normal name */
        RtlCopyMemory(Name, SubKeyNode->Name,
                      min(*NameSize, SubKeyNode->NameLength));
    }

    if (*NameSize >= NameLength + sizeof(WCHAR))
    {
        Name[NameLength / sizeof(WCHAR)] = UNICODE_NULL;
    }

    *NameSize = NameLength + sizeof(WCHAR);

    HvReleaseCell(Hive, CellIndex);

    if (SubKey != NULL)
        *SubKey = HCI_TO_HKEY(CellIndex);

    TRACE("RegEnumKey done -> %u, '%.*S'\n", *NameSize, *NameSize, Name);
    return ERROR_SUCCESS;
}
#endif

LONG
RegOpenKey(
    _In_ HKEY ParentKey,
    _In_z_ PCWSTR KeyName,
    _Out_ PHKEY Key)
{
    UNICODE_STRING RemainingPath, SubKeyName;
    UNICODE_STRING CurrentControlSet = RTL_CONSTANT_STRING(L"CurrentControlSet");
    PHHIVE Hive = (ParentKey ? GET_HHIVE_FROM_HKEY(ParentKey) : GET_HHIVE(CmSystemHive));
    PCM_KEY_NODE KeyNode;
    HCELL_INDEX CellIndex;

    TRACE("RegOpenKey(%p, '%S', %p)\n", ParentKey, KeyName, Key);

    /* Initialize the remaining path name */
    RtlInitUnicodeString(&RemainingPath, KeyName);

    /* Check if we have a parent key */
    if (ParentKey == NULL)
    {
        UNICODE_STRING SubKeyName1, SubKeyName2, SubKeyName3;
        UNICODE_STRING RegistryPath = RTL_CONSTANT_STRING(L"Registry");
        UNICODE_STRING MachinePath = RTL_CONSTANT_STRING(L"MACHINE");
        UNICODE_STRING SystemPath = RTL_CONSTANT_STRING(L"SYSTEM");

        TRACE("RegOpenKey: absolute path\n");

        if ((RemainingPath.Length < sizeof(WCHAR)) ||
            RemainingPath.Buffer[0] != '\\')
        {
            /* The key path is not absolute */
            ERR("RegOpenKey: invalid path '%S' (%wZ)\n", KeyName, &RemainingPath);
            return ERROR_PATH_NOT_FOUND;
        }

        /* Skip initial path separator */
        RemainingPath.Buffer++;
        RemainingPath.Length -= sizeof(WCHAR);

        /* Get the first 3 path elements */
        GetNextPathElement(&SubKeyName1, &RemainingPath);
        GetNextPathElement(&SubKeyName2, &RemainingPath);
        GetNextPathElement(&SubKeyName3, &RemainingPath);
        TRACE("RegOpenKey: %wZ / %wZ / %wZ\n", &SubKeyName1, &SubKeyName2, &SubKeyName3);

        /* Check if we have the correct path */
        if (!RtlEqualUnicodeString(&SubKeyName1, &RegistryPath, TRUE) ||
            !RtlEqualUnicodeString(&SubKeyName2, &MachinePath, TRUE) ||
            !RtlEqualUnicodeString(&SubKeyName3, &SystemPath, TRUE))
        {
            /* The key path is not inside HKLM\Machine\System */
            ERR("RegOpenKey: invalid path '%S' (%wZ)\n", KeyName, &RemainingPath);
            return ERROR_PATH_NOT_FOUND;
        }

        /* Use the root key */
        CellIndex = SystemRootCell;
    }
    else
    {
        /* Use the parent key */
        CellIndex = HKEY_TO_HCI(ParentKey);
    }

    /* Check if this is the root key */
    if (CellIndex == SystemRootCell)
    {
        UNICODE_STRING TempPath = RemainingPath;

        /* Get the first path element */
        GetNextPathElement(&SubKeyName, &TempPath);

        /* Check if this is CurrentControlSet */
        if (RtlEqualUnicodeString(&SubKeyName, &CurrentControlSet, TRUE))
        {
            /* Use the CurrentControlSetKey and update the remaining path */
            CellIndex = HKEY_TO_HCI(CurrentControlSetKey);
            RemainingPath = TempPath;
        }
    }

    /* Get the key node */
    KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, CellIndex);
    ASSERT(KeyNode);
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    TRACE("RegOpenKey: RemainingPath '%wZ'\n", &RemainingPath);

    /* Loop while there are path elements */
    while (GetNextPathElement(&SubKeyName, &RemainingPath))
    {
        HCELL_INDEX NextCellIndex;

        TRACE("RegOpenKey: next element '%wZ'\n", &SubKeyName);

        /* Get the next sub key */
        NextCellIndex = CmpFindSubKeyByName(Hive, KeyNode, &SubKeyName);
        HvReleaseCell(Hive, CellIndex);
        CellIndex = NextCellIndex;
        if (CellIndex == HCELL_NIL)
        {
            WARN("Did not find sub key '%wZ' (full: %S)\n", &SubKeyName, KeyName);
            return ERROR_PATH_NOT_FOUND;
        }

        /* Get the found key */
        KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, CellIndex);
        ASSERT(KeyNode);
        ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);
    }

    HvReleaseCell(Hive, CellIndex);
    *Key = HCI_TO_HKEY(CellIndex);

    return ERROR_SUCCESS;
}

static
VOID
RepGetValueData(
    _In_ PHHIVE Hive,
    _In_ PCM_KEY_VALUE ValueCell,
    _Out_opt_ PULONG Type,
    _Out_opt_ PUCHAR Data,
    _Inout_opt_ PULONG DataSize)
{
    ULONG DataLength;
    PVOID DataCell;

    /* Does the caller want the type? */
    if (Type != NULL)
        *Type = ValueCell->Type;

    /* Does the caller provide DataSize? */
    if (DataSize != NULL)
    {
        // NOTE: CmpValueToData doesn't support big data (the function will
        // bugcheck if so), FreeLdr is not supposed to read such data.
        // If big data is needed, use instead CmpGetValueData.
        // CmpGetValueData(Hive, ValueCell, DataSize, &DataCell, ...);
        DataCell = CmpValueToData(Hive, ValueCell, &DataLength);

        /* Does the caller want the data? */
        if ((Data != NULL) && (*DataSize != 0))
        {
            RtlCopyMemory(Data,
                          DataCell,
                          min(*DataSize, DataLength));
        }

        /* Return the actual data length */
        *DataSize = DataLength;
    }
}

LONG
RegQueryValue(
    _In_ HKEY Key,
    _In_z_ PCWSTR ValueName,
    _Out_opt_ PULONG Type,
    _Out_opt_ PUCHAR Data,
    _Inout_opt_ PULONG DataSize)
{
    PHHIVE Hive = GET_HHIVE_FROM_HKEY(Key);
    PCM_KEY_NODE KeyNode;
    PCM_KEY_VALUE ValueCell;
    HCELL_INDEX CellIndex;
    UNICODE_STRING ValueNameString;

    TRACE("RegQueryValue(%p, '%S', %p, %p, %p)\n",
          Key, ValueName, Type, Data, DataSize);

    /* Get the key node */
    KeyNode = GET_CM_KEY_NODE(Hive, Key);
    ASSERT(KeyNode);
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    /* Initialize value name string */
    RtlInitUnicodeString(&ValueNameString, ValueName);
    CellIndex = CmpFindValueByName(Hive, KeyNode, &ValueNameString);
    if (CellIndex == HCELL_NIL)
    {
        TRACE("RegQueryValue value not found in key (%.*s)\n",
              KeyNode->NameLength, KeyNode->Name);
        HvReleaseCell(Hive, HKEY_TO_HCI(Key));
        return ERROR_FILE_NOT_FOUND;
    }
    HvReleaseCell(Hive, HKEY_TO_HCI(Key));

    /* Get the value cell */
    ValueCell = (PCM_KEY_VALUE)HvGetCell(Hive, CellIndex);
    ASSERT(ValueCell != NULL);

    RepGetValueData(Hive, ValueCell, Type, Data, DataSize);

    HvReleaseCell(Hive, CellIndex);

    return ERROR_SUCCESS;
}

/*
 * NOTE: This function is currently unused in FreeLdr; however it is kept here
 * as an implementation reference of RegEnumValue using CMLIB that may be used
 * elsewhere in ReactOS.
 */
#if 0
LONG
RegEnumValue(
    _In_ HKEY Key,
    _In_ ULONG Index,
    _Out_ PWCHAR ValueName,
    _Inout_ PULONG NameSize,
    _Out_opt_ PULONG Type,
    _Out_opt_ PUCHAR Data,
    _Inout_opt_ PULONG DataSize)
{
    PHHIVE Hive = GET_HHIVE_FROM_HKEY(Key);
    PCM_KEY_NODE KeyNode;
    PCELL_DATA ValueListCell;
    PCM_KEY_VALUE ValueCell;
    USHORT NameLength;

    TRACE("RegEnumValue(%p, %lu, %S, %p, %p, %p, %p (%lu))\n",
          Key, Index, ValueName, NameSize, Type, Data, DataSize, *DataSize);

    /* Get the key node */
    KeyNode = GET_CM_KEY_NODE(Hive, Key);
    ASSERT(KeyNode);
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    /* Check if the index is valid */
    if ((KeyNode->ValueList.Count == 0) ||
        (KeyNode->ValueList.List == HCELL_NIL) ||
        (Index >= KeyNode->ValueList.Count))
    {
        ERR("RegEnumValue: index invalid\n");
        HvReleaseCell(Hive, HKEY_TO_HCI(Key));
        return ERROR_NO_MORE_ITEMS;
    }

    ValueListCell = (PCELL_DATA)HvGetCell(Hive, KeyNode->ValueList.List);
    ASSERT(ValueListCell != NULL);

    /* Get the value cell */
    ValueCell = (PCM_KEY_VALUE)HvGetCell(Hive, ValueListCell->KeyList[Index]);
    ASSERT(ValueCell != NULL);
    ASSERT(ValueCell->Signature == CM_KEY_VALUE_SIGNATURE);

    if (ValueCell->Flags & VALUE_COMP_NAME)
    {
        NameLength = CmpCompressedNameSize(ValueCell->Name, ValueCell->NameLength);

        /* Compressed name */
        CmpCopyCompressedName(ValueName,
                              *NameSize,
                              ValueCell->Name,
                              ValueCell->NameLength);
    }
    else
    {
        NameLength = ValueCell->NameLength;

        /* Normal name */
        RtlCopyMemory(ValueName, ValueCell->Name,
                      min(*NameSize, ValueCell->NameLength));
    }

    if (*NameSize >= NameLength + sizeof(WCHAR))
    {
        ValueName[NameLength / sizeof(WCHAR)] = UNICODE_NULL;
    }

    *NameSize = NameLength + sizeof(WCHAR);

    RepGetValueData(Hive, ValueCell, Type, Data, DataSize);

    HvReleaseCell(Hive, ValueListCell->KeyList[Index]);
    HvReleaseCell(Hive, KeyNode->ValueList.List);
    HvReleaseCell(Hive, HKEY_TO_HCI(Key));

    TRACE("RegEnumValue done -> %u, '%.*S'\n", *NameSize, *NameSize, ValueName);
    return ERROR_SUCCESS;
}
#endif

/* EOF */
