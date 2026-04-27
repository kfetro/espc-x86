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

namespace video {

class VideoScanout;

// Common inferface for CGA/EGA/HGC
class VideoAdapter {

public:

  virtual ~VideoAdapter() {}

  virtual void init(uint8_t *ram, VideoScanout *video) = 0;
  virtual void reset() = 0;

  // BIOS Video Interrupt (INT 10h)
  virtual void handleInt10h() = 0;

  // I/O ports
  virtual uint8_t readPort(uint16_t port) = 0;
  virtual void writePort(uint16_t port, uint8_t value) = 0;

  // Mapped memory
  virtual uint8_t  readMem8(uint32_t physAddr) = 0;
  virtual uint16_t readMem16(uint32_t physAddr) = 0;
  virtual void writeMem8(uint32_t physAddr, uint8_t value) = 0;
  virtual void writeMem16(uint32_t physAddr, uint16_t value) = 0;
};

} // end of namespace
