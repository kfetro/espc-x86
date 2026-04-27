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

#include "video/video_adapter.h"
#include "video/scanout_context.h"

#include <stdint.h>
#include <string.h>

// Hercules / MDA Video Memory Layout

// Full 64 KB VRAM window (B0000h–BFFFFh)
#define HGC_VRAM_SIZE      65536

#define HGC_VRAM_BASE      0xB0000
#define HGC_VRAM_LIMIT     0xBFFFF

// Two 32 KB pages (only one used at a time for graphics)
#define HGC_PAGE_SIZE      0x8000
#define HGC_PAGE0_OFFSET   0x0000
#define HGC_PAGE1_OFFSET   0x8000

// Hercules / MDA I/O Ports
#define HGC_PORT_CRTC_INDEX    0x03B4
#define HGC_PORT_CRTC_DATA     0x03B5
#define HGC_PORT_MODECTL       0x03B8
#define HGC_PORT_STATUS        0x03BA  // Read-only
#define HGC_PORT_LPT_DATA      0x03BC  // Printer Data
#define HGC_PORT_LPT_STATUS    0x03BD  // Printer Status (Read-only)
#define HGC_PORT_LPT_CONTROL   0x03BE  // Printer Control
#define HGC_PORT_CONF_SWITCH   0x03BF  // Configuration Switch (Hercules only)

// CRTC (Cathode Ray Tube Controller) Registers
#define HGC_CRTC_CURSORSTART   0x0A
#define HGC_CRTC_CURSOREND     0x0B
#define HGC_CRTC_STARTADDR_HI  0x0C
#define HGC_CRTC_STARTADDR_LO  0x0D
#define HGC_CRTC_CURSORPOS_HI  0x0E
#define HGC_CRTC_CURSORPOS_LO  0x0F

// --- Mode Control Register ---
// Note that the original MDA do not have graphics
#define HGC_MC_TEXT_MODE_B     0x01
#define HGC_MC_GRAPHICS_MODE   0x02
#define HGC_MC_BLINK_ENABLE    0x04
#define HGC_MC_VIDEO_ENABLE    0x08
// Use page 1 in 0xB8000 for graphics
#define HGC_MC_PAGE1           0x80

// Configuration Switch
#define HGC_CS_ALLOW_GRAPHICS  0x01    // 1 = permite entrar en modo gráfico
#define HGC_CS_ALLOW_PAGE1     0x02    // 1 = permite usar la página 1

using fabgl::RGB222;

