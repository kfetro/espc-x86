/*
 * Copyright (c) 2026 Jesus Martinez-Mateo
 *
 * Author: Jesus Martinez-Mateo <jesus.martinez.mateo@gmail.com>
 *
 * This file is part of a GPL-licensed project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "host/vfs_fat.h"

#include <dirent.h>
#include <stdio.h>

// FAT file system with VFS
#include "esp_err.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "diskio_impl.h"

// Logic unit (maximum FF_VOLUMES defined in ffconf.h)
#define CUSTOM_DRIVE 1

// FAT file systems typically use a sector size of 512 bytes
const size_t sector_size = 512;

static FILE *s_custom_fd[FF_VOLUMES] = { NULL };

void vfs_fat_floppy_init()
{
  FILE *fd;
  const int floppy_type = 1;

  static const uint8_t bootsector[512] = {
    0xeb, 0x3c, 0x90, 0x4d, 0x53, 0x57, 0x49, 0x4e, 0x34, 0x2e, 0x31, 0x00, 0x02, 0x01, 0x01, 0x00, 0x02, 0xe0, 0x00, 0x40, 0x0b, 0xf0, 0x09, 0x00,
    0x12, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x46, 0x41, 0x54, 0x31, 0x32, 0x20, 0x20, 0x20, 0x33, 0xc9, 0x8e, 0xd1, 0xbc, 0xfc, 0x7b, 0x16, 0x07, 0xbd,
    0x78, 0x00, 0xc5, 0x76, 0x00, 0x1e, 0x56, 0x16, 0x55, 0xbf, 0x22, 0x05, 0x89, 0x7e, 0x00, 0x89, 0x4e, 0x02, 0xb1, 0x0b, 0xfc, 0xf3, 0xa4, 0x06,
    0x1f, 0xbd, 0x00, 0x7c, 0xc6, 0x45, 0xfe, 0x0f, 0x38, 0x4e, 0x24, 0x7d, 0x20, 0x8b, 0xc1, 0x99, 0xe8, 0x7e, 0x01, 0x83, 0xeb, 0x3a, 0x66, 0xa1,
    0x1c, 0x7c, 0x66, 0x3b, 0x07, 0x8a, 0x57, 0xfc, 0x75, 0x06, 0x80, 0xca, 0x02, 0x88, 0x56, 0x02, 0x80, 0xc3, 0x10, 0x73, 0xed, 0x33, 0xc9, 0xfe,
    0x06, 0xd8, 0x7d, 0x8a, 0x46, 0x10, 0x98, 0xf7, 0x66, 0x16, 0x03, 0x46, 0x1c, 0x13, 0x56, 0x1e, 0x03, 0x46, 0x0e, 0x13, 0xd1, 0x8b, 0x76, 0x11,
    0x60, 0x89, 0x46, 0xfc, 0x89, 0x56, 0xfe, 0xb8, 0x20, 0x00, 0xf7, 0xe6, 0x8b, 0x5e, 0x0b, 0x03, 0xc3, 0x48, 0xf7, 0xf3, 0x01, 0x46, 0xfc, 0x11,
    0x4e, 0xfe, 0x61, 0xbf, 0x00, 0x07, 0xe8, 0x28, 0x01, 0x72, 0x3e, 0x38, 0x2d, 0x74, 0x17, 0x60, 0xb1, 0x0b, 0xbe, 0xd8, 0x7d, 0xf3, 0xa6, 0x61,
    0x74, 0x3d, 0x4e, 0x74, 0x09, 0x83, 0xc7, 0x20, 0x3b, 0xfb, 0x72, 0xe7, 0xeb, 0xdd, 0xfe, 0x0e, 0xd8, 0x7d, 0x7b, 0xa7, 0xbe, 0x7f, 0x7d, 0xac,
    0x98, 0x03, 0xf0, 0xac, 0x98, 0x40, 0x74, 0x0c, 0x48, 0x74, 0x13, 0xb4, 0x0e, 0xbb, 0x07, 0x00, 0xcd, 0x10, 0xeb, 0xef, 0xbe, 0x82, 0x7d, 0xeb,
    0xe6, 0xbe, 0x80, 0x7d, 0xeb, 0xe1, 0xcd, 0x16, 0x5e, 0x1f, 0x66, 0x8f, 0x04, 0xcd, 0x19, 0xbe, 0x81, 0x7d, 0x8b, 0x7d, 0x1a, 0x8d, 0x45, 0xfe,
    0x8a, 0x4e, 0x0d, 0xf7, 0xe1, 0x03, 0x46, 0xfc, 0x13, 0x56, 0xfe, 0xb1, 0x04, 0xe8, 0xc2, 0x00, 0x72, 0xd7, 0xea, 0x00, 0x02, 0x70, 0x00, 0x52,
    0x50, 0x06, 0x53, 0x6a, 0x01, 0x6a, 0x10, 0x91, 0x8b, 0x46, 0x18, 0xa2, 0x26, 0x05, 0x96, 0x92, 0x33, 0xd2, 0xf7, 0xf6, 0x91, 0xf7, 0xf6, 0x42,
    0x87, 0xca, 0xf7, 0x76, 0x1a, 0x8a, 0xf2, 0x8a, 0xe8, 0xc0, 0xcc, 0x02, 0x0a, 0xcc, 0xb8, 0x01, 0x02, 0x80, 0x7e, 0x02, 0x0e, 0x75, 0x04, 0xb4,
    0x42, 0x8b, 0xf4, 0x8a, 0x56, 0x24, 0xcd, 0x13, 0x61, 0x61, 0x72, 0x0a, 0x40, 0x75, 0x01, 0x42, 0x03, 0x5e, 0x0b, 0x49, 0x75, 0x77, 0xc3, 0x03,
    0x18, 0x01, 0x27, 0x0d, 0x0a, 0x49, 0x6e, 0x76, 0x61, 0x6c, 0x69, 0x64, 0x20, 0x73, 0x79, 0x73, 0x74, 0x65, 0x6d, 0x20, 0x64, 0x69, 0x73, 0x6b,
    0xff, 0x0d, 0x0a, 0x44, 0x69, 0x73, 0x6b, 0x20, 0x49, 0x2f, 0x4f, 0x20, 0x65, 0x72, 0x72, 0x6f, 0x72, 0xff, 0x0d, 0x0a, 0x52, 0x65, 0x70, 0x6c,
    0x61, 0x63, 0x65, 0x20, 0x74, 0x68, 0x65, 0x20, 0x64, 0x69, 0x73, 0x6b, 0x2c, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x68, 0x65, 0x6e, 0x20, 0x70,
    0x72, 0x65, 0x73, 0x73, 0x20, 0x61, 0x6e, 0x79, 0x20, 0x6b, 0x65, 0x79, 0x0d, 0x0a, 0x00, 0x00, 0x49, 0x4f, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x53, 0x59, 0x53, 0x4d, 0x53, 0x44, 0x4f, 0x53, 0x20, 0x20, 0x20, 0x53, 0x59, 0x53, 0x7f, 0x01, 0x00, 0x41, 0xbb, 0x00, 0x07, 0x60, 0x66, 0x6a,
    0x00, 0xe9, 0x3b, 0xff, 0x00, 0x00, 0x55, 0xaa
  };

  // Offset 0x0c (16 bytes):
  static const uint8_t geometry[3][16]  = {
    { 0x02, 0x02, 0x01, 0x00, 0x02, 0x70, 0x00, 0xA0, 0x05, 0xF9, 0x03, 0x00, 0x09, 0x00, 0x02, 0x00 },  // 720K
    { 0x02, 0x01, 0x01, 0x00, 0x02, 0xE0, 0x00, 0x40, 0x0B, 0xF0, 0x09, 0x00, 0x12, 0x00, 0x02, 0x00 },  // 1440K
    { 0x02, 0x02, 0x01, 0x00, 0x02, 0xF0, 0x00, 0x80, 0x16, 0xF0, 0x09, 0x00, 0x24, 0x00, 0x02, 0x00 },  // 2880K
  };

  // Sizes in bytes
  static const int size_bytes[3] = {
    512 *  9 * 80 * 2, // 737280 (720K)
    512 * 18 * 80 * 2, // 1474560 (1440K)
    512 * 36 * 80 * 2, // 2949120 (2880K)
  };

  // FAT
  static const struct {
    uint8_t id;
    uint8_t fat1size;  // in 512 blocks
    uint8_t fat2size;  // in 512 blocks
  } FAT[3] = {
    { 0xf9, 3, 10 },   // 720K
    { 0xf0, 9, 23 },   // 1440K
    { 0xf0, 9, 24 },   // 2880K
  };

  fd = s_custom_fd[CUSTOM_DRIVE];

/*
  auto sectbuf = (uint8_t *) SOC_EXTRAM_DATA_LOW; // use PSRAM as buffer

  // initial fill with all 0xf6 up to entire size
  memset(sectbuf, 0xf6, 512);
  fseek(fd, 0, SEEK_SET);

  for (int i = 0; i < size_bytes[floppy_type]; i += 512) {
    fwrite(sectbuf, 1, 512, fd);
  }
*/
  // Write boot sector
  // offset 0x00 (512 bytes)
  fseek(fd, 0, SEEK_SET);
  fwrite(bootsector, 1, 512, fd);

  // Disk geometry
  // offset 0x0c (16 bytes)
  fseek(fd, 0x0c, SEEK_SET);
  fwrite(geometry[floppy_type], 1, 16, fd);

  // Volume serial number
  // offset 0x27 (4 bytes)
  const uint32_t volSN = rand();
  fseek(fd, 0x27, SEEK_SET);
  fwrite(&volSN, 1, 4, fd);
