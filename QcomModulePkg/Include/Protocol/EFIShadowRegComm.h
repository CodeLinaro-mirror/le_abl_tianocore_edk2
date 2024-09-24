/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __EFI_SHADOW_REG_COMM_H__
#define __EFI_SHADOW_REG_COMM_H__

typedef struct _EFI_SHADOW_REG_COMM_PROTOCOL EfiShadowRegCommProtocol;

/*
  External reference to the Shadow Register Communication Protocol GUID.
 */
extern EFI_GUID gEfiShadowRegCommProtocolGuid;

typedef EFI_STATUS (EFIAPI *EFI_SHADOW_REG_COMM_INIT)(IN void);

typedef EFI_STATUS (EFIAPI *EFI_SHADOW_REG_READ)(
            IN UINT32 Addr,
            IN UINT32 *Value
);

typedef EFI_STATUS (EFIAPI *EFI_SHADOW_REG_WRITE)(
            IN UINT32 Addr,
            IN UINT32 Value
);

typedef EFI_STATUS (EFIAPI *EFI_SHADOW_REG_GET_ADDR)(
            IN UINT32 Addr,
            IN OUT UINT64 *Value
);

/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/

typedef struct _EFI_SHADOW_REG_COMM_PROTOCOL {
  UINT64                   Revision;
  EFI_SHADOW_REG_COMM_INIT ShadowRegCommInit;
  EFI_SHADOW_REG_READ      ShadowRegRead;
  EFI_SHADOW_REG_WRITE     ShadowRegWrite;
  EFI_SHADOW_REG_GET_ADDR  ShadowRegGetAddr;
} EfiShadowRegCommProtocol;

#endif /* __EFI_SHADOW_REG_COMM_H__ */
