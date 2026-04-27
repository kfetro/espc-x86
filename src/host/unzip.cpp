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

#include "host/unzip.h"

#if UNZIP_USE_UNZIPLIB

// Unzip
#include "unzipLIB.h"

// FAT file system with VFS
#include "esp_vfs.h"
// Memory allocation
#include "esp_heap_caps.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

// UNZIP zip; // statically allocate the UNZIP structure (41K)

void *unzip_open_callback(const char *szFilename, int32_t *pFileSize)
{
  FILE *fd = fopen(szFilename, "rb");
  if (!fd) {
    printf("zip: Unable to open file\n");
    return NULL;
  }
  fseek(fd, 0, SEEK_END);
  *pFileSize = (int32_t) ftell(fd);
  return (void *) fd;
}

void unzip_close_callback(void *pFile)
{
  ZIPFILE *pzf = (ZIPFILE*) pFile;
  FILE *fd = (FILE*) pzf->fHandle;
  if (fd) {
    fclose(fd);
  }
}

int32_t unzip_read_callback(void *pFile, uint8_t *pBuf, int32_t iLen)
{
  ZIPFILE *pzf = (ZIPFILE*) pFile;
  FILE *fd = (FILE*) pzf->fHandle;
  if (!fd) {
    printf("zip: Unable to read (file not opened)\n");
    return 0;
  }
  return fread(pBuf, 1, iLen, fd);
}

int32_t unzip_seek_callback(void *pFile, int32_t iPosition, int iType)
{
  ZIPFILE *pzf = (ZIPFILE*) pFile;
  FILE *fd = (FILE*) pzf->fHandle;
  if (!fd) {
    printf("zip: Unable to write (file not opened)\n");
    return 0;
  }
  return fseek(fd, iPosition, iType);
}

int unzip_file_to_path(char *filename, const char *path, unzip_progress_callback_t progress_callback, void *ctx)
{
  int rc;
  UNZIP *zip;
  static char szComment[256], szName[256];
  static char filepath[512];

  zip = (UNZIP *) heap_caps_malloc(sizeof(UNZIP), MALLOC_CAP_SPIRAM);
  if (!zip) {
    printf("unzip: ERROR! Unable to allocate memory\n");
    return UNZIP_ERROR;
  }

  rc = zip->openZIP(filename, unzip_open_callback, unzip_close_callback, unzip_read_callback, unzip_seek_callback);
  if (rc != UNZ_OK) {
    printf("unzip: ERROR! Unable to open zip file %s\n", filename);
    heap_caps_free(zip);
    return UNZIP_ERROR;
  }

  rc = zip->getGlobalComment(szComment, sizeof(szComment));
  printf("zip: Global comment = %s\n", szComment);

  unsigned long totalSize = 0;
  if (progress_callback) {
    rc = zip->gotoFirstFile();
    while (rc == UNZ_OK) {
      unz_file_info f_info;
      rc = zip->getFileInfo(&f_info, szName, sizeof(szName), NULL, 0, szComment, sizeof(szComment));
      if (rc == UNZ_OK) {
        totalSize += f_info.uncompressed_size;
      }
      rc = zip->gotoNextFile();
    }
  }

  unsigned long curSize = 0;
  rc = zip->gotoFirstFile();
  while (rc == UNZ_OK) {
    unz_file_info f_info;

    rc = zip->getFileInfo(&f_info, szName, sizeof(szName), NULL, 0, szComment, sizeof(szComment));
    if (rc == UNZ_OK) {
      printf("unzip: %s (%lu/%lu)\n", szName, f_info.compressed_size, f_info.uncompressed_size);
      if (zip->openCurrentFile() == UNZ_OK) {
        sprintf(filepath, "%s/%s", path, szName);

        FILE *fd = fopen(filepath, "wb");
        if (fd == NULL) {
          printf("unzip: Unable to create file %s\n", filepath);
        } else {
          static uint8_t buffer[4096];
          int bytes_read = zip->readCurrentFile(buffer, sizeof(buffer));
          while (bytes_read > 0) {
            size_t bytes_written = fwrite(buffer, 1, bytes_read, fd);
            if (bytes_written != bytes_read) {
              printf("unzip: Unable to write data (%d, %d)\n", bytes_written, bytes_read);
            }
            bytes_read = zip->readCurrentFile(buffer, sizeof(buffer));
          }
          fclose(fd);
        }

      if (progress_callback) {
        curSize += f_info.uncompressed_size;
        progress_callback(ctx, (int) (100 * curSize / totalSize), szName);
      }

      zip->closeCurrentFile();
      }
    }

    // Yield the CPU and avoid WDT in long loops
    vTaskDelay(1);

    rc = zip->gotoNextFile();
  }

  zip->closeZIP();

  heap_caps_free(zip);

// Only for debug
#if 0
  DIR *dir = opendir(path);
  if (dir == NULL) {
    printf("Unable to open dir %s\n", path);
  } else {
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_type == DT_REG) {
        printf("%s\n", entry->d_name);
      }
    }
    closedir(dir);
  }
