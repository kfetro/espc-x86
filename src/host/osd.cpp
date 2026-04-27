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

#include "osd.h"
#include <string.h>
#include <stdio.h>

OSD::OSD(uint8_t *vram) :
  m_vram(vram),
  m_fg(COL_WHITE),
  m_bg(COL_BLACK)
{
  if (!fabgl::PS2Controller::initialized()) {
    fabgl::PS2Controller::begin(fabgl::PS2Preset::KeyboardPort0_MousePort1, fabgl::KbdMode::GenerateVirtualKeys);
  } else {
    fabgl::PS2Controller::keyboard()->enableVirtualKeys(true, true);
  }
  fillRect(0, 0, Columns, Rows, ' ');
}

OSD::~OSD()
{
  fabgl::PS2Controller::keyboard()->enableVirtualKeys(false, false);
}

void OSD::setForeground(uint8_t c)
{
  m_fg = c & 0x0F;
}

void OSD::setBackground(uint8_t c)
{
  m_bg = c & 0x0F;
}

void OSD::setColor(uint8_t fg, uint8_t bg)
{
  m_fg = fg & 0x0F;
  m_bg = bg & 0x0F;
}

inline void OSD::putChar(int x, int y, char ch)
{
  if ((x < 0) || (x >= Columns) || (y < 0) || (y >= Rows))
    return;

  int pos = (y * Columns + x) * 2;
  m_vram[pos] = (uint8_t) ch;
  m_vram[pos + 1] = (m_bg << 4) | m_fg;
}

inline void OSD::putTextRaw(int x, int y, const char *txt)
{
  while (*txt) {
    putChar(x++, y, *txt++);
  }
}

inline void OSD::fillRect(int x, int y, int w, int h, char ch)
{
  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      putChar(x + xx, y + yy, ch);
    }
  }
}

int OSD::readKey()
{
  fabgl::VirtualKeyItem k;

  while (true) {
    if (fabgl::PS2Controller::keyboard()->virtualKeyAvailable()) {
      fabgl::PS2Controller::keyboard()->getNextVirtualKey(&k);
      if (!k.down) {
        return k.vk;
      }
    }
    vTaskDelay(5/portTICK_PERIOD_MS);
  }
}

void OSD::drawBorder(int x, int y, int w, int h,
                     bool simpleBorder,
                     uint8_t fg, uint8_t bg)
{
  setColor(fg, bg);

  char tl = simpleBorder ? SL : DL;
  char tr = simpleBorder ? SR : DR;
  char bl = simpleBorder ? TL : UL;
  char br = simpleBorder ? TR : UR;
  char hc = simpleBorder ? SH : DH;
  char vc = simpleBorder ? SV : DV;

  for (int i = x + 1; i < x + w - 1; i++) {
    putChar(i, y,         hc);
    putChar(i, y + h - 1, hc);
  }

  for (int j = y + 1; j < y + h - 1; j++) {
    putChar(x,         j, vc);
    putChar(x + w - 1, j, vc);
  }

  putChar(x,         y,         tl);
  putChar(x + w - 1, y,         tr);
  putChar(x,         y + h - 1, bl);
  putChar(x + w - 1, y + h - 1, br);
}

void OSD::drawShadow(int x, int y, int w, int h)
{
  setColor(COL_BLACK, COL_BLACK);

  for (int yy = y + 1; yy < y + h; yy++) {
    putChar(x + w, yy, ' ');
  }
  for (int xx = x + 1; xx < x + w + 1; xx++) {
    putChar(xx, y + h, ' ');
  }
}

void OSD::drawButtonShadow(int x, int y, int width, uint8_t bg)
{
  setColor(COL_BLACK, bg);

  putChar(x + width, y, (char) 220);

  for (int i = 0; i < width; i++) {
    putChar(x + 1 + i, y + 1, (char) 223);
  }
}