namespace video {

class VideoScanout;

// Hercules (HGC) and MDA (Monochrome Display Adapter) video cards emulation
class HGC : public VideoAdapter,
            public ScanoutContext {

public:

   HGC();
  ~HGC();

  void init(uint8_t *ram, VideoScanout *video);
  void reset();

  // BIOS Video Interrupt (INT 10h)
  void handleInt10h();

  // I/O Ports
  uint8_t readPort(uint16_t port);
  void    writePort(uint16_t port, uint8_t value);

  // Mapped Memory (B0000h–BFFFFh)
  uint8_t  readMem8(uint32_t physAddr);
  uint16_t readMem16(uint32_t physAddr);
  void writeMem8(uint32_t physAddr, uint8_t value);
  void writeMem16(uint32_t physAddr, uint16_t value);

  // Scanout Context
  uint8_t *vram() override { return m_vram; }
  size_t vramSize() override { return HGC_VRAM_SIZE; }

  uint32_t startAddress() override { return (uint32_t) m_startAddress; }
  uint16_t textPageSize() override { return m_textPageSize; }
  bool cursorEnabled() override { return !m_cursorDisable; }
  uint8_t cursorStart() override { return m_cursorStart; }
  uint8_t cursorEnd() override { return m_cursorEnd; }
  uint8_t activePage() override { return m_activePage; }
  uint8_t cursorRow(uint8_t page) override { return m_cursorRow[page]; }
  uint8_t cursorCol(uint8_t page) override { return m_cursorCol[page]; }
  RGB222 paletteMap(uint8_t index, uint8_t total) override {
    return (index == 0) ? RGB222(0, 0, 0) : RGB222(3, 3, 3);
  }
  bool blinkEnabled() override { return isBit7Blinking(); }
  uint8_t colorPlaneEnable() override { return 1; }

  uint32_t renderStamp() { return m_stamp; }

private:

  // --- External ---
  uint8_t *s_ram;  // Main Memory (needed to update BDA)

  VideoScanout *m_video; // Renderer

  // --- Internal ---
  uint8_t *m_vram; // Video Memory

  // CRTC Registers (Motorola 6845)
  uint8_t m_crtc[0x20];
  uint8_t m_crtcIndex;

  // HGC Registers
  uint8_t m_modeControl;
  uint8_t m_lptData;     // Printer data
  uint8_t m_lptControl;  // Printer control
  uint8_t m_confSwitch;
  // State
  uint16_t m_statusQuery;

  uint16_t m_startAddress; // VRAM start offset (in words)

  bool m_cursorDisable;

  // Cursor Shape
  uint8_t  m_cursorStart;
  uint8_t  m_cursorEnd;

  uint8_t m_activePage;

  // Cursor Position
  uint8_t m_cursorRow[8];
  uint8_t m_cursorCol[8];

  uint16_t m_currentMode;

  // Text mode dimensions
  uint8_t  m_textRows;
  uint8_t  m_textCols;
  uint16_t m_textPageSize; // in bytes

  const uint8_t m_textAttr;

  bool m_dirty;
  uint32_t m_stamp;

  void initRegisters();

  void setMode(uint8_t mode);

  inline bool isVideoEnabled() const { return (m_modeControl & HGC_MC_VIDEO_ENABLE) != 0; }
  inline bool isBit7Blinking() const { return (m_modeControl & HGC_MC_BLINK_ENABLE) != 0; }
  inline bool isGraphicsMode() const { return (m_modeControl & HGC_MC_GRAPHICS_MODE) != 0; }
  inline bool isPage1Active()  const { return (m_modeControl & HGC_MC_PAGE1) != 0; }

  inline bool allowGraphics() const { return (m_confSwitch & HGC_CS_ALLOW_GRAPHICS) != 0; }
  inline bool allowPage1()    const { return (m_confSwitch & HGC_CS_ALLOW_PAGE1) != 0; }

  inline uint8_t *activePageBase() const {
    return m_vram + (isPage1Active() && allowPage1() ? HGC_PAGE1_OFFSET : HGC_PAGE0_OFFSET);
    // Nota: la página activa aplica a la salida de vídeo en modo GRÁFICO.
    // La CPU sigue pudiendo leer/escribir todo B0000h–BFFFFh.
  }

  // Escritura de píxel en 720×348 (interleaving 4×0x2000)
  void writePixelHGC_720(uint16_t x, uint16_t y, bool on);

  void writeCharAttr(uint8_t ch, uint8_t attr, uint16_t count, uint8_t page);
  void writeCharOnly(uint8_t ch, uint16_t count, uint8_t page);

  void scrollUpWindow(uint8_t lines, uint8_t attr, uint8_t top, uint8_t left, uint8_t bottom, uint8_t right);
  void scrollDownWindow(uint8_t lines, uint8_t attr, uint8_t top, uint8_t left, uint8_t bottom, uint8_t right);

  // Writes a single character using teletype rules (advance, wrap, scroll)
  void ttyOutput(uint8_t ch, uint8_t page, uint8_t color);

  void writeString(uint8_t flags, uint8_t page, uint8_t attr,
                   uint8_t row, uint8_t col,
                   uint16_t ES, uint16_t BP, uint16_t len);

  void newLine(uint8_t page);

  // Scrolls the specified text page up by 1 line and clears the last line
  void scrollUp(uint8_t page);

  // Computes linear byte offset (within VRAM window) for a text cell
  inline uint32_t textCellOffset(uint8_t page, uint8_t row, uint8_t col) const {
    return (uint32_t) page * m_textPageSize + ((uint32_t) row * m_textCols + col) * 2;
  }

  void clearScreen();

  // Update BIOS Data Area (BDA)
  void syncBDA();

  // Syncs the cursor position with CRTC registers
  // and update BDA
  void syncCursorPos();
};

} // end of namespace
