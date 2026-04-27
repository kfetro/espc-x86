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

#pragma once

#include <stdint.h>

namespace fabgl {

/**
 * @brief Represents the coordinate of a point.
 *
 * Coordinates start from 0.
 */
struct Point {
  int16_t X;    /**< Horizontal coordinate */
  int16_t Y;    /**< Vertical coordinate */

  Point() : X(0), Y(0) { }
  Point(int X_, int Y_) : X(X_), Y(Y_) { }
  
  Point add(Point const & p) const { return Point(X + p.X, Y + p.Y); }
  Point sub(Point const & p) const { return Point(X - p.X, Y - p.Y); }
  Point neg() const                { return Point(-X, -Y); }
  bool operator==(Point const & r) { return X == r.X && Y == r.Y; }
  bool operator!=(Point const & r) { return X != r.X || Y != r.Y; }
} __attribute__ ((packed));

/**
 * @brief Represents a bidimensional size.
 */
struct Size {
  int16_t width;   /**< Horizontal size */
  int16_t height;  /**< Vertical size */

  Size() : width(0), height(0) { }
  Size(int width_, int height_) : width(width_), height(height_) { }
  bool operator==(Size const & r) { return width == r.width && height == r.height; }
  bool operator!=(Size const & r) { return width != r.width || height != r.height; }
} __attribute__ ((packed));

/**
 * @brief Represents a rectangle.
 *
 * Top and Left coordinates start from 0.
 */
struct Rect {
  int16_t X1;   /**< Horizontal top-left coordinate */
  int16_t Y1;   /**< Vertical top-left coordinate */
  int16_t X2;   /**< Horizontal bottom-right coordinate */
  int16_t Y2;   /**< Vertical bottom-right coordinate */

  Rect() : X1(0), Y1(0), X2(0), Y2(0) { }
  Rect(int X1_, int Y1_, int X2_, int Y2_) : X1(X1_), Y1(Y1_), X2(X2_), Y2(Y2_) { }
  Rect(Rect const & r) { X1 = r.X1; Y1 = r.Y1; X2 = r.X2; Y2 = r.Y2; }

  Rect& operator=(const Rect& r) {
    X1 = r.X1; Y1 = r.Y1; X2 = r.X2; Y2 = r.Y2;
    return *this;
  }

  bool operator==(Rect const & r)                { return X1 == r.X1 && Y1 == r.Y1 && X2 == r.X2 && Y2 == r.Y2; }
  bool operator!=(Rect const & r)                { return X1 != r.X1 || Y1 != r.Y1 || X2 != r.X2 || Y2 != r.Y2; }
  Point pos() const                              { return Point(X1, Y1); }
  Size size() const                              { return Size(X2 - X1 + 1, Y2 - Y1 + 1); }
  int width() const                              { return X2 - X1 + 1; }
  int height() const                             { return Y2 - Y1 + 1; }
  Rect translate(int offsetX, int offsetY) const { return Rect(X1 + offsetX, Y1 + offsetY, X2 + offsetX, Y2 + offsetY); }
  Rect translate(Point const & offset) const     { return Rect(X1 + offset.X, Y1 + offset.Y, X2 + offset.X, Y2 + offset.Y); }
  Rect move(Point const & position) const        { return Rect(position.X, position.Y, position.X + width() - 1, position.Y + height() - 1); }
  Rect move(int x, int y) const                  { return Rect(x, y, x + width() - 1, y + height() - 1); }
  Rect shrink(int value) const                   { return Rect(X1 + value, Y1 + value, X2 - value, Y2 - value); }
  Rect hShrink(int value) const                  { return Rect(X1 + value, Y1, X2 - value, Y2); }
  Rect vShrink(int value) const                  { return Rect(X1, Y1 + value, X2, Y2 - value); }
  Rect resize(int width, int height) const       { return Rect(X1, Y1, X1 + width - 1, Y1 + height - 1); }
  Rect resize(Size size) const                   { return Rect(X1, Y1, X1 + size.width - 1, Y1 + size.height - 1); }
  Rect intersection(Rect const & rect) const;
  bool intersects(Rect const & rect) const       { return X1 <= rect.X2 && X2 >= rect.X1 && Y1 <= rect.Y2 && Y2 >= rect.Y1; }
  bool contains(Rect const & rect) const         { return (rect.X1 >= X1) && (rect.Y1 >= Y1) && (rect.X2 <= X2) && (rect.Y2 <= Y2); }
  bool contains(Point const & point) const       { return point.X >= X1 && point.Y >= Y1 && point.X <= X2 && point.Y <= Y2; }
  bool contains(int x, int y) const              { return x >= X1 && y >= Y1 && x <= X2 && y <= Y2; }
  Rect merge(Rect const & rect) const;
} __attribute__ ((packed));

bool clipLine(int & x1, int & y1, int & x2, int & y2, Rect const & clipRect, bool checkOnly);

/*
template <typename T>
struct StackItem {
  StackItem * next;
  T           item;
  StackItem(StackItem * next_, T const & item_) : next(next_), item(item_) { }
};

template <typename T>
class Stack {
public:
  Stack() : m_items(nullptr) { }
  bool isEmpty() { return m_items == nullptr; }
  void push(T const & value) {
    m_items = new StackItem<T>(m_items, value);
  }
  T pop() {
    if (m_items) {
      StackItem<T> * iptr = m_items;
      m_items = iptr->next;
      T r = iptr->item;
      delete iptr;
      return r;
    } else
      return T();
  }
  int count() {
    int r = 0;
    for (auto i = m_items; i; i = i->next)
      ++r;
    return r;
  }
private:
  StackItem<T> * m_items;
};

void removeRectangle(Stack<Rect> & rects, Rect const & mainRect, Rect const & rectToRemove);
*/

// LightMemoryPool
// Each allocated block starts with a two bytes header (int16_t). Bit 15 is allocation flag (0=free, 1=allocated).
// Bits 14..0 represent the block size.
// The maximum size of a block is 32767 bytes.
// free() just marks the block header as free.

class LightMemoryPool {
public:
  LightMemoryPool(int poolSize);
  ~LightMemoryPool();
  void * alloc(int size);
  void free(void * mem) { if (mem) markFree((uint8_t*)mem - m_mem - 2); }

  bool memCheck();
  int totFree();      // get total free memory
  int totAllocated(); // get total allocated memory
  int largestFree();

private:

  void mark(int pos, int16_t size, bool allocated);
  void markFree(int pos) { m_mem[pos + 1] &= 0x7f; }
  int16_t getSize(int pos);
  bool isFree(int pos);

  uint8_t * m_mem;
  int       m_poolSize;
};

} // end of namespace
