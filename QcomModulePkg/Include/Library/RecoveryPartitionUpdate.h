/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __RECOVERY_PARTITION_UPDATE_H__
#define __RECOVERY_PARTITION_UPDATE_H__

#include <Uefi.h>
#include <Library/PartitionTableUpdate.h>

#define RECOVERY_INFO_MISC_OFFSET (2 * 1024)
#define RESERVED_FIELD_SIZE       16
#define MISC_GPT_ENTRY_MAGIC      0x47505445UL    /* 'GPTE' */

typedef struct {
  UINT32 UpdateFlag;
  UINT32 EntryMagic;
  EFI_PARTITION_ENTRY PartitionEntries[MAX_SLOTS];
  UINT32 Reserved;
} RecoveryInfoMiscData;

EFI_STATUS
WriteRecoveryInfoMisc (RecoveryInfoMiscData *RecInfo);

EFI_STATUS
ReadRecoveryInfoMisc (RecoveryInfoMiscData *RecInfo);
#endif
