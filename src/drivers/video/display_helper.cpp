/*
 * Based on FabGL - ESP32 Graphics Library
 * Original project by Fabrizio Di Vittorio
 * https://github.com/fdivitto/FabGL
 *
 * Original Copyright (c) 2019-2022 Fabrizio Di Vittorio
 *
 * Modifications and further development:
 * Copyright (c) 2026 Jesus Martinez-Mateo
 * Author: Jesus Martinez-Mateo <jesus.martinez.mateo@gmail.com>
 *
 * This file is part of a derived work from FabGL
 * and is distributed under the terms of the
 * GNU General Public License version 3 or later.
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

#include "drivers/video/display_helper.h"
#include "drivers/hal_math.h"

#include "esp_heap_caps.h"

namespace fabgl {

// Rect
Rect Rect::merge(Rect const & rect) const
{
  return Rect(imin(rect.X1, X1), imin(rect.Y1, Y1), imax(rect.X2, X2), imax(rect.Y2, Y2));
}

Rect Rect::intersection(Rect const & rect) const
{
  return Rect(tmax(X1, rect.X1), tmax(Y1, rect.Y1), tmin(X2, rect.X2), tmin(Y2, rect.Y2));
}

// Sutherland-Cohen line clipping algorithm
static int clipLine_code(int x, int y, Rect const & clipRect)
{
  int code = 0;
  if (x < clipRect.X1)
    code = 1;
  else if (x > clipRect.X2)
    code = 2;
  if (y < clipRect.Y1)
    code |= 4;
  else if (y > clipRect.Y2)
    code |= 8;
  return code;
}

// false = line is out of clipping rect
// true = line intersects or is inside the clipping rect (x1, y1, x2, y2 are changed if checkOnly=false)
bool clipLine(int & x1, int & y1, int & x2, int & y2, Rect const & clipRect, bool checkOnly)
{
  int newX1 = x1;
  int newY1 = y1;
  int newX2 = x2;
  int newY2 = y2;
  int topLeftCode     = clipLine_code(newX1, newY1, clipRect);
  int bottomRightCode = clipLine_code(newX2, newY2, clipRect);
  while (true) {
    if ((topLeftCode == 0) && (bottomRightCode == 0)) {
      if (!checkOnly) {
        x1 = newX1;
        y1 = newY1;
        x2 = newX2;
        y2 = newY2;
      }
      return true;
    } else if (topLeftCode & bottomRightCode) {
      break;
    } else {
      int x = 0, y = 0;
      int ncode = topLeftCode != 0 ? topLeftCode : bottomRightCode;
      if (ncode & 8) {
        x = newX1 + (newX2 - newX1) * (clipRect.Y2 - newY1) / (newY2 - newY1);
        y = clipRect.Y2;
      } else if (ncode & 4) {
        x = newX1 + (newX2 - newX1) * (clipRect.Y1 - newY1) / (newY2 - newY1);
        y = clipRect.Y1;
      } else if (ncode & 2) {
        y = newY1 + (newY2 - newY1) * (clipRect.X2 - newX1) / (newX2 - newX1);
        x = clipRect.X2;
      } else if (ncode & 1) {
        y = newY1 + (newY2 - newY1) * (clipRect.X1 - newX1) / (newX2 - newX1);
        x = clipRect.X1;
      }
      if (ncode == topLeftCode) {
        newX1 = x;
        newY1 = y;
        topLeftCode = clipLine_code(newX1, newY1, clipRect);
      } else {
        newX2 = x;
        newY2 = y;
        bottomRightCode = clipLine_code(newX2, newY2, clipRect);
      }
    }
  }
  return false;
}

/*
// remove "rectToRemove" from "mainRect", pushing remaining rectangles to "rects" stack
void removeRectangle(Stack<Rect> & rects, Rect const & mainRect, Rect const & rectToRemove)
{
  if (!mainRect.intersects(rectToRemove) || rectToRemove.contains(mainRect))
    return;

  // top rectangle
  if (mainRect.Y1 < rectToRemove.Y1)
    rects.push(Rect(mainRect.X1, mainRect.Y1, mainRect.X2, rectToRemove.Y1 - 1));

  // bottom rectangle
  if (mainRect.Y2 > rectToRemove.Y2)
    rects.push(Rect(mainRect.X1, rectToRemove.Y2 + 1, mainRect.X2, mainRect.Y2));

  // left rectangle
  if (mainRect.X1 < rectToRemove.X1)
    rects.push(Rect(mainRect.X1, tmax(rectToRemove.Y1, mainRect.Y1), rectToRemove.X1 - 1, tmin(rectToRemove.Y2, mainRect.Y2)));

  // right rectangle
  if (mainRect.X2 > rectToRemove.X2)
    rects.push(Rect(rectToRemove.X2 + 1, tmax(rectToRemove.Y1, mainRect.Y1), mainRect.X2, tmin(rectToRemove.Y2, mainRect.Y2)));
}
*/

