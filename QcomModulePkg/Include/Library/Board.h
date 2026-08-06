/* Copyright (c) 2015-2018, 2020-2021, The Linux Foundation. All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 *  with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/Debug.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/EFIChipInfo.h>
#include <Protocol/EFIPlatformInfo.h>
#include <Protocol/EFIPmicVersion.h>
#include <Protocol/EFIRamPartition.h>
#include <Protocol/EFISoftSKU.h>

#define HANDLE_MAX_INFO_LIST 128
#define CHIP_BASE_BAND_LEN 4
#define CHIP_BASE_BAND_MSM "msm"
#define CHIP_BASE_BAND_APQ "apq"
#define CHIP_BASE_BAND_MDM "mdm"

#define BIT(x) (1ULL << x)

extern RamPartitionEntry *RamPartitionEntries;

typedef enum {
  EMMC = 0,
  UFS = 1,
  NAND = 2,
  UNKNOWN,
} MemCardType;

#define BOOT_DEVICE_SHIFT      16
#define DDR_SHIFT              8

#define MB             (1024 * 1024UL)
#define DDR_1024MB      (1024 * MB)    /* 1 GB */
#define DDR_2048MB      (2048 * MB)    /* 2 GB */
#define DDR_4096MB      (4096 * MB)    /* 4 GB */
#define DDR_8192MB      (8192 * MB)    /* 8 GB */
#define DDR_12288MB     (12288 * MB)   /* 12 GB */
#define DDR_16384MB     (16384 * MB)   /* 16 GB */
#define DDR_18432MB     (18432 * MB)   /* 18 GB */
#define DDR_24576MB     (24576 * MB)   /* 24 GB */
#define DDR_32768MB     (32768 * MB)   /* 32 GB */
#define DDR_36864MB     (36864 * MB)   /* 36 GB */
#define DDR_49152MB     (49152 * MB)   /* 48 GB */
#define DDR_65536MB     (65536 * MB)   /* 64 GB */
#define DDR_98304MB     (98304 * MB)   /* 96 GB */
#define DDR_131072MB    (131072 * MB)  /* 128 GB */

typedef enum {
  DDRTYPE_1024MB = 1,   /* 1 GB */
  DDRTYPE_2048MB,       /* 2 GB */
  DDRTYPE_4096MB,       /* 4 GB */
  DDRTYPE_8192MB,       /* 8 GB */
  DDRTYPE_12288MB,      /* 12 GB */
  DDRTYPE_16384MB,      /* 16 GB */
  DDRTYPE_18432MB,      /* 18 GB */
  DDRTYPE_24576MB,      /* 24 GB */
  DDRTYPE_32768MB,      /* 32 GB */
  DDRTYPE_36864MB,      /* 36 GB */
  DDRTYPE_49152MB,      /* 48 GB */
  DDRTYPE_65536MB,      /* 64 GB */
  DDRTYPE_98304MB,      /* 96 GB */
  DDRTYPE_131072MB,     /* 128 GB */
} DdrType;

/*
 * DDR type is encoded in HlosSubType across two non-contiguous bit fields:
 *   bits [10:8]  = lower 3 bits of DDR type
 *   bits [13:12] = upper 2 bits of DDR type
 *
 * This allows up to 32 DDR type values. Types 1-7 use only bits [10:8];
 * types 8+ use bits [13:12] as well.
 *
 * Packed values are monotonically increasing, so <= comparisons remain valid.
 */
#define DDR_TYPE_TO_HLOS(type) \
    ((((type) >> 3) << 12) | (((type) & 0x7) << 8))

struct BoardInfo {
  EFI_PLATFORMINFO_PLATFORM_INFO_TYPE PlatformInfo;
  UINT32 RawChipId;
  CHAR8 ChipBaseBand[EFICHIPINFO_MAX_ID_LENGTH];
  EFIChipInfoVersionType ChipVersion;
  EFIChipInfoFoundryIdType FoundryId;
  UINT32 PackageId;
  UINT32 HlosSubType;
  UINT32 SoftSKUId;
};

/*
 qcom,sku-id=<0xABCDEFGH>

 Each Nibble holds a configuration value

 Currently H will track Sub SKU ID and F will track Software Config.
*/

#define BAD_SOFTSKU_ID  0xBADDEBAD
#define SOFTSKU_ID_SUBSKU_SHIFT  0
#define SOFTSKU_ID_SUBSKU_MASK  (0xf << SOFTSKU_ID_SUBSKU_SHIFT)
#define SOFTSKU_ID_SWCONFIG_SHFIT  8
#define SOFTSKU_ID_SWCONFIG_MASK  (0xf << SOFTSKU_ID_SWCONFIG_SHFIT)

extern BOOLEAN IsSoftSkuProtocolAvailable;

EFI_STATUS
BaseMem (UINT64 *BaseMemory);

UINT32
BoardPmicModel (UINT32 PmicDeviceIndex);

UINT32
BoardPmicTarget (UINT32 PmicDeviceIndex);

EFI_STATUS BoardInit (VOID);

EFI_STATUS
BoardSerialNum (CHAR8 *StrSerialNum, UINT32 Len);
UINT32 BoardPlatformRawChipId (VOID);
CHAR8 *BoardPlatformChipBaseBand (VOID);
EFIChipInfoVersionType BoardPlatformChipVersion (VOID);
EFIChipInfoFoundryIdType BoardPlatformFoundryId (VOID);
UINT32 BoardPlatformPackageId (VOID);
EFI_PLATFORMINFO_PLATFORM_TYPE BoardPlatformType (VOID);
UINT32 BoardPlatformVersion (VOID);
UINT32 BoardPlatformSubType (VOID);
UINT32 BoardOEMVariantId (VOID);
UINT32 BoardTargetId (VOID);
VOID
GetRootDeviceType (CHAR8 *StrDeviceType, UINT32 Len);
MemCardType
CheckRootDeviceType (VOID);
VOID
BoardHwPlatformName (CHAR8 *StrHwPlatform, UINT32 Len);
EFI_STATUS
UfsGetSetBootLun (UINT32 *UfsBootlun, BOOLEAN IsGet);
BOOLEAN BoardPlatformFusion (VOID);
UINT32 BoardPlatformRawChipId (VOID);
EFI_STATUS ReadRamPartitions (RamPartitionEntry **RamPartitions,
                  UINT32 *NumPartitions);
EFI_STATUS GetGranuleSize (UINT32 *MinPasrGranuleSize);
VOID GetPageSize (UINT32 *PageSize);
EFI_STATUS GetDdrSize (UINT64 *DdrSize);
EFI_STATUS BoardDdrType (UINT32 *Type);
UINT32 BoardPlatformHlosSubType (VOID);
VOID BoardSoftSKU (UINT32 *SKUId);
UINT32 BoardSKUId (VOID);

#ifdef ENABLE_DC_TARGET
EFI_STATUS GetPlatformTypeData(UINT32 *PlatformType);
#endif

EFI_STATUS
GetSoftSKUFeatureInfo (SOFT_SKU_SWCFG_FEATURE_ID FeatureID, UINT32 *FeatureVal);
#endif
