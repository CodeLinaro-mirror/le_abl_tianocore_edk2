/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "AvbPopulateBccParams.h"

/* SetDummyBccParams will set the BccParams all 0s.
 */
STATIC void
SetDummyBccParams (BccParams_t *bcc_params)
{
    avb_memset ((void *)bcc_params, 0, sizeof (*bcc_params));
    DEBUG ((EFI_D_INFO, "VB: Setting Dummy DICE params\n"));
    /* AVF debug policy requires mode to be in debug */
    if (IsUnlocked ()) {
         bcc_params->Mode = kDiceModeDebug;
    }
}

/* PopulateAuthorityHash will Populate the Authority Hash for BCC Params.
 * The authority hash will be hash of public key of vbmeta and
 * vbmeta_system
 */

STATIC EFI_STATUS
PopulateAuthorityHash (AvbSlotVerifyData *SlotData, BccParams_t *bcc_params)
{
    EFI_STATUS Status = EFI_SUCCESS;
    const uint8_t* PkData = NULL;
    size_t PkLen = 0;
    AvbSHA512Ctx Ctx = {{0}};
    size_t Index = 0;
    uint8_t* authoritydigest = NULL;
    AvbVBMetaImageHeader VbmetaHeader = {{0}};

    avb_sha512_init (&Ctx);
    for (Index = 0; Index < SlotData->num_vbmeta_images; Index++) {
        /* Authority hash includes hash of vbmeta's public key
         * and vbmeta_system's public key
         */
        if (avb_strcmp
            (SlotData->vbmeta_images[Index].partition_name, "vbmeta")
            == 0 ||
           avb_strcmp
            (SlotData->vbmeta_images[Index].partition_name, "vbmeta_system")
            == 0) {
            if (SlotData->vbmeta_images[Index].vbmeta_data == NULL) {
                Status = EFI_INVALID_PARAMETER;
                goto out;
            }
            avb_vbmeta_image_header_to_host_byte_order (
            (AvbVBMetaImageHeader*)(SlotData->vbmeta_images[Index].vbmeta_data),
                &VbmetaHeader);
            PkData = SlotData->vbmeta_images[Index].vbmeta_data +
                     sizeof (AvbVBMetaImageHeader) +
                     VbmetaHeader.authentication_data_block_size +
                     VbmetaHeader.public_key_offset;
            PkLen = VbmetaHeader.public_key_size;
            avb_sha512_update (&Ctx, PkData, PkLen);
        }
    }
    if (&Ctx == NULL ||
        bcc_params->ChildImage.AuthorityHash == NULL) {
        Status = EFI_INVALID_PARAMETER;
        goto out;
    }

    authoritydigest = avb_sha512_final (&Ctx);
    if (authoritydigest == NULL) {
        Status = EFI_INVALID_PARAMETER;
        goto out;
    }
    avb_memcpy (bcc_params->ChildImage.AuthorityHash, authoritydigest,
                DICE_HASH_SIZE);
out:
    return Status;
}

/* PopulateBccImgParams will populate Image measurements like image name,
 * code hash and authority hash. For now, only pvmfw image measurement is
 * populated.
 */
