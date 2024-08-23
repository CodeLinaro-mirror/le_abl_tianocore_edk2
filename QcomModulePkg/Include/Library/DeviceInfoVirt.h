/** @file DeviceInfoVirt.h
*
*  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
*
*  SPDX-License-Identifier: BSD-3-Clause-Clear
*
**/

#ifndef _DEVINFOVIRT_H_
#define _DEVINFOVIRT_H_

#include <Uefi.h>

/** Store device info for virtual bootloadr.

  @retval  EFI_SUCCES   On success
  @retval  Other value  On serial number access problems

--*/
EFI_STATUS
StoreDeviceInfoVirt (
  VOID
  );

#endif
