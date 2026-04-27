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

#include "drivers/video/display.h"

using fabgl::RGB222;

namespace video {

// IBM EGA 64-color master palette (RGB222).
// Index bit order: Red-L, Green-L, Blue-L, Red-H, Green-H, Blue-H
// Channel value = L + 2*H (each in {0,1}) -> RGB222 components in {0,1,2,3}.
// Reference for EGA bit layout/order: ModdingWiki EGA Palette.
static const RGB222 EGA_palette[64] = {
  RGB222(0,0,0),  //  0: black (R0 G0 B0)
  RGB222(0,0,2),  //  1: medium blue (R0 G0 B2)
  RGB222(0,2,0),  //  2: medium green (R0 G2 B0)
  RGB222(0,2,2),  //  3: medium cyan (R0 G2 B2)
  RGB222(2,0,0),  //  4: medium red (R2 G0 B0)
  RGB222(2,0,2),  //  5: medium magenta (R2 G0 B2)
  RGB222(2,2,0),  //  6: medium yellow (R2 G2 B0)
  RGB222(2,2,2),  //  7: light gray (R2 G2 B2)

  RGB222(0,0,1),  //  8: low blue / navy (R0 G0 B1)
  RGB222(0,0,3),  //  9: bright blue (R0 G0 B3)
  RGB222(0,2,1),  // 10: teal-ish mix (R0 G2 B1)
  RGB222(0,2,3),  // 11: cyan (high blue) (R0 G2 B3)
  RGB222(2,0,1),  // 12: purple-ish (R2 G0 B1)
  RGB222(2,0,3),  // 13: magenta (high blue) (R2 G0 B3)
  RGB222(2,2,1),  // 14: olive-ish mix (R2 G2 B1)
  RGB222(2,2,3),  // 15: bluish light gray (R2 G2 B3)

  RGB222(0,1,0),  // 16: low green / dark green (R0 G1 B0)
  RGB222(0,1,2),  // 17: blue-green (low green) (R0 G1 B2)
  RGB222(0,3,0),  // 18: bright green (R0 G3 B0)
  RGB222(0,3,2),  // 19: cyan (high green) (R0 G3 B2)
  RGB222(2,1,0),  // 20: olive-brown mix (R2 G1 B0)
  RGB222(2,1,2),  // 21: desaturated magenta mix (R2 G1 B2)
  RGB222(2,3,0),  // 22: chartreuse (R2 G3 B0)
  RGB222(2,3,2),  // 23: spring green (R2 G3 B2)

  RGB222(0,1,1),  // 24: low cyan / dark teal (R0 G1 B1)
  RGB222(0,1,3),  // 25: blue-green (high blue) (R0 G1 B3)
  RGB222(0,3,1),  // 26: lime-teal mix (R0 G3 B1)
  RGB222(0,3,3),  // 27: bright cyan (R0 G3 B3)
  RGB222(2,1,1),  // 28: muted red-cyan mix (R2 G1 B1)
  RGB222(2,1,3),  // 29: violet-blue (R2 G1 B3)
  RGB222(2,3,1),  // 30: yellow-green (R2 G3 B1)
  RGB222(2,3,3),  // 31: aquamarine (R2 G3 B3)

  RGB222(1,0,0),  // 32: low red / maroon (R1 G0 B0)
  RGB222(1,0,2),  // 33: purple (R1 G0 B2)
  RGB222(1,2,0),  // 34: olive (R1 G2 B0)
  RGB222(1,2,2),  // 35: gray-cyan mix (R1 G2 B2)
  RGB222(3,0,0),  // 36: bright red (R3 G0 B0)
  RGB222(3,0,2),  // 37: magenta (R3 G0 B2)
  RGB222(3,2,0),  // 38: orange (R3 G2 B0)
  RGB222(3,2,2),  // 39: salmon / light red (R3 G2 B2)

  RGB222(1,0,1),  // 40: low magenta / dark magenta (R1 G0 B1)
  RGB222(1,0,3),  // 41: violet (R1 G0 B3)
  RGB222(1,2,1),  // 42: muted teal mix (R1 G2 B1)
  RGB222(1,2,3),  // 43: cyan-purple mix (R1 G2 B3)
  RGB222(3,0,1),  // 44: rose (R3 G0 B1)
  RGB222(3,0,3),  // 45: bright magenta (R3 G0 B3)
  RGB222(3,2,1),  // 46: ochre (R3 G2 B1)
  RGB222(3,2,3),  // 47: pinkish lavender (R3 G2 B3)

  RGB222(1,1,0),  // 48: olive (low yellow) (R1 G1 B0)
  RGB222(1,1,2),  // 49: slate / cadet (R1 G1 B2)
  RGB222(1,3,0),  // 50: lime (R1 G3 B0)
  RGB222(1,3,2),  // 51: spring green (R1 G3 B2)
  RGB222(3,1,0),  // 52: orange-red (R3 G1 B0)
  RGB222(3,1,2),  // 53: mauve / violet mix (R3 G1 B2)
  RGB222(3,3,0),  // 54: bright yellow (R3 G3 B0)
  RGB222(3,3,2),  // 55: khaki (R3 G3 B2)

  RGB222(1,1,1),  // 56: dark gray / bright black (R1 G1 B1)
  RGB222(1,1,3),  // 57: steel blue gray (R1 G1 B3)
  RGB222(1,3,1),  // 58: mint (R1 G3 B1)
  RGB222(1,3,3),  // 59: aqua / light cyan (R1 G3 B3)
  RGB222(3,1,1),  // 60: pink (R3 G1 B1)
  RGB222(3,1,3),  // 61: magenta-pink (R3 G1 B3)
  RGB222(3,3,1),  // 62: cream (R3 G3 B1)
  RGB222(3,3,3),  // 63: white / bright white (R3 G3 B3)
};

// Default 16-color attribute map within the 64-color EGA gamut.
// Order matches the classic CGA/EGA default set (incl. brown at index 20).
static uint8_t EGA_paletteMap[16] = {
  0, 1, 2, 3, 4, 5, 20, 7, 56, 57, 58, 59, 60, 61, 62, 63
};

} // end of namespace
