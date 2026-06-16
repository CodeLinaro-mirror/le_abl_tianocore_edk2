/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <Uefi.h>
#include <Library/BootLinux.h>
#include <Library/DebugLib.h>
#include <Library/LinuxLoaderLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/RecoveryPartitionUpdate.h>
#include <Library/UefiBootServicesTableLib.h>
#include <FastbootLib/FastbootCmds.h>

STATIC CHAR16 RecoveryInfoPartition[] = L"recoveryinfo";

EFI_STATUS
WriteRecoveryInfoMisc (RecoveryInfoMiscData *RecInfo)
{
  EFI_STATUS Status;
  EFI_BLOCK_IO_PROTOCOL *BlockIo = NULL;
  EFI_HANDLE *Handle = NULL;
  UINT32 BlockSize;
  UINT64 Offset;
  UINT64 WriteSize;
  VOID *Buffer = NULL;

  if (RecInfo == NULL) {
    DEBUG ((EFI_D_ERROR, "WriteRecoveryInfoMisc: Invalid input\n"));
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((EFI_D_VERBOSE, "WriteRecoveryInfoMisc: Looking up partition '%s'\n",
          RecoveryInfoPartition));
  Status = PartitionGetInfo (RecoveryInfoPartition, &BlockIo, &Handle);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR,
            "WriteRecoveryInfoMisc: PartitionGetInfo failed: %r\n", Status));
    return Status;
  }
  if (!BlockIo) {
    DEBUG ((EFI_D_ERROR, "WriteRecoveryInfoMisc: BlockIo is NULL\n"));
    return EFI_VOLUME_CORRUPTED;
  }
  if (!Handle) {
    DEBUG ((EFI_D_ERROR, "WriteRecoveryInfoMisc: Handle is NULL\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  BlockSize = BlockIo->Media->BlockSize;
  DEBUG ((EFI_D_VERBOSE, "WriteRecoveryInfoMisc: BlockSize=%u, Offset=%u\n",
          BlockSize, RECOVERY_INFO_MISC_OFFSET));
  if (BlockSize == 0) {
    DEBUG ((EFI_D_ERROR, "WriteRecoveryInfoMisc: Invalid BlockSize\n"));
    return EFI_INVALID_PARAMETER;
  }
  if ((RECOVERY_INFO_MISC_OFFSET + sizeof (RecoveryInfoMiscData)) > BlockSize) {
    DEBUG ((EFI_D_ERROR,
            "WriteRecoveryInfoMisc: BlockSize %u too small for offset %u "
            "+ struct size %u\n",
            BlockSize, RECOVERY_INFO_MISC_OFFSET,
            sizeof (RecoveryInfoMiscData)));
    return EFI_BAD_BUFFER_SIZE;
  }

  /* Since RECOVERY_INFO_MISC_OFFSET (2KB) is not block-aligned (4KB),
   * we need to do read-modify-write of the entire block */
  Offset = 0;  /* Always read/write the first block */
  WriteSize = BlockSize;

  Buffer = AllocateZeroPool (WriteSize);
  if (Buffer == NULL) {
    DEBUG ((EFI_D_ERROR,
            "WriteRecoveryInfoMisc: Failed to allocate buffer\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  /* Read the entire block first */
  Status = BlockIo->ReadBlocks (BlockIo, BlockIo->Media->MediaId,
                                Offset, WriteSize, Buffer);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR,
            "WriteRecoveryInfoMisc: ReadBlocks failed: %r\n", Status));
    FreePool (Buffer);
    return Status;
  }

  /* Modify only the 2KB offset portion with our data */
  gBS->CopyMem ((UINT8 *)Buffer + RECOVERY_INFO_MISC_OFFSET,
                RecInfo, sizeof (RecoveryInfoMiscData));

  DEBUG ((EFI_D_VERBOSE,
          "WriteRecoveryInfoMisc: Writing block at LBA %lu, size=%lu bytes\n",
          Offset, WriteSize));

  Status = WriteBlockToPartition (BlockIo, Handle, Offset, WriteSize, Buffer);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR,
            "WriteRecoveryInfoMisc: WriteBlockToPartition failed: %r\n", Status));
  } else {
    DEBUG ((EFI_D_VERBOSE, "WriteRecoveryInfoMisc: Write succeeded\n"));
  }

  FreePool (Buffer);
  return Status;
}

