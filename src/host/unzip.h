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

#pragma once

#include <stddef.h>

#define UNZIP_OK     0
#define UNZIP_ERROR  1

#define UNZIP_USE_UNZIPLIB 1
//#define UNZIP_USE_MINIZ 1

typedef void (*unzip_progress_callback_t)(void *ctx,
                                          int percent,
                                          const char *msg);


// Extract a ZIP file to a destination path.
// progress_callback may be NULL (no progress reporting)
int unzip_file_to_path(char *filename,
                       const char *path,
                       unzip_progress_callback_t progress_callback = NULL,
                       void *ctx = NULL);
