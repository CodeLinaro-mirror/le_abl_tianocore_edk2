/** @file DeviceInfoVirt.c
*
*  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
*
*  SPDX-License-Identifier: BSD-3-Clause-Clear
*
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/Board.h>
#include <Library/DebugLib.h>
#include <Library/DeviceInfo.h>
#include <Library/PcdLib.h>

#include "DeviceInfoVirt.h"

#define DEVINF_MAGIC_STR   "_DEVINF_"
#define DEVINF_MAGIC_SIZE  8
#define SERIAL_NUM_SIZE    64

#pragma pack (1)
typedef struct DeviceInfoVirt {
  CHAR8   Magic[DEVINF_MAGIC_SIZE];
  UINT32  Reserved;
  CHAR8   SerialNumber[SERIAL_NUM_SIZE];
  UINT32  Crc;
} DeviceInfoVirtT;
#pragma pack ()

/** Store device info for virtual bootloadr.

  @retval  EFI_SUCCES   On success
  @retval  Other value  On serial number access problems

--*/
EFI_STATUS
StoreDeviceInfoVirt (
  VOID
  )
{
  EFI_STATUS       Status;
  DeviceInfoVirtT  DevInf;
  UINT64           DevInfAddress;

  DevInfAddress = (UINT64)PcdGet64 (PcdDeviceInfoVirtBaseAddress);

  ZeroMem (&DevInf, sizeof (DevInf));

  AsciiStrnCpy (DevInf.Magic, DEVINF_MAGIC_STR, DEVINF_MAGIC_SIZE);

  Status = BoardSerialNum (DevInf.SerialNumber, sizeof (DevInf.SerialNumber));

  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Error: failed to read board serial num: %d\n", Status));
    ZeroMem (DevInf.SerialNumber, sizeof (DevInf.SerialNumber));
  }

  Status = gBS->CalculateCrc32 (&DevInf, sizeof (DevInf) - sizeof (DevInf.Crc), &DevInf.Crc);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Error: failed to calculate checksum: %d\n", Status));
    return Status;
  }

  CopyMem ((DeviceInfoVirtT *)DevInfAddress, &DevInf, sizeof (DevInf));

  return Status;
}