STATIC EFI_STATUS
PopulateBccImgParams (AvbSlotVerifyData *SlotData, BootInfo *Info,
                      BccParams_t *bcc_params, uint32_t PartitionIndex)
{
    EFI_STATUS Status = EFI_SUCCESS;
    AvbSHA512Ctx CodeCtx = {{0}};
    uint8_t* CodeDigest = NULL;
    uint32_t PnameLen = 0;
    char* SdvDiceComponentName = "PVM";
    uint32_t VbmetaIndex;
    char** partition_name = NULL;

    Status = PopulateAuthorityHash (SlotData, bcc_params);
    if (Status != EFI_SUCCESS) {
        DEBUG ((EFI_D_ERROR, "VB: PopulateAuthorityHash: failed with Status:%r",
              Status));
        goto out;
    }

    if (SlotData->loaded_partitions[PartitionIndex].partition_name == NULL ||
        bcc_params->ChildImage.ComponentName == NULL ||
        SlotData->loaded_partitions[PartitionIndex].data == NULL ||
        bcc_params->ChildImage.CodeHash == NULL) {
        Status = EFI_INVALID_PARAMETER;
        goto out;
    }

    if (!Info->HasSdvDiceEnabled) {
        partition_name = &SlotData->loaded_partitions[PartitionIndex].partition_name;
    } else {
        /* For SDV, the ComponentName is "PVM", and not the image name. */
        partition_name = &SdvDiceComponentName;
    }

    PnameLen = avb_strlen(*partition_name);
    if (PnameLen >= BCC_COMPONENT_NAME_BUFFER_MAX_SIZE) {
      Status = EFI_BUFFER_TOO_SMALL;
      goto out;
    }
    avb_memcpy (bcc_params->ChildImage.ComponentName,
                *partition_name,
                PnameLen);
    bcc_params->ChildImage.ComponentName[PnameLen] = '\0';

    avb_sha512_init (&CodeCtx);
    avb_sha512_update (&CodeCtx,
                       SlotData->loaded_partitions[PartitionIndex].data,
                       SlotData->loaded_partitions[PartitionIndex].data_size);

    /* For SDV, Vbmeta images are also included in the CodeDigest. */
    if (Info->HasSdvDiceEnabled) {
      for (VbmetaIndex = 0; VbmetaIndex < SlotData->num_vbmeta_images; VbmetaIndex++) {
            if (SlotData->vbmeta_images[VbmetaIndex].vbmeta_data != NULL) {
              avb_sha512_update (&CodeCtx,
                SlotData->vbmeta_images[VbmetaIndex].vbmeta_data,
                SlotData->vbmeta_images[VbmetaIndex].vbmeta_size);
            }
      }
    }

    CodeDigest = avb_sha512_final (&CodeCtx);
    if (CodeDigest == NULL) {
        Status = EFI_INVALID_PARAMETER;
        goto out;
    }
    avb_memcpy (bcc_params->ChildImage.CodeHash, CodeDigest, DICE_HASH_SIZE);

out:
    return Status;
}

/* PopulateBccParams will populate BCC measurements for DICE Engine, which
 * includes Authority hash, Code Hash and Mode.
 */
EFI_STATUS
PopulateBccParams (AvbSlotVerifyData *SlotData, BootInfo *Info,
                   BccParams_t *bcc_params)
{
    EFI_STATUS Status = EFI_SUCCESS;
    char* partition_measured_pvmfw = "pvmfw";
    char* partition_measured_sdv = "boot";
    char** partition_measured = NULL;

    if (Info->HasSdvDiceEnabled) {
       partition_measured = &partition_measured_sdv;
    }
    else {
       partition_measured = &partition_measured_pvmfw;
    }

    if (SlotData == NULL ||
        bcc_params == NULL) {
        DEBUG ((EFI_D_ERROR, "VB: PopulateBccParams: Parameter received"
                "is NULL"));
        Status = EFI_INVALID_PARAMETER;
        goto out;
    }

    // Set the DICE mode
    if (Info->BootIntoRecovery) {
         bcc_params->Mode = kDiceModeMaintenance;
    } else if (IsUnlocked ()) {
         bcc_params->Mode = kDiceModeDebug;
    } else {
         bcc_params->Mode = kDiceModeNormal;
    }

#ifdef DICE_FRS
    Status = KeyMasterGetFRSAndUDS (bcc_params);
    if (Status != EFI_SUCCESS) {
        DEBUG ((EFI_D_ERROR, "VB: AvbPopulateBccParams: failed with"
                " Status:%r\n", Status));

         SetDummyBccParams (bcc_params);
         goto out;
    }
#endif

    for (UINTN LoadedIndex = 0; LoadedIndex < SlotData->num_loaded_partitions;
        LoadedIndex++) {
        DEBUG ((EFI_D_ERROR, "Loaded Partition: %a\n",
                SlotData->loaded_partitions[LoadedIndex].partition_name));
        if (avb_strcmp (SlotData->loaded_partitions[LoadedIndex].partition_name,
                        *partition_measured) == 0 ) {
            if (SlotData->loaded_partitions[LoadedIndex].verify_result ==
                AVB_SLOT_VERIFY_RESULT_OK) {
                Status = PopulateBccImgParams (SlotData, Info, bcc_params,
                                               LoadedIndex);
                if (Status != EFI_SUCCESS) {
                    DEBUG ((EFI_D_ERROR, "VB: PopulateBccImgParams: failed with"
                            " Status:%r\n", Status));
                    goto out;
                }
                DEBUG ((EFI_D_INFO, "VB: Bcc Params populated\n"));
            } else {
                SetDummyBccParams (bcc_params);
            }
            break;
        }
    }
out:
  return Status;
}
