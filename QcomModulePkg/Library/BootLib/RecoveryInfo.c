/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "RecoveryInfo.h"
#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Protocol/EFIRecoveryInfo.h>
#include <Library/UefiBootServicesTableLib.h>
#include "PartitionTableUpdate.h"
#include <VerifiedBoot.h>


STATIC INT64 HasRecoveryInfo = -1;

BOOLEAN IsRecoveryInfo ()
{
  EFI_STATUS Status = EFI_SUCCESS ;
  EFI_RECOVERYINFO_PROTOCOL *pRecoveryInfoProtocol = NULL;
  RECOVERY_STATUS_STATE RecoveryState;

  if (HasRecoveryInfo == -1 ) {
    DEBUG (( EFI_D_VERBOSE,  "Initializing HasRecoveryInfo\n"));
    Status = gBS->LocateProtocol (& gEfiRecoveryInfoProtocolGuid, NULL,
                                  (VOID **) & pRecoveryInfoProtocol);
    if (Status != EFI_SUCCESS) {
      HasRecoveryInfo = 0;
      return FALSE;
    }

    Status = pRecoveryInfoProtocol -> GetRecoveryState (pRecoveryInfoProtocol,
                                                        &RecoveryState);
    if (Status != EFI_SUCCESS) {
      HasRecoveryInfo = 1;
    } else if ((RecoveryState != RECOVERY_INFO_PARTITION_FAIL) &&
         (Status == EFI_SUCCESS)) {
      DEBUG (( EFI_D_INFO,  "RecoveryInfo is enabled\n"));
      HasRecoveryInfo = 1;
    }
  }

  return (HasRecoveryInfo == 1);
}
