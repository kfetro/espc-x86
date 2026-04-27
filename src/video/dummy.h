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

#include "video/cga_palette.h"

#include "video/scanout_context.h"
#include "video/video_scanout.h"

#include "esp_heap_caps.h"

#include <stdint.h>
#include <string.h>

// Video Memory
#define DUMMY_VRAM_SIZE 16384

namespace video {

class DummyVideoCard : public ScanoutContext {

public:

  DummyVideoCard() :
  	m_video(nullptr),
    m_vram(nullptr),
    m_stamp(0)
  {
  }

  ~DummyVideoCard() {
  	if (m_vram) {
      heap_caps_free((void*) m_vram);
      m_vram = nullptr;
    }
  }

  void init(uint8_t *ram, VideoScanout *video) {
  	// External video scanout
    m_video = video;

    // Internal video memory
    m_vram = (uint8_t*) heap_caps_malloc(DUMMY_VRAM_SIZE, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    if (!m_vram) {
      printf("dummy: Not enough DRAM!!\n");
    } else {
      // Clear video memory
      memset(m_vram, 0, DUMMY_VRAM_SIZE);
	}
  }

  uint8_t *vram() override { return m_vram; }
  size_t vramSize() override { return DUMMY_VRAM_SIZE; }

  uint32_t startAddress() override { return 0; }
  uint16_t textPageSize() override { return 0; }
  bool cursorEnabled() override { return false; }
  uint8_t cursorStart() override { return 0; }
  uint8_t cursorEnd() override { return 0; }
  uint8_t activePage() override { return 0; }
  uint8_t cursorRow(uint8_t page) override { return 0; }
  uint8_t cursorCol(uint8_t page) override { return 0; }
  RGB222 paletteMap(uint8_t index, uint8_t total) override { return CGA_palette[index]; }
  bool blinkEnabled() override { return false; }
  uint8_t colorPlaneEnable() override { return 0; }

  uint32_t renderStamp() { return m_stamp; }

private:

  // --- External ---
  VideoScanout *m_video; // Renderer

  // --- Internal ---
  uint8_t *m_vram; // Video Memory

  uint32_t m_stamp;
};

} // end of namespace
