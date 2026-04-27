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

// Returns the greater of two values of any comparable type T
template <typename T>
const T & tmax(const T & a, const T & b)
{
  return (a < b) ? b : a;
}

// Returns the lesser of two values
template <typename T>
const T & tmin(const T & a, const T & b)
{
  return !(b < a) ? a : b;
}

// Constrains an value within a fixed range [lo, hi]
template <typename T>
const T & tclamp(const T & v, const T & lo, const T & hi)
{
  return (v < lo ? lo : (v > hi ? hi : v));
}

// Cycles a value to the opposite boundary of a range if it exceeds the specified limits
template <typename T>
const T & twrap(const T & v, const T & lo, const T & hi)
{
  return (v < lo ? hi : (v > hi ? lo : v));
}

// Exchanges the values of two variables
template <typename T>
void tswap(T & v1, T & v2)
{
  T t = v1;
  v1 = v2;
  v2 = t;
}

// Returns the greater of two integer values
constexpr auto imax = tmax<int>;

// Returns the lesser of two integer values
constexpr auto imin = tmin<int>;

// Constrains an integer within a fixed range [lo, hi]
constexpr auto iclamp = tclamp<int>;

// Cycles an integer to the opposite boundary if it exceeds the specified range
constexpr auto iwrap = twrap<int>;

// Exchanges the values of two integer variables
constexpr auto iswap = tswap<int>;
