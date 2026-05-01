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

#include <ctype.h>
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

static void normalizeKeymap(char *s)
{
  for (int i = 0; s[i] && i < 7; i++) {
    s[i] = (char) toupper((unsigned char) s[i]);
  }
  s[7] = 0;
}

static bool isSupportedKeymap(const char *s)
{
  return strcmp(s, "US") == 0 ||
         strcmp(s, "UK") == 0 ||
         strcmp(s, "DE") == 0 ||
         strcmp(s, "IT") == 0 ||
         strcmp(s, "ES") == 0 ||
         strcmp(s, "FR") == 0 ||
         strcmp(s, "BE") == 0 ||
         strcmp(s, "NO") == 0;
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

  strncpy(cfg->keymap, SETUP_DEFAULT_KEYMAP, sizeof(cfg->keymap) - 1);
  cfg->keymap[sizeof(cfg->keymap) - 1] = 0;
  normalizeKeymap(cfg->keymap);

  strcpy(cfg->media_path, SETUP_DEFAULT_MEDIA_PATH);
  strcpy(cfg->disks_path, SETUP_DEFAULT_DISKS_PATH);

  cfg->boot     = SETUP_DEFAULT_BOOT;
  for (int i = 0; i < 4; i++) {
    cfg->drive[i][0] = 0;
  }
}

// Ensures the project directory structure and default configuration 
// file exist on the SD card
void setupDirStructure()
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

int setupLoad(const char *path, Setup *cfg)
{
  FILE *fd;

  // Always start from defaults
  setupSetDefaults(cfg);

  // Check directory structure (and create them if missing)
  setupDirStructure();

  fd = fopen(path, "r");
  if (!fd) {
    printf("setup: Unable to open configuration file %s\n", path);
    // Configuration missing: create it with defaults
    printf("setup: Saving defaults...\n");
    setupSave(path, cfg);
    return SETUP_ERROR;
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
      // Video card
      cfg->video = (uint8_t) atoi(val);
    } else if (strcmp(key, "sound") == 0) {
      // Sound card
      cfg->sound = (uint8_t) atoi(val);
    } else if (strcmp(key, "speaker") == 0) {
      // PC Speaker
      cfg->speaker = atoi(val) ? true : false;
    } else if (strcmp(key, "joystick") == 0) {
      // Joystick
      cfg->joystick = atoi(val) ? 1 : 0;
    } else if (strcmp(key, "ram") == 0) {
      // Main Memory
      cfg->ram = (uint16_t) atoi(val);
    } else if (strcmp(key, "keyboard") == 0) {
      // Keyboard
      cfg->keyboard = (uint8_t) atoi(val);
    } else if (strcmp(key, "keymap") == 0) {
      // Keymap
      strncpy(cfg->keymap, val, sizeof(cfg->keymap) - 1);
      cfg->keymap[sizeof(cfg->keymap) - 1] = 0;
      normalizeKeymap(cfg->keymap);

      if (!isSupportedKeymap(cfg->keymap)) {
        printf("setup: Unsupported keymap! Setting default... %s\n", SETUP_DEFAULT_KEYMAP);
        strncpy(cfg->keymap, SETUP_DEFAULT_KEYMAP, sizeof(cfg->keymap) - 1);
        cfg->keymap[sizeof(cfg->keymap) - 1] = 0;
        normalizeKeymap(cfg->keymap);
      }
    } else if (strcmp(key, "turbo") == 0) {
      // Turbo
      cfg->turbo = atoi(val) ? true : false;
    } else if (strcmp(key, "mouse") == 0) {
      // Mouse
      cfg->mouse = atoi(val) ? true : false;
    } else if (strcmp(key, "media_path") == 0) {
      // Media Path
      strncpy(cfg->media_path, val, sizeof(cfg->media_path) - 1);
      cfg->media_path[sizeof(cfg->media_path) - 1] = 0;
    } else if (strcmp(key, "disks_path") == 0) {
      // Disks path
      strncpy(cfg->disks_path, val, sizeof(cfg->disks_path) - 1);
      cfg->disks_path[sizeof(cfg->disks_path) - 1] = 0;
    } else if (strncmp(key, "drive", 5) == 0) {
      // Drives
      int idx = key[5] - '0';
      if (idx >= 0 && idx < 4) {
        strncpy(cfg->drive[idx], val, sizeof(cfg->drive[idx]) - 1);
        cfg->drive[idx][sizeof(cfg->drive[idx]) - 1] = 0;
      }
    } else if (strcmp(key, "boot") == 0) {
      // Boot Drive
      cfg->boot = (uint8_t)atoi(val);
      if (cfg->boot > 3)
        cfg->boot = SETUP_DEFAULT_BOOT;
    }
    // Unknown keys are ignored
  }

  fclose(fd);
  return SETUP_OK;
}

int setupSave(const char *path, const Setup *cfg)
{
  FILE *fd;
  
  fd = fopen(path, "w");
  if (!fd) {
    printf("setup: Unable to save configuration file %s\n", path);
    return SETUP_ERROR;
  }

  fprintf(fd, "// ESPC-x86 emulator setup\n\n");

  fprintf(fd, "video=%d\n", cfg->video);
  fprintf(fd, "sound=%d\n", cfg->sound);
  fprintf(fd, "speaker=%d\n", cfg->speaker ? 1 : 0);
  fprintf(fd, "joystick=%d\n\n", cfg->joystick);

  fprintf(fd, "ram=%d\n", cfg->ram);
  fprintf(fd, "keyboard=%d\n", cfg->keyboard);
  fprintf(fd, "keymap=%s\n", cfg->keymap);
  fprintf(fd, "turbo=%d\n", cfg->turbo ? 1 : 0);
  fprintf(fd, "mouse=%d\n\n", cfg->mouse ? 1 : 0);

  fprintf(fd, "media_path=%s\n", cfg->media_path);
  fprintf(fd, "disks_path=%s\n\n", cfg->disks_path);

  for (int i = 0; i < 4; i++) {
    fprintf(fd, "drive%d=%s\n", i, cfg->drive[i]);
  }
  fprintf(fd, "\nboot=%d\n", cfg->boot);

  fclose(fd);
  return SETUP_OK;
}