/*
  // FAT
  memset(sectbuf, 0x00, 512);
  fseek(fd, 0x200, SEEK_SET);
  for (int i = 0; i < (FAT[floppy_type].fat1size + FAT[floppy_type].fat2size); i++) {
    fwrite(sectbuf, 1, 512, fd);
  }
  sectbuf[0] = FAT[floppy_type].id;
  sectbuf[1] = 0xff;
  sectbuf[2] = 0xff;
  fseek(fd, 0x200, SEEK_SET);
  fwrite(sectbuf, 1, 3, fd);
  fseek(fd, 0x200 + FAT[floppy_type].fat1size * 512, SEEK_SET);
  fwrite(sectbuf, 1, 3, fd);
*/
}

// Create a minimal FAT16 HDD image manually (MBR + 1 partition)
void vfs_fat_hdd_init(FILE *fd, uint32_t totalBytes)
{
  const uint16_t heads = 16;
  const uint16_t spt   = 63;
  const uint32_t totalSectors = totalBytes / 512;
  const uint32_t partStart = 63;  // first track
  const uint32_t partSectors = totalSectors - partStart;

  uint8_t sector[512];
  memset(sector, 0, 512);

  // --- MBR (Master Boot Record) ---
  uint8_t *p = sector + 0x1BE; // Pointer to the first partition entry

  p[0] = 0x80;        // Boot indicator (0x80 = active/bootable, 0x00 = inactive)
  
  // Starting CHS values for Sector 63 (C=0, H=1, S=1)
  p[1] = 0x01;        // Starting Head
  p[2] = 0x01;        // Starting Sector (bits 0-5) and Cylinder high (bits 6-7)
  p[3] = 0x00;        // Starting Cylinder low
  
  p[4] = 0x06;        // Partition Type (0x06 = FAT16 BIG, more compatible than 0x04)

  // Ending CHS values (Set to maximum 1023/254/63 to force LBA usage)
  p[5] = 0xFE;        // Ending Head
  p[6] = 0xFF;        // Ending Sector
  p[7] = 0xFF;        // Ending Cylinder

  // Starting LBA (Sector 63)
  p[8]  = partStart & 0xFF;
  p[9]  = (partStart >> 8) & 0xFF;
  p[10] = (partStart >> 16) & 0xFF;
  p[11] = (partStart >> 24) & 0xFF;

  // Partition size in sectors
  p[12] = partSectors & 0xFF;
  p[13] = (partSectors >> 8) & 0xFF;
  p[14] = (partSectors >> 16) & 0xFF;
  p[15] = (partSectors >> 24) & 0xFF;

  // MBR Boot Signature
  sector[510] = 0x55;
  sector[511] = 0xAA;

  fseek(fd, 0, SEEK_SET);
  fwrite(sector, 1, 512, fd);

#if 1
  // --- VBR ---
  memset(sector, 0, 512);
  sector[0] = 0xEB; sector[1] = 0x3C; sector[2] = 0x90;
  memcpy(sector + 3, "MSDOS5.0", 8);

  sector[0x0B] = 0x00; sector[0x0C] = 0x02;   // 512 bytes/sector
  sector[0x0D] = 0x01;                         // sectors/cluster
  sector[0x0E] = 0x01; sector[0x0F] = 0x00;   // reserved sectors
  sector[0x10] = 0x02;                         // FATs
  sector[0x11] = 0x00; sector[0x12] = 0x02;   // root entries = 512
  sector[0x13] = partSectors & 0xFF;
  sector[0x14] = (partSectors >> 8) & 0xFF;
  sector[0x15] = 0xF8;                         // fixed disk
  sector[0x16] = 0x40; sector[0x17] = 0x00;   // FAT size (64 sectors)
  sector[0x18] = spt & 0xFF;
  sector[0x19] = (spt >> 8) & 0xFF;
  sector[0x1A] = heads & 0xFF;
  sector[0x1B] = (heads >> 8) & 0xFF;
  sector[0x1C] = partStart & 0xFF;
  sector[0x1D] = (partStart >> 8) & 0xFF;
  sector[0x1E] = (partStart >> 16) & 0xFF;
  sector[0x1F] = (partStart >> 24) & 0xFF;
  sector[0x26] = 0x29;                         // extended signature
  memcpy(sector + 0x2B, "NO NAME    ", 11);
  memcpy(sector + 0x36, "FAT16   ", 8);
  sector[510] = 0x55;
  sector[511] = 0xAA;

  fseek(fd, partStart * 512, SEEK_SET);
  fwrite(sector, 1, 512, fd);

  // --- FATs + ROOT ---
  memset(sector, 0, 512);
  sector[0] = 0xF8;
  sector[1] = 0xFF;
  sector[2] = 0xFF;
  sector[3] = 0xFF;

  uint32_t fat1 = partStart + 1;
  uint32_t fat2 = fat1 + 64;
  uint32_t root = fat2 + 64;

  fseek(fd, fat1 * 512, SEEK_SET);
  fwrite(sector, 1, 512, fd);
  memset(sector, 0, 512);
  for (int i = 1; i < 64; i++) fwrite(sector, 1, 512, fd);

  fseek(fd, fat2 * 512, SEEK_SET);
  fwrite(sector, 1, 512, fd);
  for (int i = 1; i < 64; i++) fwrite(sector, 1, 512, fd);

  fseek(fd, root * 512, SEEK_SET);
  for (int i = 0; i < 32; i++) fwrite(sector, 1, 512, fd);
#else
  // Calculation of FAT Geometry
  const uint8_t sectorsPerCluster = 4; // Use 4 for better compatibility in small disks

  // Calculate how many sectors are needed for one FAT table
  // (partSectors / sectorsPerCluster) * 2 bytes per entry / 512 bytes per sector
  uint32_t fatSize = ((partSectors / sectorsPerCluster) * 2) / 512;
  if (fatSize == 0) {
    fatSize = 1; // Minimum 1 sector
  }

  // --- VBR (Volume Boot Record) ---
  memset(sector, 0, 512);
  sector[0] = 0xEB;
  sector[1] = 0x3C;
  sector[2] = 0x90;

  memcpy(sector + 3, "MSDOS5.0", 8);

  sector[0x0B] = 0x00;
  sector[0x0C] = 0x02; // 512 bytes/sector
  sector[0x0D] = sectorsPerCluster; // sectors/cluster (updated)
  sector[0x0E] = 0x01;
  sector[0x0F] = 0x00; // 1 reserved sector (the VBR itself)
  sector[0x10] = 0x02; // 2 FATs
  sector[0x11] = 0x00;
  sector[0x12] = 0x02; // 512 root entries
  
  // Logic for Total Sectors (Point 3)
  if (partSectors < 0xFFFF) {
      sector[0x13] = partSectors & 0xFF;        // 16-bit total sectors
      sector[0x14] = (partSectors >> 8) & 0xFF;
  } else {
      sector[0x13] = 0; sector[0x14] = 0;        // Set to 0 to use 32-bit field
      sector[0x20] = partSectors & 0xFF;        // 32-bit total sectors (offset 0x20)
      sector[0x21] = (partSectors >> 8) & 0xFF;
      sector[0x22] = (partSectors >> 16) & 0xFF;
      sector[0x23] = (partSectors >> 24) & 0xFF;
  }

  sector[0x15] = 0xF8;                         // Media descriptor (Fixed disk)
  sector[0x16] = fatSize & 0xFF;               // FAT size (Point 2 - updated)
  sector[0x17] = (fatSize >> 8) & 0xFF;
  
  // ... rest of VBR (spt, heads, etc. same as before) ...
  sector[0x18] = spt & 0xFF;
  sector[0x19] = (spt >> 8) & 0xFF;
  sector[0x1A] = heads & 0xFF;
  sector[0x1B] = (heads >> 8) & 0xFF;
  sector[0x1C] = partStart & 0xFF;
  sector[0x1D] = (partStart >> 8) & 0xFF;
  sector[0x1E] = (partStart >> 16) & 0xFF;
  sector[0x1F] = (partStart >> 24) & 0xFF;
  sector[0x26] = 0x29;
  memcpy(sector + 0x2B, "NO NAME    ", 11);
  memcpy(sector + 0x36, "FAT16   ", 8);
  sector[510] = 0x55;
  sector[511] = 0xAA;

  fseek(fd, partStart * 512, SEEK_SET);
  fwrite(sector, 1, 512, fd);

  // --- Dynamic FATs + ROOT ---
  // Now we use the calculated fatSize instead of the hardcoded 64
  uint32_t fat1Start = partStart + 1;
  uint32_t fat2Start = fat1Start + fatSize;
  uint32_t rootStart = fat2Start + fatSize;

  // Write FAT 1
  memset(sector, 0, 512);
  sector[0] = 0xF8;
  sector[1] = 0xFF;
  sector[2] = 0xFF;
  sector[3] = 0xFF; // Media + EOC
  fseek(fd, fat1Start * 512, SEEK_SET);
  fwrite(sector, 1, 512, fd);
  memset(sector, 0, 512);
  for (uint32_t i = 1; i < fatSize; i++) {
    fwrite(sector, 1, 512, fd);
  }

  // Write FAT 2
  fseek(fd, fat2Start * 512, SEEK_SET);
  sector[0] = 0xF8;
  sector[1] = 0xFF;
  sector[2] = 0xFF;
  sector[3] = 0xFF;
  fwrite(sector, 1, 512, fd);
  memset(sector, 0, 512);
  for (uint32_t i = 1; i < fatSize; i++) {
    fwrite(sector, 1, 512, fd);
  }

  // Write Root Directory (512 entries * 32 bytes/entry = 32 sectors)
  fseek(fd, rootStart * 512, SEEK_SET);
  for (int i = 0; i < 32; i++) {
    fwrite(sector, 1, 512, fd);
  }
#endif

  fflush(fd);
}