#endif

  return UNZIP_OK;
}

#endif // UNZIP_USE_UNZIPLIB

#if UNZIP_USE_MINIZ

static void *my_alloc_func(void *opaque, size_t items, size_t size)
{
    printf("alloc %d %d\n", items, size);
    return heap_caps_malloc(items * size, MALLOC_CAP_SPIRAM);
}

static void my_free_func(void *opaque, void *address)
{
    printf("free\n");
    heap_caps_free(address);
}

static void *my_realloc_func(void *opaque, void *address, size_t items, size_t size)
{
    printf("realloc\n");
    return heap_caps_realloc(address, items * size, MALLOC_CAP_SPIRAM);
}

// miniz calls this function repeatedly with chunks of the uncompressed data
// file_ofs: posición actual en el archivo descomprimido
static size_t unzip_callback_write(void *pUser_data, mz_uint64 file_ofs, const void *pBuffer, size_t len)
{
  FILE *fd = (FILE*) pUser_data;
  size_t written = fwrite(pBuffer, 1, len, fd);
  return written;
}

int uncompress_file(char *filename, char *path)
{
  mz_bool status;
  mz_zip_archive *zip_archive;
  mz_zip_archive_file_stat file_stat;
  mz_uint num_files;
  size_t unzip_size;
  static char filepath[256];
/*
 mz_stream stream;
    memset(&stream, 0, sizeof(stream));

    // Asigna tus funciones
    stream.zalloc = my_alloc_func;
    stream.zfree = my_free_func;
    stream.opaque = NULL; // Puntero opcional para contexto de usuario

    // Inicializa miniz (ejemplo con deflate)
    if (mz_inflateInit(&stream) != MZ_OK)
        printf("error\n");
*/

  zip_archive = (mz_zip_archive*) heap_caps_malloc(sizeof(mz_zip_archive), MALLOC_CAP_SPIRAM);
  if (zip_archive == NULL) {
    printf("unzip: ERROR! Unable to allocate memory\n");
    return UNZIP_ERROR;
  }

  // Now try to open the archive
  memset(zip_archive, 0, sizeof(mz_zip_archive));

  zip_archive->m_pAlloc = &my_alloc_func;
  zip_archive->m_pFree = &my_free_func;
  zip_archive->m_pRealloc = &my_realloc_func;

printf("pipo3\n");
  status = mz_zip_reader_init_file(zip_archive, filename, 0);
  if (!status) {
    printf("unzip: ERROR! Unable to open zip file %s\n", filename);
    free(zip_archive);
    return UNZIP_ERROR;
  }

printf("pipo4\n");

  // Get and print information about each file in the archive
  num_files = mz_zip_reader_get_num_files(zip_archive);
  for (int i = 0; i < (int) num_files; i++) {

    if (!mz_zip_reader_file_stat(zip_archive, i, &file_stat)) {
      printf("unzip: Unable to get file stats (%d)\n", i);
      mz_zip_reader_end(zip_archive);
      free(zip_archive);
      return UNZIP_ERROR;
    }
    // file_stat.m_comment
    printf("unzip: %s (%d/%d)\n", file_stat.m_filename, (int) file_stat.m_uncomp_size, (int) file_stat.m_comp_size);
    // mz_zip_reader_is_file_a_directory(&zip_archive, i));

#if 1
printf("pipo5_1\n");
    sprintf(filepath, "%s/%s", path, file_stat.m_filename);
printf("pipo5_2\n");
    FILE *fd = fopen(filepath, "wb");
    if (fd == NULL) {
        printf("unzip: ERROR! Unable to create file %s\n", filepath);
    }
printf("pipo5_3\n");
    status = mz_zip_reader_extract_to_callback(zip_archive, i, unzip_callback_write, fd, 0);
    if (!status) {
      printf("unzip: Error when reading file %s\n", file_stat.m_filename);
    }
printf("pipo5_4\n");
    fclose(fd);
#else
printf("pipo5\n");
    sprintf(filepath, "%s/%s", path, file_stat.m_filename);
    printf("file %s\n", filepath);
    status = mz_zip_reader_extract_to_file(zip_archive, i, filepath, 0);
    if (!status) {
      printf("unzip: Error when reading file %s\n", file_stat.m_filename);
    }
printf("pipo6\n");
#endif

  }

  mz_zip_reader_end(zip_archive);
  free(zip_archive);

  return UNZIP_OK;
}

#endif // UNZIP_USE_MINIZ
