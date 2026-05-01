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

#include "host/snapshot.h"

#include "esp_heap_caps.h"
#include "esp_timer.h"

#include <stdio.h>
#include <sys/stat.h>

// Write 16-bit little-endian value
static void write_u16_le(FILE *fd, uint16_t v)
{
  fputc((int) ( v       & 0xFF), fd);
  fputc((int) ((v >> 8) & 0xFF), fd);
}

// Write 32-bit little-endian value
static void write_u32_le(FILE *fd, uint32_t v)
{
  fputc((int) ( v        & 0xFF), fd);
  fputc((int) ((v >>  8) & 0xFF), fd);
  fputc((int) ((v >> 16) & 0xFF), fd);
  fputc((int) ((v >> 24) & 0xFF), fd);
}

// Write signed 32-bit little-endian value
static void write_s32_le(FILE *f, int32_t v)
{
  write_u32_le(f, (uint32_t) v);
}

// Saves a BMP (8-bit indexed) image from raw VGA scanout data.
//
// Raw pixels coming from the VGA controller include HSYNC/VSYNC bits
// in bits 6..7. These bits are removed in-place, keeping only RGB222
// color information in bits 0..5.
int snapshot(uint16_t width, uint16_t height, uint8_t *src, const char *path)
{
  // Mask to keep only RGB222 bits (remove HSYNC/VSYNC bits 6..7)
  const uint8_t colorMask = 0x3F;

  // Clean raw pixels in-place
  uint32_t totalPixels = (uint32_t) width * height;
  for (uint32_t i = 0; i < totalPixels; i++) {
    src[i] &= colorMask;
  }

  // BMP rows must be aligned to 4 bytes
  uint32_t rowStride = (uint32_t) (width + 3) & ~3;
  uint32_t imageSize = rowStride * (uint32_t) height;
  uint32_t paletteSize = 256 * 4;
  uint32_t fileHeaderSize = 14;
  uint32_t infoHeaderSize = 40;
  uint32_t pixelDataOffset = fileHeaderSize + infoHeaderSize + paletteSize;
  uint32_t fileSize = pixelDataOffset + imageSize;

  char filename[64];
  struct stat st;
  int i = 0;

  do {
    snprintf(filename, sizeof(filename), "%s/%s%d.%s", path, SNAPSHOT_FILENAME_PREFIX, i++, SNAPSHOT_FILENAME_EXT);
  } while (stat(filename, &st) == 0);

  FILE *fd = fopen(filename, "wb");
  if (!fd) {
  	printf("snapshot: Unable to create file %s\n", filename);
    return -1;
  }

  // BITMAPFILEHEADER (14 bytes)
  fputc('B', fd);
  fputc('M', fd);
  write_u32_le(fd, fileSize);
  write_u16_le(fd, 0);
  write_u16_le(fd, 0);
  write_u32_le(fd, pixelDataOffset);

  // BITMAPINFOHEADER (40 bytes)
  // Use negative height for top-down bitmap
  write_u32_le(fd, infoHeaderSize);
  write_s32_le(fd, (int32_t) width);
  write_s32_le(fd, -(int32_t) height);
  write_u16_le(fd, 1);
  write_u16_le(fd, 8);
  write_u32_le(fd, 0);               // BI_RGB (no compression)
  write_u32_le(fd, imageSize);
  write_s32_le(fd, 0);
  write_s32_le(fd, 0);
  write_u32_le(fd, 256);
  write_u32_le(fd, 256);

  // Palette (256 entries, BGRA)
  // RGB222 layout:
  //   R: bits 0..1
  //   G: bits 2..3
  //   B: bits 4..5
  for (uint32_t v = 0; v < 256; v++) {

    uint8_t c = (uint8_t) (v & colorMask);

    uint8_t r2 = (c >> 0) & 3;
    uint8_t g2 = (c >> 2) & 3;
    uint8_t b2 = (c >> 4) & 3;

    uint8_t r8 = (uint8_t) (r2 * 85);
    uint8_t g8 = (uint8_t) (g2 * 85);
    uint8_t b8 = (uint8_t) (b2 * 85);

    fputc(b8, fd);
    fputc(g8, fd);
    fputc(r8, fd);
    fputc(0,  fd);
  }

  uint32_t pad = rowStride - (uint32_t) width;
  const uint8_t padByte = 0;

  const int64_t t0 = esp_timer_get_time();

  // 4KB output buffer in PSRAM (minimize DRAM usage)
  uint8_t *outBuf = (uint8_t *) heap_caps_malloc(4096, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!outBuf) {
    fclose(fd);
    printf("snapshot: Unable to allocate 4KB output buffer\n");
    return -1;
  }

  size_t outPos = 0;

  // Note that src is NOT linear since VGADirectController stores
  // pixels with byte swizzle: pixel X is stored at row[X ^ 2].
  for (uint16_t y = 0; y < height; y++) {

    const uint8_t *srcRow = src + (uint32_t) y * width;

    for (uint16_t x = 0; x + 3 < width; x += 4) {

      // Read 4 physical bytes from the swizzled row
      uint8_t b0 = srcRow[x + 0];
      uint8_t b1 = srcRow[x + 1];
      uint8_t b2 = srcRow[x + 2];
      uint8_t b3 = srcRow[x + 3];

      // Unswizzle to linear pixel order:
      // pixels [x+0..x+3] = bytes [2,3,0,1] and mask sync bits
      uint8_t p0 = b2 & colorMask;
      uint8_t p1 = b3 & colorMask;
      uint8_t p2 = b0 & colorMask;
      uint8_t p3 = b1 & colorMask;

      // Flush if buffer would overflow
      if (outPos + 4 > 4096) {
        fwrite(outBuf, 1, outPos, fd);
        outPos = 0;
      }

      outBuf[outPos++] = p0;
      outBuf[outPos++] = p1;
      outBuf[outPos++] = p2;
      outBuf[outPos++] = p3;
    }

    // Row padding (BMP alignment)
    for (uint32_t p = 0; p < pad; ++p) {
      if (outPos + 1 > 4096) {
        fwrite(outBuf, 1, outPos, fd);
        outPos = 0;
      }
      outBuf[outPos++] = padByte;
    }
  }

  // Final flush
  if (outPos) {
    fwrite(outBuf, 1, outPos, fd);
    outPos = 0;
  }

  heap_caps_free(outBuf);

  const int64_t t1 = esp_timer_get_time();

  fclose(fd);
  printf("snapshot: Image saved to %s (%ux%u) [%lld ms]\n", filename, width, height, (long long) ((t1 - t0) / 1000));
  return 0;
}
