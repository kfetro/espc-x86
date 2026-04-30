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

// Recursively creates all directories in a path (mkdir -p behavior)
void mkdir_recursive(const char *path)
{
  // Make a mutable copy of the path
  char tmp[512];
  char *p = NULL;
  size_t len;

  snprintf(tmp, sizeof(tmp), "%s", path);
  len = strlen(tmp);

  // If path ends with '/' remove it
  if (tmp[len - 1] == '/') {
    tmp[len - 1] = 0;
  }
  // Whenever we find '/', temporarily terminate the string
  // and make directory
  for (p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = 0;
      mkdir(tmp, 0755);
      *p = '/';
    }
  }
  mkdir(tmp, 0755);
}

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

// Aggregated unzip context, this avoids multiple allocs
// and makes cleanup trivial
typedef struct {
  unz_file_info fileInfo;
  char    szComment[256];
  char    szName[256];
  char    filepath[512]; // Destination full path
  uint8_t buffer[4096];  // Extraction I/O buffer
  UNZIP   hZip; // unzipLIB ZIP context
} unzip_t;

int unzip_file_to_path(char *filename, const char *path, unzip_progress_callback_t progress_callback, void *ctx)
{
  int rc;

  unzip_t *unzip = (unzip_t *) heap_caps_malloc(sizeof(unzip_t), MALLOC_CAP_SPIRAM);
  if (!unzip) {
    printf("unzip: ERROR! Unable to allocate memory\n");
    return UNZIP_ERROR;
  }

  rc = unzip->hZip.openZIP(filename, unzip_open_callback,
                                     unzip_close_callback,
                                     unzip_read_callback,
                                     unzip_seek_callback);
  if (rc != UNZ_OK) {
    printf("unzip: ERROR! Unable to open zip file %s\n", filename);
    heap_caps_free((void *) unzip);
    return UNZIP_ERROR;
  }

  rc = unzip->hZip.getGlobalComment(unzip->szComment, sizeof(unzip->szComment));
  printf("zip: Global comment = %s\n", unzip->szComment);

  unsigned long totalSize = 0;
  if (progress_callback) {
  	// Pre-compute total size
    rc = unzip->hZip.gotoFirstFile();
    while (rc == UNZ_OK) {
      rc = unzip->hZip.getFileInfo(&unzip->fileInfo,
                                   unzip->szName, sizeof(unzip->szName),
                                   NULL, 0,
                                   unzip->szComment, sizeof(unzip->szComment));
      if (rc == UNZ_OK) {
        totalSize += unzip->fileInfo.uncompressed_size;
      }
      rc = unzip->hZip.gotoNextFile();
    }
  }

  unsigned long curSize = 0;
  rc = unzip->hZip.gotoFirstFile();
  while (rc == UNZ_OK) {
    rc = unzip->hZip.getFileInfo(&unzip->fileInfo,
                                 unzip->szName, sizeof(unzip->szName),
                                 NULL, 0,
                                 unzip->szComment, sizeof(unzip->szComment));
    if (rc == UNZ_OK) {
      printf("unzip: %s (%lu/%lu)\n", unzip->szName, unzip->fileInfo.compressed_size,
                                                     unzip->fileInfo.uncompressed_size);

      snprintf(unzip->filepath, sizeof(unzip->filepath), "%s/%s", path, unzip->szName);
  
      bool is_dir = (unzip->szName[strlen(unzip->szName) - 1] == '/');

      if (is_dir) {
        mkdir_recursive(unzip->filepath);
      } else {
        char *last_slash = strrchr(unzip->filepath, '/');
        if (last_slash) {
          *last_slash = 0;
          mkdir_recursive(unzip->filepath);
          *last_slash = '/';
        }

        FILE *fd = fopen(unzip->filepath, "wb");
        if (fd == NULL) {
          printf("unzip: Unable to create file %s\n", unzip->filepath);
        } else {
          if (unzip->hZip.openCurrentFile() == UNZ_OK) {
            int bytes_read = unzip->hZip.readCurrentFile(unzip->buffer, sizeof(unzip->buffer));
            while (bytes_read > 0) {
              size_t bytes_written = fwrite(unzip->buffer, 1, bytes_read, fd);
              if (bytes_written != bytes_read) {
                printf("unzip: Unable to write data (%d, %d)\n", bytes_written, bytes_read);
              }
              bytes_read = unzip->hZip.readCurrentFile(unzip->buffer, sizeof(unzip->buffer));
            }
            unzip->hZip.closeCurrentFile();
          }
          fclose(fd);
        }

        if (progress_callback) {
          curSize += unzip->fileInfo.uncompressed_size;
          progress_callback(ctx, (int) (100 * curSize / totalSize), unzip->szName);
        }

        // Yield the CPU and avoid WDT in long loops
        vTaskDelay(1);
      }
    }
    rc = unzip->hZip.gotoNextFile();
  }

  unzip->hZip.closeZIP();

  heap_caps_free((void *) unzip);

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