void OSD::drawVScroll(int x, int y, int visible, int total, int first)
{
  setColor(OSD_SCROLL_FG, OSD_SCROLL_BG);
  putChar(x, y, (char) 24);

  int area = visible - 2;
  if (area < 1)
    area = 1;

  int size = (visible * visible) / total;
  if (size < 1) {
    size = 1;
  } else if (size > area) {
    size = area;
  }

  int maxOff = area - size;
  int offset = (first * maxOff) / (total - visible);

  for (int i = 0; i < area; i++) {

    bool thumb = (i >= offset && i < offset + size);

    if (thumb) {
      setColor(OSD_SCROLL_THUMB_FG, OSD_SCROLL_THUMB_BG);
    } else {
      setColor(OSD_SCROLL_EMPTY_FG, OSD_SCROLL_EMPTY_BG);
    }
    putChar(x, y + 1 + i, thumb ? (char) 178 : (char) 176);
  }

  setColor(OSD_SCROLL_FG, OSD_SCROLL_BG);
  putChar(x, y + 1 + area, (char) 25);
}

void OSD::drawRadioGroupItem(int index, bool focused, uint8_t bg)
{
  RadioGroupItem &g = m_radioGroup[index];

  for (int i = 0; i < g.count; i++) {

    bool sel = (i == *g.current);

    setColor((focused && sel) ? OSD_RADIOLIST_SEL_FG : OSD_RADIOLIST_FG, bg);

    putTextRaw(g.x, g.y + i, sel ? "(*) " : "( ) ");
    putTextRaw(g.x + 4, g.y + i, g.items[i]);
  }
}

void OSD::frame(int x, int y, int width, int height,
                const char *title, bool centered,
                uint8_t fg, uint8_t bg,
                bool simpleBorder, bool shadow)
{
  drawBorder(x, y, width, height, simpleBorder, fg, bg);

  setColor(fg, bg);
  fillRect(x + 1, y + 1, width - 2, height - 2, ' ');

  if (title) {
    char b[140];
    int L = strlen(title);
    if (L > 120)
      L = 120;

    b[0] = ' ';
    memcpy(b + 1, title, L);
    b[L+1] = ' ';
    b[L+2] = 0;

    int pos = centered ? x + (width - (L+2)) / 2 : x + 1;
    putTextRaw(pos, y, b);
  }

  if (shadow)
    drawShadow(x, y, width, height);
}

void OSD::box(int x, int y, int width, int height,
              uint8_t fg, uint8_t bg, bool border)
{
  setColor(fg, bg);
  fillRect(x, y, width, height, ' ');

  if (border)
    drawBorder(x, y, width, height, true, fg, bg);
}

void OSD::text(int x, int y, const char *txt, uint8_t fg, uint8_t bg)
{
  setColor(fg, bg);

  int cx = x;
  int cy = y;

  while (*txt) {
    if (*txt == '\n') {
      cy++;
      cx = x;
      txt++;
      continue;
    }
    putChar(cx++, cy, *txt++);
  }
}

int OSD::menu(int x, int y, int width, const char *opts, uint8_t bg)
{
  int count = 1;
  for (const char *p = opts; *p; p++) {
    if (*p == ';')
      count++;
  }

  char **items = new char*[count];
  const char *start = opts;
  int idx = 0;

  for (const char *p = opts;; p++) {
    if ((*p == ';') || (*p == 0)) {
      int L = p - start;
      items[idx] = new char[L + 1];
      memcpy(items[idx], start, L);
      items[idx][L] = 0;
      idx++;
      if (!*p) break;
      start = p + 1;
    }
  }

  int sel = 0;

  while (true) {

    for (int i = 0; i < count; i++) {

      bool s = (i == sel);

      setColor(s ? OSD_BUTTON_SEL_FG : OSD_BUTTON_FG,
               s ? OSD_BUTTON_SEL_BG : OSD_BUTTON_BG);

      fillRect(x, y + i*2, width, 1, ' ');
      putTextRaw(x + 2, y + i*2, items[i]);

      drawButtonShadow(x, y + i*2, width, bg);
    }

    int key = readKey();

    switch (key) {
      case fabgl::VK_UP:
        if (sel > 0)
          sel--;
        break;

      case fabgl::VK_DOWN:
        if (sel < count - 1)
          sel++;
        break;

      case fabgl::VK_RETURN:
        for (int i = 0; i < count; i++)
          delete[] items[i];
        delete[] items;
        return sel;

      case fabgl::VK_ESCAPE:
        for (int i = 0; i < count; i++)
          delete[] items[i];
        delete[] items;
        return -1;
    }
  }
}