// LightMemoryPool

void LightMemoryPool::mark(int pos, int16_t size, bool allocated)
{
  m_mem[pos]     = size & 0xff;
  m_mem[pos + 1] = ((size >> 8) & 0x7f) | (allocated ? 0x80 : 0);
}

int16_t LightMemoryPool::getSize(int pos)
{
  return m_mem[pos] | ((m_mem[pos + 1] & 0x7f) << 8);
}


bool LightMemoryPool::isFree(int pos)
{
  return (m_mem[pos + 1] & 0x80) == 0;
}

LightMemoryPool::LightMemoryPool(int poolSize)
{
  m_poolSize = poolSize + 2;
  m_mem = (uint8_t*) heap_caps_malloc(m_poolSize, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
  mark(0, m_poolSize - 2, false);
}

LightMemoryPool::~LightMemoryPool()
{
  heap_caps_free(m_mem);
}

void * LightMemoryPool::alloc(int size)
{
  for (int pos = 0; pos < m_poolSize; ) {
    int16_t blockSize = getSize(pos);
    if (isFree(pos)) {
      if (blockSize == size) {
        // found a block having the same size
        mark(pos, size, true);
        return m_mem + pos + 2;
      } else if (blockSize > size) {
        // found a block having larger size
        int remainingSize = blockSize - size - 2;
        if (remainingSize > 0)
          mark(pos + 2 + size, remainingSize, false);  // create new free block at the end of this block
        else
          size = blockSize; // to avoid to waste last block
        mark(pos, size, true);  // reduce size of this block and mark as allocated
        return m_mem + pos + 2;
      } else {
        // this block hasn't enough space
        // can merge with next block?
        int nextBlockPos = pos + 2 + blockSize;
        if (nextBlockPos < m_poolSize && isFree(nextBlockPos)) {
          // join blocks and stay at this pos
          mark(pos, blockSize + getSize(nextBlockPos) + 2, false);
        } else {
          // move to the next block
          pos += blockSize + 2;
        }
      }
    } else {
      // move to the next block
      pos += blockSize + 2;
    }
  }
  return nullptr;
}

bool LightMemoryPool::memCheck()
{
  int pos = 0;
  while (pos < m_poolSize) {
    int16_t blockSize = getSize(pos);
    pos += blockSize + 2;
  }
  return pos == m_poolSize;
}

int LightMemoryPool::totFree()
{
  int r = 0;
  for (int pos = 0; pos < m_poolSize; ) {
    int16_t blockSize = getSize(pos);
    if (isFree(pos))
      r += blockSize;
    pos += blockSize + 2;
  }
  return r;
}

int LightMemoryPool::totAllocated()
{
  int r = 0;
  for (int pos = 0; pos < m_poolSize; ) {
    int16_t blockSize = getSize(pos);
    if (!isFree(pos))
      r += blockSize;
    pos += blockSize + 2;
  }
  return r;
}

int LightMemoryPool::largestFree()
{
  int r = 0;
  for (int pos = 0; pos < m_poolSize; ) {
    int16_t blockSize = getSize(pos);
    if (isFree(pos) && blockSize > r)
      r = blockSize;
    pos += blockSize + 2;
  }
  return r;
}

} // end of namespace
