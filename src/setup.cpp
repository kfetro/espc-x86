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

#include "setup.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static char *trim(char *s)
{
  while ((*s == ' ') || (*s == '\t'))
    s++;

  if (*s == 0)
    return s;

  char *end = s + strlen(s) - 1;
  while (end > s &&
        ((*end == ' ')  || (*end == '\t') ||
         (*end == '\r') || (*end == '\n'))) {
    *end-- = 0;
  }
  return s;
}

static void setupSetDefaults(Setup *cfg)
{
  cfg->video    = SETUP_DEFAULT_VIDEO;
  cfg->sound    = SETUP_DEFAULT_SOUND;
  cfg->speaker  = SETUP_DEFAULT_SPEAKER;
  cfg->joystick = SETUP_DEFAULT_JOYSTICK;

  cfg->ram      = SETUP_DEFAULT_RAM;
  cfg->keyboard = SETUP_DEFAULT_KEYBOARD;
  cfg->turbo    = SETUP_DEFAULT_TURBO;
  cfg->mouse    = SETUP_DEFAULT_MOUSE;

  strcpy(cfg->media_path, SETUP_DEFAULT_MEDIA_PATH);
  strcpy(cfg->disks_path, SETUP_DEFAULT_DISKS_PATH);

  cfg->boot     = SETUP_DEFAULT_BOOT;

  for (int i = 0; i < 4; i++)
    cfg->drive[i][0] = 0;
}

// Ensures the project directory structure and default configuration 
// file exist on the SD card
void checkDirs()
{
  struct stat st = {0};

  // Ensure the base project directory exists
  if (stat(SETUP_BASE_PATH, &st) == -1) {
    if (mkdir(SETUP_BASE_PATH, 0775) == 0) {
      printf("setup: Created directory: %s\n", SETUP_BASE_PATH);
    } else {
      printf("setup: Error creating %s (errno: %d)\n", SETUP_BASE_PATH, errno);
    }
  }

  // Ensure the media directory exists
  if (stat(SETUP_DEFAULT_MEDIA_PATH, &st) == -1) {
    if (mkdir(SETUP_DEFAULT_MEDIA_PATH, 0775) == 0) {
      printf("setup: Created directory: %s\n", SETUP_DEFAULT_MEDIA_PATH);
    } else {
      printf("setup: Error creating %s (errno: %d)\n", SETUP_DEFAULT_MEDIA_PATH, errno);
    }
  }

  // Ensure the disks directory exists
  if (stat(SETUP_DEFAULT_DISKS_PATH, &st) == -1) {
    if (mkdir(SETUP_DEFAULT_DISKS_PATH, 0775) == 0) {
      printf("setup: Created directory: %s\n", SETUP_DEFAULT_DISKS_PATH);
    } else {
      printf("setup: Error creating %s (errno: %d)\n", SETUP_DEFAULT_DISKS_PATH, errno);
    }
  }

  // Ensure the snapshots directory exists
  if (stat(SETUP_SNAPHOTS_PATH, &st) == -1) {
    if (mkdir(SETUP_SNAPHOTS_PATH, 0775) == 0) {
      printf("setup: Created directory: %s\n", SETUP_SNAPHOTS_PATH);
    } else {
      printf("setup: Error creating %s (errno: %d)\n", SETUP_SNAPHOTS_PATH, errno);
    }
  }
}

bool setupLoad(const char *path, Setup *cfg)
{
  // Always start from defaults
  setupSetDefaults(cfg);

  checkDirs();

  FILE *fd = fopen(path, "r");
  if (!fd) {
    // Configuration missing: create it with defaults
    setupSave(path, cfg);
    return false;
  }

  char line[256];

  while (fgets(line, sizeof(line), fd)) {

    char *s = trim(line);

    // Ignore empty lines and comments
    if ((*s == 0) || (*s == '#') || (*s == ';') || ((*s == '/') && (s[1] == '/')))
      continue;

    char *eq = strchr(s, '=');
    if (!eq)
      continue;

    *eq = 0;
    char *key = trim(s);
    char *val = trim(eq + 1);

    if (strcmp(key, "video") == 0) {
      cfg->video = (uint8_t) atoi(val);
    } else if (strcmp(key, "sound") == 0) {
      cfg->sound = (uint8_t)atoi(val);
    } else if (strcmp(key, "speaker") == 0) {
      cfg->speaker = atoi(val) ? true : false;
    } else if (strcmp(key, "joystick") == 0) {
      cfg->joystick = atoi(val) ? 1 : 0;
    } else if (strcmp(key, "ram") == 0) {
      cfg->ram = (uint16_t)atoi(val);
    } else if (strcmp(key, "keyboard") == 0) {
      cfg->keyboard = (uint8_t)atoi(val);
    } else if (strcmp(key, "turbo") == 0) {
      cfg->turbo = atoi(val) ? true : false;
    } else if (strcmp(key, "mouse") == 0) {
      cfg->mouse = atoi(val) ? true : false;
    } else if (strcmp(key, "media_path") == 0) {
      strncpy(cfg->media_path, val, sizeof(cfg->media_path) - 1);
      cfg->media_path[sizeof(cfg->media_path) - 1] = 0;
    } else if (strcmp(key, "disks_path") == 0) {
      strncpy(cfg->disks_path, val, sizeof(cfg->disks_path) - 1);
      cfg->disks_path[sizeof(cfg->disks_path) - 1] = 0;
    } else if (strncmp(key, "drive", 5) == 0) {
      int idx = key[5] - '0';
      if (idx >= 0 && idx < 4) {
        strncpy(cfg->drive[idx], val, sizeof(cfg->drive[idx]) - 1);
        cfg->drive[idx][sizeof(cfg->drive[idx]) - 1] = 0;
      }
    } else if (strcmp(key, "boot") == 0) {
      cfg->boot = (uint8_t)atoi(val);
      if (cfg->boot > 3)
        cfg->boot = SETUP_DEFAULT_BOOT;
    }
    // Unknown keys are ignored
  }

  fclose(fd);
  return true;
}

bool setupSave(const char *path, const Setup *cfg)
{
  FILE *f = fopen(path, "w");
  if (!f)
    return false;

  fprintf(f, "// i8086 emulator setup\n\n");

  fprintf(f, "video=%d\n",    cfg->video);
  fprintf(f, "sound=%d\n",    cfg->sound);
  fprintf(f, "speaker=%d\n",  cfg->speaker ? 1 : 0);
  fprintf(f, "joystick=%d\n\n", cfg->joystick);

  fprintf(f, "ram=%d\n",      cfg->ram);
  fprintf(f, "keyboard=%d\n", cfg->keyboard);
  fprintf(f, "turbo=%d\n",    cfg->turbo ? 1 : 0);
  fprintf(f, "mouse=%d\n\n",  cfg->mouse ? 1 : 0);

  fprintf(f, "media_path=%s\n", cfg->media_path);
  fprintf(f, "disks_path=%s\n\n", cfg->disks_path);

  for (int i = 0; i < 4; i++) {
    fprintf(f, "drive%d=%s\n", i, cfg->drive[i]);
  }

  fprintf(f, "\nboot=%d\n", cfg->boot);

  fclose(f);
  return true;
}
