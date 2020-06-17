/* Copyright (c) 2020, The Linux Foundation. All rights reserved.
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

#include "Ext2.h"
#include <Library/BootLinux.h>

STATIC struct ext2_sblock *Ext2Sb;
STATIC struct ext2_block_group *Ext2BlkGrp;
STATIC struct Ext2Inode *Ext2Inode;
STATIC EFI_BLOCK_IO_PROTOCOL *BlkIo;
STATIC HandleInfo HandleInfoList[1];
UINT32 BlkMult = 0;
UINT32 BlkIdOffset = 0;

/* Initialze the super block */
STATIC EFI_STATUS Ext2Init (VOID)
{
  EFI_STATUS Status;
  PartiSelectFilter HandleFilter;
  STATIC UINT32 MaxHandles;
  STATIC UINT32 BlkIOAttrib = 0;
  VOID *Ext2SbTmp = NULL;

  BlkIOAttrib = BLK_IO_SEL_PARTITIONED_MBR;
  BlkIOAttrib |= BLK_IO_SEL_PARTITIONED_GPT;
  BlkIOAttrib |= BLK_IO_SEL_MEDIA_TYPE_NON_REMOVABLE;
  BlkIOAttrib |= BLK_IO_SEL_MATCH_PARTITION_LABEL;

  HandleFilter.RootDeviceType = NULL;
  HandleFilter.PartitionLabel = L"boot_a";
  HandleFilter.VolumeName = NULL;

  MaxHandles = sizeof (HandleInfoList) / sizeof (*HandleInfoList);

  Status =
      GetBlkIOHandles (BlkIOAttrib, &HandleFilter, HandleInfoList, &MaxHandles);

  if (Status == EFI_SUCCESS) {
    if (MaxHandles == 0)
      return EFI_NO_MEDIA;

    if (MaxHandles != 1) {
      // Unable to deterministically load from single partition
      DEBUG (
          (EFI_D_ERROR, "ExecImgFromVolume(): multiple partitions found.\r\n"));
      return EFI_LOAD_ERROR;
    }
  } else {
    DEBUG ((DEBUG_WARN, "%s: GetBlkIOHandles failed: %r\n", __func__, Status));
    return Status;
  }

  /* Initialize the super block */
  BlkIo = HandleInfoList[0].BlkIo;
  Ext2SbTmp = AllocatePages (
      ALIGN_PAGES (sizeof (struct ext2_sblock), ALIGNMENT_MASK_4KB));
  if (!Ext2SbTmp) {
    DEBUG ((DEBUG_WARN, "Failed to allocate for Ext2 Sb\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = BlkIo->ReadBlocks (
      BlkIo, BlkIo->Media->MediaId, 0,
      ROUND_TO_PAGE (sizeof (struct ext2_sblock), BlkIo->Media->BlockSize - 1),
      Ext2SbTmp);

  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_WARN, "%s: ReadBlocks failed: %r\n", __func__, Status));
    return Status;
  }

  Ext2Sb = Ext2SbTmp + EXT2_SB_OFFSET;
  if (Ext2Sb->Magic != EXT2_MAGIC) {
    DEBUG ((DEBUG_WARN, "Ext2Init : not an ext2 filesystem, magic 0x%x\n",
            Ext2Sb->Magic));
    return EFI_INVALID_PARAMETER;
  }

  Ext2Sb->BlockSize = 1024 << Ext2Sb->BlockSize;
  BlkMult = BlkIo->Media->BlockSize / Ext2Sb->BlockSize;
  BlkIdOffset = EXT2_SB_OFFSET / Ext2Sb->BlockSize;
  DEBUG ((DEBUG_INFO, "BlockSize:  %u\n", Ext2Sb->BlockSize));
  Ext2BlkGrp = AllocatePages (
      ALIGN_PAGES (sizeof (struct ext2_block_group), ALIGNMENT_MASK_4KB));
  if (!Ext2BlkGrp) {
    DEBUG ((DEBUG_WARN, "Failed to allocate for Ext2 Block Group\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  Ext2Inode = AllocatePages (
      ALIGN_PAGES (sizeof (struct Ext2Inode), ALIGNMENT_MASK_4KB));
  if (!Ext2Inode) {
    DEBUG ((DEBUG_WARN, "Failed to allocate for Ext2 Inode\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  return Status;
}

/* Get the Inode content */
STATIC struct Ext2Inode *
Ext2ReadInode (INT32 Ino)
{
  UINT32 GrpBlkId;
  UINT32 GrpBlkIdTemp;
  UINT32 InodeBlkId;
  UINT32 InodeBlkIdTemp;
  struct Ext2Inode *Inode = NULL;
  struct ext2_block_group *Ext2BlkGrpTemp = NULL;

  Ino--; // Inode num start from 1
  GrpBlkId = (Ino / Ext2Sb->InodesPerGroup) * Ext2Sb->BlocksPerGroup + 1;
  Inode = Ext2Inode;
  Ext2BlkGrpTemp = Ext2BlkGrp;
  GrpBlkIdTemp = (GrpBlkId + BlkIdOffset) / BlkMult;

  BlkIo->ReadBlocks (BlkIo, BlkIo->Media->MediaId, GrpBlkIdTemp, SIZE_4K,
                     Ext2BlkGrpTemp);
  Ext2BlkGrpTemp = (VOID *)Ext2BlkGrpTemp +
                   Ext2Sb->BlockSize * ((GrpBlkId + BlkIdOffset) % BlkMult);
  InodeBlkId = Ext2BlkGrpTemp->InodeTableId +
               (Ino * Ext2Sb->InodeSize) / Ext2Sb->BlockSize;
  InodeBlkIdTemp = InodeBlkId / BlkMult;

  BlkIo->ReadBlocks (BlkIo, BlkIo->Media->MediaId, InodeBlkIdTemp, SIZE_4K,
                     Inode);

  Inode = (VOID *)Inode + (Ext2Sb->BlockSize * (InodeBlkId % BlkMult)) +
          ((Ino * Ext2Sb->InodeSize) % Ext2Sb->BlockSize);

  return Inode;
}

/* Get the inode num of the the Fname,
   Return 0 if failed to find. */
STATIC UINT32
Ext2FindFile (CHAR8 *Fname, UINT32 DirIno)
{
  struct Ext2Dirent *Ext2Dir, *Ext2DirEnt;
  struct Ext2Inode *Inode;
  CHAR8 FileName[MAX_NAMELEN + 1];
  UINT32 Ino = 0;

  if (!Fname)
    return 0;

  /*get the content of this dir*/
  Inode = Ext2ReadInode (DirIno);
  Ext2DirEnt = AllocatePages (
      ALIGN_PAGES (sizeof (struct Ext2Dirent), ALIGNMENT_MASK_4KB));
  if (!Ext2DirEnt) {
    DEBUG ((DEBUG_WARN, "Failed to allocate for Ext2 Dir\n"));
    return 0;
  }
  BlkIo->ReadBlocks (BlkIo, BlkIo->Media->MediaId, Inode->Block[0] / BlkMult,
                     SIZE_4K, Ext2DirEnt);

  Ext2Dir = (VOID *)Ext2DirEnt +
            Ext2Sb->BlockSize * (Inode->Block[0] % BlkMult) + DIR_ENT_OFFSET;
  while (1) {
    /*reach the end*/
    if (Ext2Dir->Inode == 0)
      break;

    memcpy (FileName, (VOID *)Ext2Dir + sizeof (struct Ext2Dirent),
            Ext2Dir->NameLen);
    FileName[Ext2Dir->NameLen] = '\0';
    if (AsciiStrCmp (FileName, Fname) == 0) {
      Ino = Ext2Dir->Inode;
      break;
    }

    if (Ext2Dir->FileType == FILETYPE_DIRECTORY) {
      Ino = Ext2FindFile (Fname, Ext2Dir->Inode);
      if (Ino > 0)
        break;
    }

    Ext2Dir =
        (struct Ext2Dirent *)((UINT32 *)Ext2Dir + (sizeof (struct Ext2Dirent) +
                                                   Ext2Dir->NameLen + 3) /
                                                      4);
  }

  FreePages (Ext2DirEnt,
             ALIGN_PAGES (sizeof (struct Ext2Dirent), ALIGNMENT_MASK_4KB));
  return Ino;
}

/* Find the Match in Str, and initialize the Dest */
STATIC EFI_STATUS
Ext2FindStr (CHAR8 *Str, CONST CHAR8 *Match, CHAR8 *Dest)
{
  BOOLEAN found = FALSE;

  if (!Str || !Match)
    return EFI_INVALID_PARAMETER;

  for (; *Str; Str++) {
    if (AsciiStrnCmp (Str, Match, strlen (Match)) == 0) {
      found = TRUE;
      break;
    }
  }

  if (!found)
    return EFI_INVALID_PARAMETER;

  Str += strlen (Match);
  for (; *Str; Str++) {
    if (*Str == 0x22 || *Str == 0x27) // 0x22 = ", 0x27 = '
      continue;
    if (*Str == '\n' || *Str == ';') {
      *Dest = '\0';
      break;
    }
    *Dest = *Str;
    Dest++;
  }

  return EFI_SUCCESS;
}

STATIC VOID
Ext2PrintStr (CHAR8 *Str)
{
  for (; *Str != '\0'; Str++)
    DEBUG ((EFI_D_INFO, "%c", *Str));
}

/* Read the Block of BlockID to Buffer;
   Shift: 1 means the Block is Indirect blocks;
          2 means Double indirect;
          3 means Triple indirect;
   RetVal: TRUE means read complete;
           FALSE means need continue. */
STATIC BOOLEAN
Ext2ReadBlock (UINT32 BlockID,
               UINT32 Shift,
               VOID **Buffer,
               UINT32 *BlockCount,
               UINT32 *Blocksum)
{
  BOOLEAN Complete = FALSE;
  UINT32 *Block = NULL;
  UINT32 *BlockHead = NULL;
  UINT32 BlockIdTemp = 0xFFFFFFFF;
  VOID *BufferTemp = NULL;
  INT32 i;
  INT32 pre_i = -1;
  UINT32 BufferAlign = 0;

  if (BlockID == 0)
    return TRUE; // read complete

  Block = AllocatePages (ALIGN_PAGES (Ext2Sb->BlockSize, ALIGNMENT_MASK_4KB));
  if (!Block) {
    DEBUG ((DEBUG_WARN, "Failed to allocate for block\n"));
    return FALSE;
  }
  BlockHead = Block;

  BufferTemp = AllocatePages (ALIGN_PAGES (SIZE_4K, ALIGNMENT_MASK_4KB));
  if (!BufferTemp) {
    DEBUG ((DEBUG_WARN, "Failed to allocate for BufferTemp\n"));
    FreePages (BlockHead, ALIGN_PAGES (Ext2Sb->BlockSize, ALIGNMENT_MASK_4KB));
    return FALSE;
  }

  BlkIo->ReadBlocks (BlkIo, BlkIo->Media->MediaId, BlockID / BlkMult, SIZE_4K,
                     Block);
  Block = (VOID *)Block + Ext2Sb->BlockSize * (BlockID % BlkMult);

  for (i = 0; i < (Ext2Sb->BlockSize / sizeof (UINT32)); i++) {
    if (Block[i] == 0) {
      if (*BlockCount >= *Blocksum) {
        Complete = TRUE; // read complete
        break;
      } else {
        continue;
      }
    }
    (*BlockCount)++;

    if (Shift == 1) { // Direct blocks
      BufferAlign = Ext2Sb->BlockSize * (i - pre_i - 1);
      if (BufferAlign) {
        memset (*Buffer, 0, BufferAlign);
        *Buffer += BufferAlign;
      }
      pre_i = i;
      if (BlockIdTemp == (Block[i] / BlkMult)) {
        memcpy (*Buffer,
                BufferTemp + (Ext2Sb->BlockSize * (Block[i] % BlkMult)),
                Ext2Sb->BlockSize);
        *Buffer += Ext2Sb->BlockSize;
        continue;
      }
      BlockIdTemp = Block[i] / BlkMult;
      memset (BufferTemp, 0,
              ALIGNMENT_MASK_4KB *
                  ALIGN_PAGES (Ext2Sb->BlockSize, ALIGNMENT_MASK_4KB));
      BlkIo->ReadBlocks (BlkIo, BlkIo->Media->MediaId, BlockIdTemp, SIZE_4K,
                         BufferTemp);
      memcpy (*Buffer, BufferTemp + (Ext2Sb->BlockSize * (Block[i] % BlkMult)),
              Ext2Sb->BlockSize);
      *Buffer += Ext2Sb->BlockSize;
    } else { // Indirect blocks, recurrently call
      if (Ext2ReadBlock (Block[i], Shift - 1, Buffer, BlockCount, Blocksum)) {
        Complete = TRUE;
        break; // read complete
      }
    }
  }

  FreePages (BlockHead, ALIGN_PAGES (Ext2Sb->BlockSize, ALIGNMENT_MASK_4KB));
  FreePages (BufferTemp, ALIGN_PAGES (SIZE_4K, ALIGNMENT_MASK_4KB));
  return Complete;
}

/* Read a file of Inode to Buf */
STATIC VOID
Ext2ReadFile (struct Ext2Inode *Inode, VOID *Buf)
{
  INT32 i;
  INT32 pre_i = -1;
  UINT32 BufferAlign = 0;
  VOID *Buffer = Buf;
  UINT32 BlkIdTemp = 0xFFFFFFFF;
  VOID *BufferTemp = NULL;
  UINT32 BlockCount = 0;
  UINT32 BlockSum = (Inode->BlockCnt * 512) / Ext2Sb->BlockSize;

  BufferTemp = AllocatePages (ALIGN_PAGES (SIZE_4K, ALIGNMENT_MASK_4KB));
  if (!BufferTemp) {
    DEBUG ((DEBUG_WARN, "Failed to allocate for BufferTemp\n"));
    return;
  }
  /* Direct blocks */
  for (i = 0; i < INDIRECT_BLOCKS; i++) {
    if (Inode->Block[i] == 0) {
      if (BlockCount >= BlockSum) {
        goto out;
      } else {
        continue;
      }
    }
    BlockCount++;

    BufferAlign = Ext2Sb->BlockSize * (i - pre_i - 1);
    if (BufferAlign) {
      memset (Buffer, 0, BufferAlign);
      Buffer += BufferAlign;
    }
    pre_i = i;

    if (BlkIdTemp == (Inode->Block[i] / BlkMult)) {
      memcpy (Buffer,
              BufferTemp + (Ext2Sb->BlockSize * (Inode->Block[i] % BlkMult)),
              Ext2Sb->BlockSize);
      Buffer += Ext2Sb->BlockSize;
      continue;
    }
    BlkIdTemp = Inode->Block[i] / BlkMult;
    memset (BufferTemp, 0,
            ALIGNMENT_MASK_4KB *
                ALIGN_PAGES (Ext2Sb->BlockSize, ALIGNMENT_MASK_4KB));
    BlkIo->ReadBlocks (BlkIo, BlkIo->Media->MediaId, BlkIdTemp, SIZE_4K,
                       BufferTemp);

    memcpy (Buffer,
            BufferTemp + Ext2Sb->BlockSize * (Inode->Block[i] % BlkMult),
            Ext2Sb->BlockSize);
    Buffer += Ext2Sb->BlockSize;
  }
  /* Indirect blocks */
  BlockCount++;
  if (Ext2ReadBlock (Inode->IndirBlock, 1, &Buffer, &BlockCount, &BlockSum)) {
    goto out;
  }

  /* Double indirect */
  BlockCount++;
  if (Ext2ReadBlock (Inode->DouIndirBlock, 2, &Buffer, &BlockCount,
                     &BlockSum)) {
    goto out;
  }
  /* Triple indirect, fail means it exceed the file Size limit */
  BlockCount++;
  ASSERT (
      Ext2ReadBlock (Inode->TriIndirBlock, 3, &Buffer, &BlockCount, &BlockSum));

out:
  FreePages (BufferTemp, ALIGN_PAGES (SIZE_4K, ALIGNMENT_MASK_4KB));
  return;
}

/* Set the Cmdline */
STATIC VOID
Ext2SetCmdLine (BootInfo *Info, VOID *BootImgBuf, VOID *MenuBuf)
{
  EFI_STATUS Status;
  CHAR8 MenuCmd[MAX_NAMELEN + 1];
  CHAR8 *CmdLine = (CHAR8 *)((boot_img_hdr *)BootImgBuf)->cmdline;

  if (!CmdLine || !(*CmdLine))
    return;

  /* get the parameter from menu.cfg */
  CmdLine[BOOT_ARGS_SIZE - 1] = '\0';
  Status = Ext2FindStr (MenuBuf, "cmdline=", MenuCmd);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Failed to find cmdline from menu.cfg!\n"));
  } else {
    AsciiStrCatS (CmdLine, BOOT_ARGS_SIZE, MenuCmd);
  }

  return;
}

/* Parse the menu.cfg and load the boot image */
STATIC EFI_STATUS
Ext2Load (BootInfo *Info)
{
  EFI_STATUS Status;
  UINT32 Ino;
  struct Ext2Inode *Inode;
  VOID *MenuBuf, *BootImgBuf;
  CHAR8 BootImgStr[MAX_NAMELEN + 1];
  UINT32 ImageHdrSize = BOOT_IMG_MAX_PAGE_SIZE;
  UINT32 PageSize = 0;
  UINT32 ImageSizeActual = 0;
  UINT32 MenuBlockCnt = 0;
  /* Parse the menu.cfg */
  Ino = Ext2FindFile ("menu.cfg", ROOT_INO);
  if (!Ino) {
    DEBUG ((EFI_D_ERROR, "Failed to find menu.cfg\n"));
    return EFI_INVALID_PARAMETER;
  }
  Inode = Ext2ReadInode (Ino);
  MenuBlockCnt = Inode->BlockCnt;
  MenuBuf =
      AllocatePages (ALIGN_PAGES (MenuBlockCnt * 512, ALIGNMENT_MASK_4KB));
  if (!MenuBuf) {
    DEBUG ((EFI_D_ERROR, "Failed to allocate for Ext2 Dir\n"));
    return EFI_BAD_BUFFER_SIZE;
  }
  Ext2ReadFile (Inode, MenuBuf);

  /* Got the menu.cfg, find the boot image */
  Status = Ext2FindStr (MenuBuf, "boot=", BootImgStr);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Failed to find BootImgStr\n"));
    Status = EFI_INVALID_PARAMETER;
    goto out;
  }
  Ino = Ext2FindFile (BootImgStr, ROOT_INO);
  if (!Ino) {
    DEBUG ((EFI_D_ERROR, "Failed to find file %s\n", BootImgStr));
    Status = EFI_INVALID_PARAMETER;
    goto out;
  }

  /* Got the Image Inode, load it*/
  Inode = Ext2ReadInode (Ino);
  Ext2PrintStr (BootImgStr);
  DEBUG ((EFI_D_INFO, " as boot image, size %u\n", Inode->Size));
  BootImgBuf =
      AllocatePages (ALIGN_PAGES (Inode->BlockCnt * 512, ALIGNMENT_MASK_4KB));
  if (!BootImgBuf) {
    DEBUG ((EFI_D_ERROR, "Failed to allocate for boot image\n"));
    Status = EFI_BAD_BUFFER_SIZE;
    goto out;
  }
  Ext2ReadFile (Inode, BootImgBuf);

  // Add check for boot image header and kernel page size
  // ensure kernel command line is terminated
  Status = CheckImageHeader (BootImgBuf, ImageHdrSize, &ImageSizeActual,
                             &PageSize, Info->BootIntoRecovery);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Invalid boot image header:%r\n", Status));
    FreePages (BootImgBuf,
               ALIGN_PAGES (Inode->BlockCnt * 512, ALIGNMENT_MASK_4KB));
    goto out;
  }
  /* init the BootInfo */
  StrnCpyS (Info->Pname, MAX_GPT_NAME_SIZE, (CONST CHAR16 *)L"boot_a",
            (UINTN)StrLen (L"boot_a"));
  Info->MultiSlotBoot = TRUE;
  Info->Images[0].ImageBuffer = BootImgBuf;
  Info->Images[0].ImageSize = ImageSizeActual;

  if (!VerifiedBootEnbled ()) {
    /* setup the cmdline */
    Ext2SetCmdLine (Info, BootImgBuf, MenuBuf);
  }

out:
  FreePages (MenuBuf, ALIGN_PAGES (MenuBlockCnt * 512, ALIGNMENT_MASK_4KB));
  return Status;
}

/* Free the memory allocated */
STATIC VOID Ext2Exit (VOID)
{
  if (Ext2Sb != NULL)
    FreePages ((VOID *)Ext2Sb - EXT2_SB_OFFSET,
               ALIGN_PAGES (sizeof (struct ext2_sblock), ALIGNMENT_MASK_4KB));

  if (Ext2BlkGrp != NULL)
    FreePages (Ext2BlkGrp, ALIGN_PAGES (sizeof (struct ext2_block_group),
                                        ALIGNMENT_MASK_4KB));

  if (Ext2Inode != NULL)
    FreePages (Ext2Inode,
               ALIGN_PAGES (sizeof (struct Ext2Inode), ALIGNMENT_MASK_4KB));
}

/* Load boot image with Ext2 FS */
EFI_STATUS
LoadImageWithFs (BootInfo *Info)
{
  EFI_STATUS Status;

  DEBUG ((DEBUG_INFO, "LoadImageWithFs start : %u ms\n", GetTimerCountms ()));

  Status = Ext2Init ();
  if (Status != EFI_SUCCESS)
    goto out;

  Status = Ext2Load (Info);

out:
  Ext2Exit ();

  DEBUG ((DEBUG_INFO, "LoadImageWithFs end : %u ms, %s\n", GetTimerCountms (),
          (Status == EFI_SUCCESS) ? L"Success" : L"Failed"));
  return Status;
}