void custom_set_drive_file(BYTE pdrv, FILE *fd)
{
  if (pdrv < FF_VOLUMES) {
    s_custom_fd[pdrv] = fd;
  }
}

static DSTATUS custom_init(BYTE pdrv)
{
  // Nothing to initialize (file already opened)
  return 0;
}

static DSTATUS custom_status(BYTE pdrv)
{
  // 0 if file opened, STA_NOINIT othercase
  return (s_custom_fd[pdrv] != NULL) ? 0 : STA_NOINIT;
}

static DRESULT custom_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
  FILE *fd = s_custom_fd[pdrv];

  if (fd == NULL) {
		  return RES_NOTRDY;
  	}

  // Move to the sector
  long offset = (long) (sector * sector_size);
  if (fseek(fd, offset, SEEK_SET) != 0) {
    printf("VFS_FAT: Unable to seek (%ld)\n", offset);
    return RES_ERROR;
  }

  // Read an amount of sectors
  size_t bytes_to_read = sector_size * count;
  size_t bytes_read = fread(buff, 1, bytes_to_read, fd);
  if (bytes_read != bytes_to_read) {
    printf("VFS_FAT: Unable to read (%lu, %lu)\n", bytes_read, bytes_to_read);
    return RES_ERROR;
  }
  return RES_OK;
}

