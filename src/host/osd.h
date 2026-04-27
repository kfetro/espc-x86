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

#include <stdint.h>
#include <dirent.h>

#include "drivers/input/ps2_controller.h"
#include "drivers/input/keyboard.h"
#include "drivers/input/keycodes.h"

#define COL_BLACK         0
#define COL_BLUE          1
#define COL_GREEN         2
#define COL_CYAN          3
#define COL_RED           4
#define COL_MAGENTA       5
#define COL_BROWN         6
#define COL_LIGHTGRAY     7
#define COL_DARKGRAY      8
#define COL_LIGHTBLUE     9
#define COL_LIGHTGREEN    10
#define COL_LIGHTCYAN     11
#define COL_LIGHTRED      12
#define COL_LIGHTMAGENTA  13
#define COL_YELLOW        14
#define COL_WHITE         15

#define OSD_BUTTON_BG           COL_WHITE
#define OSD_BUTTON_FG           COL_BLACK
#define OSD_BUTTON_SEL_BG       COL_YELLOW
#define OSD_BUTTON_SEL_FG       COL_BLACK

#define OSD_RADIOLIST_FG        COL_WHITE
#define OSD_RADIOLIST_SEL_FG    COL_YELLOW

#define OSD_FILE_BG             COL_LIGHTGRAY
#define OSD_FILE_FG             COL_WHITE
#define OSD_FILE_SEL_BG         COL_BLACK
#define OSD_FILE_SEL_FG         COL_WHITE

#define OSD_SCROLL_BG           COL_BLACK
#define OSD_SCROLL_FG           COL_WHITE
#define OSD_SCROLL_THUMB_BG     COL_LIGHTGRAY
#define OSD_SCROLL_THUMB_FG     COL_WHITE
#define OSD_SCROLL_EMPTY_BG     COL_LIGHTGRAY
#define OSD_SCROLL_EMPTY_FG     COL_WHITE

#define OSD_PROGRESS_EMPTY_BG   COL_WHITE
#define OSD_PROGRESS_EMPTY_FG   COL_BLACK
#define OSD_PROGRESS_DONE_BG    COL_YELLOW
#define OSD_PROGRESS_DONE_FG    COL_BLACK
#define OSD_PROGRESS_TEXT_FG    COL_BLACK
#define OSD_PROGRESS_SHADOW_BG  COL_CYAN
#define OSD_PROGRESS_SHADOW_FG  COL_BLACK

#define OSD_MAX_RADIOGROUP_ITEMS  8

class OSD {

public:

   OSD(uint8_t *vram);
  ~OSD();

  void frame(int x, int y, int width, int height,
             const char *title, bool centered,
             uint8_t fg, uint8_t bg,
             bool simpleBorder, bool shadow);

  void box(int x, int y, int width, int height,
           uint8_t fg, uint8_t bg, bool border);

  void text(int x, int y, const char *txt, uint8_t fg, uint8_t bg);

  int menu(int x, int y, int width, const char *opts, uint8_t bg);
  int menuBar(int x, int y, int fixWidth, const char *opts, uint8_t bg);

  int radioList(int x, int y, const char *opts, int current, uint8_t bg);

  void radioGroupBegin();
  void radioGroupAdd(int x, int y, const char *opts, int *current);
  int radioGroupRun(uint8_t bg);

  int fileBrowser(int x, int y, int width, int height, const char *path, char *filename);

  void progress(int x, int y, int width, int percent, const char *txt, bool shadow);

private:

  uint8_t *m_vram;
  uint8_t m_fg;
  uint8_t m_bg;

  static constexpr int Columns = 80;
  static constexpr int Rows    = 25;

  void setForeground(uint8_t c);
  void setBackground(uint8_t c);
  void setColor(uint8_t fg, uint8_t bg);

  inline void putChar(int x, int y, char ch);
  inline void putTextRaw(int x, int y, const char *txt);
  inline void fillRect(int x, int y, int w, int h, char ch);

  int readKey();

  void drawBorder(int x, int y, int w, int h,
                  bool simpleBorder,
                  uint8_t fg, uint8_t bg);

  void drawShadow(int x, int y, int w, int h);

  void drawButtonShadow(int x, int y, int width,
                        uint8_t bg);

  void drawVScroll(int x, int y,
                   int visible, int total, int first);

  void drawRadioGroupItem(int index, bool focused, uint8_t bg);

  struct RadioGroupItem {
    int x;
    int y;
    char **items;
    int count;
    int *current;
  };

  RadioGroupItem m_radioGroup[OSD_MAX_RADIOGROUP_ITEMS];
  int m_radioGroupCount = 0;

  const char DL=(char)0xC9;
  const char DR=(char)0xBB;
  const char UL=(char)0xC8;
  const char UR=(char)0xBC;
  const char DH=(char)0xCD;
  const char DV=(char)0xBA;

  const char SL=(char)0xDA;
  const char SR=(char)0xBF;
  const char TL=(char)0xC0;
  const char TR=(char)0xD9;
  const char SH=(char)0xC4;
  const char SV=(char)0xB3;
};
