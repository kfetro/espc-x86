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

#define VFS_FAT_OK    0
#define VFS_FAT_ERROR 1

// FAT file system size in MB
#define VFS_FAT_SIZE 8

// Floppy size in bytes
// 2 sides x 80 tracks x 18 sectors x 512 bytes per sector
#define VFS_FAT_FLOPPY_SIZE 1474560

int vfs_fat_create_image(char *img_filename, const char *mount_point, bool floppy = true);
int vfs_fat_unmount_image(const char *mount_point);

int vfs_fat_check_image(char *img_filename, char *mount_point);