int OSD::menuBar(int x, int y, int fixWidth, const char *opts, uint8_t bg)
{
  int count = 1;
  for (const char *p = opts; *p; p++) {
    if (*p == ';')
      count++;
  }

  char **items = new char*[count];
  const char *start = opts;
  int idx = 0;

  for (const char *p = opts; ; p++) {
    if ((*p == ';') || (*p == 0)) {
      int L = p - start;
      items[idx] = new char[L + 1];
      memcpy(items[idx], start, L);
      items[idx][L] = 0;
      idx++;
      if (!*p)
        break;
      start = p + 1;
    }
  }

  int sel = 0;

  while (true) {
    int px = x;

    for (int i = 0; i < count; i++) {

      bool s = (i == sel);

      int bw = (fixWidth == 0)
                ? strlen(items[i]) + 4
                : fixWidth;

      setColor(s ? OSD_BUTTON_SEL_FG : OSD_BUTTON_FG,
               s ? OSD_BUTTON_SEL_BG : OSD_BUTTON_BG);

      fillRect(px, y, bw, 1, ' ');
      putTextRaw(px + 2, y, items[i]);

      drawButtonShadow(px, y, bw, bg);

      px += bw + 2;
    }

    int key = readKey();

    switch (key) {
      case fabgl::VK_LEFT:
        if (sel > 0)
          sel--;
        break;

      case fabgl::VK_RIGHT:
        if (sel < count - 1)
          sel++;
        break;

      case fabgl::VK_RETURN:
        for (int i = 0; i < count; i++)
          delete[] items[i];
        delete[] items;
        return sel;

      case fabgl::VK_ESCAPE:
        for (int i = 0; i < count; i++)
          delete[] items[i];
        delete[] items;
        return -1;
    }
  }
}

int OSD::radioList(int x, int y, const char *opts, int current, uint8_t bg)
{
  int count = 1;
  for (const char *p = opts; *p; p++) {
    if (*p == ';')
      count++;
  }

  char **items = new char*[count];
  const char *start = opts;
  int idx = 0;

  for (const char *p = opts;; p++) {
    if ((*p == ';') || (*p == 0)) {
      int L = p - start;
      items[idx] = new char[L + 1];
      memcpy(items[idx], start, L);
      items[idx][L] = 0;
      idx++;
      if (!*p)
        break;
      start = p + 1;
    }
  }

  int sel = current;
  if (sel < 0)
    sel = 0;
  if (sel >= count)
    sel = count - 1;

  while (true) {

    for (int i = 0; i < count; i++) {
      bool s = (i == sel);

      setColor(s ? OSD_RADIOLIST_SEL_FG : OSD_RADIOLIST_FG, bg);
      putTextRaw(x, y + i, s ? "(*) " : "( ) ");
      putTextRaw(x + 4, y + i, items[i]);
    }

    int key = readKey();

    switch (key) {

      case fabgl::VK_UP:
        if (sel > 0)
          sel--;
        break;

      case fabgl::VK_DOWN:
        if (sel < count - 1)
          sel++;
        break;

      case fabgl::VK_RETURN:
        for (int i = 0; i < count; i++)
          delete[] items[i];
        delete[] items;
        return sel;

      case fabgl::VK_ESCAPE:
        for (int i = 0; i < count; i++)
          delete[] items[i];
        delete[] items;
        return -1;
    }
  }
}

void OSD::radioGroupBegin()
{
  m_radioGroupCount = 0;
}

