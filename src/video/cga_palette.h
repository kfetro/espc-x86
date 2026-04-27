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

// CGA master/full palette
static const RGB222 CGA_palette[16] = {
  RGB222(0, 0, 0), // black
  RGB222(0, 0, 2), // blue
  RGB222(0, 2, 0), // green
  RGB222(0, 2, 2), // cyan
  RGB222(2, 0, 0), // red
  RGB222(2, 0, 2), // magenta
  RGB222(2, 1, 0), // brown
  RGB222(2, 2, 2), // light gray
  RGB222(1, 1, 1), // dark gray
  RGB222(1, 1, 3), // light blue
  RGB222(1, 3, 1), // light green
  RGB222(1, 3, 3), // light cyan
  RGB222(3, 1, 1), // light red
  RGB222(3, 1, 3), // light magenta
  RGB222(3, 3, 1), // yellow
  RGB222(3, 3, 3), // white
};

// Standard CGA palette mappings
static const RGB222 CGA_paletteMap[4][4] = {
  // Low intensity
  {
    // Real background comes from Color Select bits 0–3
    RGB222(0, 0, 0), // background (not used)
    RGB222(0, 2, 0), // green
    RGB222(2, 0, 0), // red
    RGB222(2, 1, 0), // brown
  },
  // High intensity
  {
    RGB222(0, 0, 0), // background (not used)
    RGB222(1, 3, 1), // light green
    RGB222(3, 1, 1), // light red
    RGB222(3, 3, 1), // yellow
  },
  // Low intensity (alternate palette)
  {
    RGB222(0, 0, 0), // background (not used)
    RGB222(0, 2, 2), // cyan
    RGB222(2, 0, 2), // magenta
    RGB222(2, 2, 2), // light gray
  },
  // High intensity (alternate palette)
  {
    RGB222(0, 0, 0), // background (not used)
    RGB222(1, 3, 3), // light cyan
    RGB222(3, 1, 3), // light magenta
    RGB222(3, 3, 3), // white
  },
};

} // end of namespace