static DRESULT custom_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
  FILE *fd = s_custom_fd[pdrv];

  if (fd == NULL) {
  	return RES_NOTRDY;
  }

  // Move to the sector
  long offset = (long) (sector * sector_size);
  if (fseek(fd, offset, SEEK_SET) != 0) {
    printf("VFS_FAT: Unable to seek (%ld)\n", offset);
    return RES_ERROR;
  }

  // Write sectors
  size_t bytes_to_write = sector_size * count;
  size_t bytes_written = fwrite(buff, 1, bytes_to_write, fd);
  if (bytes_written != bytes_to_write) {
    printf("VFS_FAT: Unable to read (%lu, %lu)\n", bytes_written, bytes_to_write);
    return RES_ERROR;
  }
  fflush(fd);

  return RES_OK;
}

static DRESULT custom_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
  FILE *fd = s_custom_fd[pdrv];

  if (fd == NULL) {
	return RES_NOTRDY;
  }

  switch (cmd) {

    case CTRL_SYNC:
      fflush(fd);
      return RES_OK;

    case GET_SECTOR_COUNT: {
      // Obtener tamaño total del archivo y dividir por 512
      long current = ftell(fd);
      fseek(fd, 0, SEEK_END);
      long size = ftell(fd);
      fseek(fd, current, SEEK_SET);

      DWORD sector_count = size / sector_size;
      *(DWORD*) buff = sector_count;
      return RES_OK;
    }

    case GET_SECTOR_SIZE:
      *(WORD*) buff = sector_size;
      return RES_OK;

    case GET_BLOCK_SIZE:
      // Tamaño de bloque para borrado: 1 sector es suficiente
      *(DWORD*) buff = 1;
      return RES_OK;

    default:
      return RES_PARERR;
  }
}