void OSD::radioGroupAdd(int x, int y, const char *opts, int *current)
{
  if (m_radioGroupCount >= OSD_MAX_RADIOGROUP_ITEMS)
    return;

  int count = 1;
  for (const char *p = opts; *p; p++) {
    if (*p == ';')
      count++;
  }

  char **items = new char*[count];

  const char *start = opts;
  int idx = 0;

  for (const char *p = opts;; p++) {
    if ((*p == ';') || (*p == 0)) {
      int L = p - start;
      char *s = new char[L + 1];
      memcpy(s, start, L);
      s[L] = 0;
      items[idx++] = s;
      if (!*p)
        break;
      start = p + 1;
    }
  }

  if (*current < 0)
    *current = 0;
  if (*current >= count)
    *current = count - 1;

  RadioGroupItem &g = m_radioGroup[m_radioGroupCount++];
  g.x = x;
  g.y = y;
  g.items = items;
  g.count = count;
  g.current = current;

  // Initial drawing (without focus)
  for (int i = 0; i < g.count; i++) {
    bool sel = (i == *g.current);
    setColor(OSD_RADIOLIST_FG, m_bg);
    putTextRaw(g.x, g.y + i, sel ? "(*) " : "( ) ");
    putTextRaw(g.x + 4, g.y + i, g.items[i]);
  }
}

int OSD::radioGroupRun(uint8_t bg)
{
  if (m_radioGroupCount == 0)
    return -1;

  int focused = 0;

  // Initial focus
  drawRadioGroupItem(focused, true, bg);

  bool finish = false;
  int retval;
  while (!finish) {

    int key = readKey();

    switch (key) {

      case fabgl::VK_UP:
        if (*m_radioGroup[focused].current > 0) {
          (*m_radioGroup[focused].current)--;
          drawRadioGroupItem(focused, true, bg);
        }
        break;

      case fabgl::VK_DOWN:
        if (*m_radioGroup[focused].current <
            m_radioGroup[focused].count - 1) {
          (*m_radioGroup[focused].current)++;
          drawRadioGroupItem(focused, true, bg);
        }
        break;

      case fabgl::VK_LEFT:
      case fabgl::VK_TAB:
        drawRadioGroupItem(focused, false, bg);
        focused--;
        if (focused < 0)
          focused = m_radioGroupCount - 1;
        drawRadioGroupItem(focused, true, bg);
        break;

      case fabgl::VK_RIGHT:
        drawRadioGroupItem(focused, false, bg);
        focused++;
        if (focused >= m_radioGroupCount)
          focused = 0;
        drawRadioGroupItem(focused, true, bg);
        break;

      case fabgl::VK_RETURN:
        finish = true;
        retval = focused;
        break;

      case fabgl::VK_ESCAPE:
        finish = true;
        retval = -1;
        break;

      default: // Nothing to do
        break;
    }
  }

  for (int g = 0; g < m_radioGroupCount; g++) {
    if (m_radioGroup[g].items) {
      for (int i = 0; i < m_radioGroup[g].count; i++) {
        delete[] m_radioGroup[g].items[i];
      }
      delete[] m_radioGroup[g].items;
      m_radioGroup[g].items = nullptr;
    }
  }
  m_radioGroupCount = 0;

  return retval;
}