EFI_STATUS
ReadRecoveryInfoMisc (RecoveryInfoMiscData *RecInfo)
{
  EFI_STATUS Status;
  EFI_BLOCK_IO_PROTOCOL *BlockIo = NULL;
  EFI_HANDLE *Handle = NULL;
  UINT32 BlockSize;
  UINT64 Offset;
  UINT64 ReadSize;
  VOID *Buffer = NULL;
  EFI_PARTITION_ENTRY *Entry;
  UINT32 i;

  if (RecInfo == NULL) {
    DEBUG ((EFI_D_ERROR, "ReadRecoveryInfoMisc: Invalid input\n"));
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((EFI_D_VERBOSE, "ReadRecoveryInfoMisc: Looking up partition '%s'\n",
          RecoveryInfoPartition));
  Status = PartitionGetInfo (RecoveryInfoPartition, &BlockIo, &Handle);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR,
            "ReadRecoveryInfoMisc: PartitionGetInfo failed: %r\n", Status));
    return Status;
  }
  if (!BlockIo) {
    DEBUG ((EFI_D_ERROR, "ReadRecoveryInfoMisc: BlockIo is NULL\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  BlockSize = BlockIo->Media->BlockSize;
  if (BlockSize == 0) {
    DEBUG ((EFI_D_ERROR, "ReadRecoveryInfoMisc: Invalid BlockSize\n"));
    return EFI_INVALID_PARAMETER;
  }

  /* Since RECOVERY_INFO_MISC_OFFSET (2KB) is not block-aligned (4KB),
   * read the entire first block and extract data from offset 2KB */
  Offset = 0;  /* Always read the first block */
  ReadSize = BlockSize;

  Buffer = AllocateZeroPool (ReadSize);
  if (Buffer == NULL) {
    DEBUG ((EFI_D_ERROR,
            "ReadRecoveryInfoMisc: Failed to allocate buffer\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = BlockIo->ReadBlocks (BlockIo, BlockIo->Media->MediaId,
                                Offset, ReadSize, Buffer);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR,
            "ReadRecoveryInfoMisc: ReadBlocks failed: %r\n", Status));
    FreePool (Buffer);
    return Status;
  }

  /* Extract data from the 2KB offset within the block */
  gBS->CopyMem (RecInfo, (UINT8 *)Buffer + RECOVERY_INFO_MISC_OFFSET,
                sizeof (RecoveryInfoMiscData));

  FreePool (Buffer);

  DEBUG ((EFI_D_VERBOSE, "RecoveryInfoMiscData contents (MAX_SLOTS=%d):\n",
          MAX_SLOTS));
  DEBUG ((EFI_D_VERBOSE, "  UpdateFlag  : 0x%x\n", RecInfo->UpdateFlag));
  DEBUG ((EFI_D_VERBOSE, "  EntryMagic  : 0x%x\n", RecInfo->EntryMagic));
  for (i = 0; i < MAX_SLOTS; i++) {
    Entry = &RecInfo->PartitionEntries[i];
    DEBUG ((EFI_D_VERBOSE, "  Slot[%d]:\n", i));
    DEBUG ((EFI_D_VERBOSE, "    PartitionName  : %s\n", Entry->PartitionName));
    DEBUG ((EFI_D_VERBOSE, "    StartingLBA    : 0x%lx\n", Entry->StartingLBA));
    DEBUG ((EFI_D_VERBOSE, "    EndingLBA      : 0x%lx\n", Entry->EndingLBA));
    DEBUG ((EFI_D_VERBOSE, "    Attributes     : 0x%lx\n", Entry->Attributes));
    DEBUG ((EFI_D_VERBOSE,
            "    PartitionTypeGUID: %g\n", &Entry->PartitionTypeGUID));
    DEBUG ((EFI_D_VERBOSE,
            "    UniquePartGUID : %g\n", &Entry->UniquePartitionGUID));
  }
  DEBUG ((EFI_D_VERBOSE, "  Reserved    : 0x%x\n", RecInfo->Reserved));
  DEBUG ((EFI_D_VERBOSE, "\n"));

  return EFI_SUCCESS;
}