static const ff_diskio_impl_t custom_impl = {
  .init   = &custom_init,
  .status = &custom_status,
  .read   = &custom_read,
  .write  = &custom_write,
  .ioctl  = &custom_ioctl
};

int vfs_fat_create_image(char *img_filename, const char *mount_point, bool floppy)
{
  esp_err_t ret;
  FATFS *fs;
  char drive[3] = {'0' + CUSTOM_DRIVE, ':', '\0'};

  printf("VFS_FAT: Creating image file (%s)...\n", img_filename);
  FILE *fd = fopen(img_filename, "wb+");
  if (fd == NULL) {
    printf("ERROR!\n");
    return VFS_FAT_ERROR;
  }

  // Extending file to FAT file system size
  if (floppy) {
    fseek(fd, VFS_FAT_FLOPPY_SIZE - 1, SEEK_SET);
  } else {
    fseek(fd, VFS_FAT_SIZE * 1024 * 1024 - 1, SEEK_SET);
  }
  fputc('\0', fd);
  fflush(fd);
  fseek(fd, 0, SEEK_SET);

  // File pointer stored as driver unit CUSTOM_DRIVE
  custom_set_drive_file(CUSTOM_DRIVE, fd);
  printf("VFS_FAT: Driver unit = %d\n", CUSTOM_DRIVE);

  // Registry FAT file system in VFS, it assigns "<CUSTOM_DRIVE>:" to "/fat"
  ret = esp_vfs_fat_register(mount_point, drive, 5, &fs);
  if (ret != ESP_OK) {
    printf("VFS_FAT: Unable to register VFS FAT (%d %s)\n", ret, esp_err_to_name(ret));
    fclose(fd);
    return VFS_FAT_ERROR;
  }
  printf("VFS_FAT: FAT registered path = %s\n", mount_point);

  ff_diskio_register(CUSTOM_DRIVE, &custom_impl);
  printf("VFS_FAT: Custom drive controller registered\n");

  static BYTE work[FF_MAX_SS];

/*
 * Not needed given that when formating the file system with f_mkfs
 * a partition will be created.
 * 
  if (!floppy) {
    printf("VFS_FAT: Partitioning... ");
    DWORD partitions[] = {100, 0, 0, 0};
    ret = f_fdisk(CUSTOM_DRIVE, partitions, work);
    if (ret != FR_OK) {
      printf("ERROR! (%d %s)\n", ret, esp_err_to_name(ret));
      fclose(fd);
      return VFS_FAT_ERROR;
    }
    printf("Ok\n");
  }
 */

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)

  MKFS_PARM opt;
 
  printf("VFS_FAT: Formatting... ");
  if (floppy) {
    // Format options
    opt.fmt = FM_ANY | FM_SFD;
    opt.n_fat = 2;    // Number of FAT copies
    opt.align = 0;    // Alignment of the volume data area (default 0)
    opt.n_root = 512;   // Number of root directory entries
    opt.au_size = 0;  // Size of the cluster (allocation unit) in unit of byte (default 0)
  } else {
    // Format options
    opt.fmt = FM_FAT;
    opt.n_fat = 2;    // Number of FAT copies
    opt.align = 0;    // Alignment of the volume data area (default 0)
    opt.n_root = 512;   // Number of root directory entries
    opt.au_size = 0;  // Size of the cluster (allocation unit) in unit of byte (default 0)
  }
  ret = f_mkfs(drive, &opt, work, sizeof(work));
  if (ret != FR_OK) {
    printf("ERROR! (%d %s)\n", ret, esp_err_to_name(ret));
    fclose(fd);
    return VFS_FAT_ERROR;
  }
  printf("Ok\n");