int OSD::fileBrowser(int x, int y, int width, int height,
                     const char *path, char *filename)
{
  DIR *dir = opendir(path);
  if (!dir)
    return -1;

  struct dirent *e;
  int count = 0;

  while ((e=readdir(dir)) != NULL) {
    if (e->d_type == DT_REG)
      count++;
  }

  rewinddir(dir);

  char **items = new char*[count];
  int idx = 0;

  while ((e=readdir(dir)) != NULL) {
    if (e->d_type == DT_REG) {
      items[idx] = new char[strlen(e->d_name) + 1];
      strcpy(items[idx], e->d_name);
      idx++;
    }
  }

  closedir(dir);

  int sel = 0;
  int visible = height;
  if (visible < 1)
    visible = 1;

  int first = 0;

  int rowStart = y;
  int colStart = x;
  int fullW = width;
  int textX = x + 1;
  int textW = width - 2;
  int scrollX = x + width - 1;

  while (true) {
    for (int i = 0; i < visible; i++) {

      int id = first + i;
      int row = rowStart + i;

      if (id < count) {

        bool s = (id == sel);

        setColor(s ? OSD_FILE_SEL_FG : OSD_FILE_FG,
                 s ? OSD_FILE_SEL_BG : OSD_FILE_BG);

        fillRect(colStart, row, fullW, 1, ' ');

        const char *name = items[id];
        char buf[260];
        int L = strlen(name);
        if (L > textW)
          L = textW;
        memcpy(buf, name, L);
        buf[L] = 0;

        putTextRaw(textX, row, buf);
      }
      else {
        setColor(OSD_FILE_FG, OSD_FILE_BG);
        fillRect(colStart, row, fullW, 1, ' ');
      }
    }

    if (count > visible)
      drawVScroll(scrollX, y, visible, count, first);

    int key = readKey();

    switch (key) {
      case fabgl::VK_UP:
        if (sel > 0) {
          sel--;
          if (sel < first)
            first--;
        }
        break;

      case fabgl::VK_DOWN:
        if (sel < count-1) {
          sel++;
          if (sel >= first + visible)
            first++;
        }
        break;

      case fabgl::VK_HOME:
        sel = 0;
        first = 0;
        break;

      case fabgl::VK_END:
        sel = count - 1;
        first = sel - visible + 1;
        if (first < 0)
          first = 0;
        break;

      case fabgl::VK_PAGEUP:
        if (first > 0) {
          first -= visible;
          if (first < 0)
            first = 0;
          if (sel > first + visible - 1)
            sel = first + visible - 1;
        } else {
          sel = 0;
          first = 0;
        }
        break;

      case fabgl::VK_PAGEDOWN:
        if (first + visible < count) {
          first += visible;
          if (first > count - visible)
            first = count - visible;
          if (sel < first)
            sel = first;
        } else {
          sel = count - 1;
          first = count - visible;
          if (first < 0)
            first = 0;
        }
        break;

      case fabgl::VK_RETURN:
        strcpy(filename, items[sel]);
        for (int i = 0; i < count; i++)
          delete[] items[i];
        delete[] items;
        return sel;

      case fabgl::VK_ESCAPE:
        for (int i = 0; i < count; i++)
          delete[] items[i];
        delete[] items;
        return -1;
    }
  }
}

void OSD::progress(int x, int y, int width,
                   int percent, const char *txt, bool shadow)
{
  if (percent < 0) {
    percent = 0;
  } else if (percent > 100) {
    percent = 100;
  }

  const int done = (percent * width) / 100;

  setColor(OSD_PROGRESS_EMPTY_FG, OSD_PROGRESS_EMPTY_BG);
  fillRect(x, y, width, 1, ' ');

  setColor(OSD_PROGRESS_DONE_FG, OSD_PROGRESS_DONE_BG);
  fillRect(x, y, done, 1, ' ');

  if (txt) {
    char buf[260];
    int L = strlen(txt);
    if (L > width)
      L = width;
    memcpy(buf, txt, L);
    buf[L] = 0;

    int cx = x;
    for (int i = 0; i < L; i++) {
      bool insideDone = (i < done);

      if (insideDone)
        setColor(OSD_PROGRESS_DONE_FG, OSD_PROGRESS_DONE_BG);
      else
        setColor(OSD_PROGRESS_EMPTY_FG, OSD_PROGRESS_EMPTY_BG);

      putChar(cx++, y, buf[i]);
    }
  }

  if (shadow) {
    setColor(OSD_PROGRESS_SHADOW_FG, OSD_PROGRESS_SHADOW_BG);
    putChar(x + width, y, (char) 220);
    for (int i = 0; i < width; i++)
      putChar(x + 1 + i, y + 1, (char) 223);
  }
}
