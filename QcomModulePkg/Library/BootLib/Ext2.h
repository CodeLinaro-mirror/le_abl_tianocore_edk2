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

/* ext2 - Second Extended filesystem */
#include "AutoGen.h"

/* The offset the super block. */
#define EXT2_SB_OFFSET 0x400
/* Magic value used to identify an ext2 filesystem.  */
#define EXT2_MAGIC 0xEF53
/* The Inode nmber of Root Dir . */
#define ROOT_INO 2
/* Amount of indirect blocks in an inode.  */
#define INDIRECT_BLOCKS 12
#define SIZE_4K 4096
#define DIR_ENT_OFFSET 0x18
#define MAX_NAMELEN 255

/* Filetype used in directory entry.  */
#define FILETYPE_UNKNOWN 0
#define FILETYPE_REG 1
#define FILETYPE_DIRECTORY 2
#define FILETYPE_SYMLINK 7

/* The ext2 superblock.  */
struct ext2_sblock {
  UINT32 TotalInodes;
  UINT32 TotalBlocks;
  UINT32 ReservedBlocks;
  UINT32 FreeBlocks;

  UINT32 FreeInodes;
  UINT32 FirstDataBlock;
  UINT32 BlockSize;
  UINT32 FragmentSize;

  UINT32 BlocksPerGroup;
  UINT32 FragmentsPerGroup;
  UINT32 InodesPerGroup;
  UINT32 MntTime;

  UINT32 WriteTime;
  UINT16 MntCount;
  UINT16 MaxMntCount;
  UINT16 Magic;
  UINT16 FsState;
  UINT16 ErrorHandling;
  UINT16 MinorRevisionLevel;

  UINT32 LastCheck;
  UINT32 CheckInterval;
  UINT32 CreatorOs;
  UINT32 RevisionLevel;

  UINT16 UidReserved;
  UINT16 GidReserved;
  UINT32 FirstInode;
  UINT16 InodeSize;
  UINT16 BlockGroupNumber;
  UINT32 FeatureCompatibility;

  UINT32 FeatureIncompat;
  UINT32 FeatureRoCompat;
  UINT16 Uuid[8];

  CHAR8 VolumeName[16];
  CHAR8 LastMountedOn[64];
  UINT32 CompressionInfo;
  UINT8 PreallocBlocks;
  UINT8 PreallocDirBlocks;
  UINT16 ReservedGdtBlocks;
  UINT8 JournalUuid[16];
  UINT32 JournalNum;
  UINT32 JournalDev;
  UINT32 LastOrphan;
  UINT32 HashSeed[4];
  UINT8 DefHashVersion;
  UINT8 JnlBackupType;
  UINT16 GroupDescSize;
  UINT32 DefaultMountOpts;
  UINT32 FirstMetaBg;
  UINT32 MkfsTime;
  UINT32 JnlBlocks[17];
};

/* The ext2 blockgroup.  */
struct ext2_block_group {
  UINT32 BlockId;
  UINT32 InodeId;
  UINT32 InodeTableId;
  UINT16 FreeBlocks;
  UINT16 FreeInodes;
  UINT16 UsedDirs;
  UINT16 Pad;
  UINT32 Reserved[3];
  UINT32 BlockIdHi;
  UINT32 InodeIdHi;
  UINT32 InodeTableIdHi;
  UINT16 FreeBlocksHi;
  UINT16 FreeInodesHi;
  UINT16 UsedDirsHi;
  UINT16 Pad2;
  UINT32 Reserved2[3];
};

/* The ext2 inode.  */
struct Ext2Inode {
  UINT16 Mode;
  UINT16 Uid;
  UINT32 Size;
  UINT32 AcTime;
  UINT32 CreatTime;

  UINT32 MdfTime;
  UINT32 DelTime;
  UINT16 GrId;
  UINT16 LinksCnt;
  UINT32 BlockCnt;

  UINT32 Flags;
  UINT32 Osd1;
  UINT32 Block[INDIRECT_BLOCKS];
  UINT32 IndirBlock;
  UINT32 DouIndirBlock;

  UINT32 TriIndirBlock;
  UINT32 Version;
  UINT32 Acl;
  UINT32 SizeHigh;

  UINT32 FragmentAddr;
  UINT32 Osd2[3];
};

/* The header of an ext2 directory entry.  */
struct Ext2Dirent {
  UINT32 Inode;
  UINT16 DirentLen;
  UINT8 NameLen;
  UINT8 FileType;
  /* the name as followed */
};