#else // ESP_IDF_VERSION

  const int ALLOCATION_UNIT_SIZE = 16384;

  printf("VFS_FAT: Formatting... ");
  if (floppy) {
    ret = f_mkfs(drive, FM_ANY | FM_SFD, ALLOCATION_UNIT_SIZE, work, sizeof(work));
  } else {
    //ret = f_mkfs(drive, FM_ANY, ALLOCATION_UNIT_SIZE, work, sizeof(work));
    ret = FR_OK;
    vfs_fat_hdd_init(fd, VFS_FAT_SIZE * 1024 * 1024);
  }
  if (ret != FR_OK) {
    printf("ERROR! (%d %s)\n", ret, esp_err_to_name(ret));
    fclose(fd);
    return VFS_FAT_ERROR;
  }
  printf("Ok\n");

#endif // ESP_IDF_VERSION

  if (floppy) {
    vfs_fat_floppy_init();
  }

  printf("VFS_FAT: Mounting FAT file system... ");
  ret = f_mount(fs, drive, 1);
  if (ret != FR_OK) {
    printf("ERROR! (%d %s)\n", ret, esp_err_to_name(ret));
    fclose(fd);
    return VFS_FAT_ERROR;
  }
  printf("Ok\n");
  return VFS_FAT_OK;
}

int vfs_fat_unmount_image(const char *mount_point)
{
  char drive[3] = {'0' + CUSTOM_DRIVE, ':', '\0'};

  // Unmount FAT file system
  f_mount(NULL, drive, 1);

  // Unregister FAT file sytem in VFS
  esp_vfs_fat_unregister_path(mount_point);
  ff_diskio_register(CUSTOM_DRIVE, NULL);

  fclose(s_custom_fd[CUSTOM_DRIVE]);
  s_custom_fd[CUSTOM_DRIVE] = NULL;

  printf("VFS_FAT: Image file system unmounted\n");
  return VFS_FAT_OK;
}

int vfs_fat_check_image(char *img_filename, char *mount_point)
{
  esp_err_t ret;
  FATFS *fs;
  char drive[3] = {'0' + CUSTOM_DRIVE, ':', '\0'};

  printf("VFS_FAT: Opening file (%s)...\n", img_filename);
  FILE *fd = fopen(img_filename, "r");
  if (fd == NULL) {
    printf("ERROR!\n");
    return VFS_FAT_ERROR;
  }

  // File pointer stored as driver unit CUSTOM_DRIVE
  custom_set_drive_file(CUSTOM_DRIVE, fd);
  printf("VFS_FAT: Driver unit = %d\n", CUSTOM_DRIVE);

  // Registry FAT file system in VFS, it assigns "<CUSTOM_DRIVE>:" to "/fat"
  ret = esp_vfs_fat_register(mount_point, drive, 5, &fs);
  if (ret != ESP_OK) {
    printf("VFS_FAT: Unable to register VFS FAT (%d %s)\n", ret, esp_err_to_name(ret));
    fclose(fd);
    return VFS_FAT_ERROR;
  }
  printf("VFS_FAT: FAT registered path = %s\n", mount_point);

  ff_diskio_register(CUSTOM_DRIVE, &custom_impl);
  printf("VFS_FAT: Custom drive controller registered\n");

  printf("VFS_FAT: Mounting FAT file system... ");
  ret = f_mount(fs, drive, 1);
  if (ret != FR_OK) {
    printf("ERROR! (%d %s)\n", ret, esp_err_to_name(ret));
    fclose(fd);
    return VFS_FAT_ERROR;
  }
  printf("Ok\n");

  printf("VFS_FAT: Listing files...\n");
  DIR *dir = opendir(mount_point);
  if (dir == NULL) {
    printf("ERROR! Unable to open dir %s\n", mount_point);
  } else {
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_type == DT_REG) {
        printf("%s\n", entry->d_name);
      }
    }
    closedir(dir);
  }

  // Unmount FAT file system
  f_mount(NULL, drive, 1);

  // Unregister FAT file sytem in VFS
  esp_vfs_fat_unregister_path(mount_point);
  ff_diskio_register(CUSTOM_DRIVE, NULL);

  fclose(s_custom_fd[CUSTOM_DRIVE]);
  s_custom_fd[CUSTOM_DRIVE] = NULL;

  printf("VFS_FAT: Image file system unmounted\n");
  return VFS_FAT_OK;
}
