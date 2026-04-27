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

/**
 * @file
 *
 * @brief This file contains codepages declarations
 *
 */

#include "keycodes.h"

// ASCII control characters
#define ASCII_NUL   0x00   // Null
#define ASCII_SOH   0x01   // Start of Heading
#define ASCII_CTRLA 0x01   // CTRL-A
#define ASCII_STX   0x02   // Start of Text
#define ASCII_CTRLB 0x02   // CTRL-B
#define ASCII_ETX   0x03   // End Of Text
#define ASCII_CTRLC 0x03   // CTRL-C
#define ASCII_EOT   0x04   // End Of Transmission
#define ASCII_CTRLD 0x04   // CTRL-D
#define ASCII_ENQ   0x05   // Enquiry
#define ASCII_CTRLE 0x05   // CTRL-E
#define ASCII_ACK   0x06   // Acknowledge
#define ASCII_CTRLF 0x06   // CTRL-F
#define ASCII_BEL   0x07   // Bell
#define ASCII_CTRLG 0x07   // CTRL-G
#define ASCII_BS    0x08   // Backspace
#define ASCII_CTRLH 0x08   // CTRL-H
#define ASCII_HT    0x09   // Horizontal Tab
#define ASCII_TAB   0x09   // Horizontal Tab
#define ASCII_CTRLI 0x09   // CTRL-I
#define ASCII_LF    0x0A   // Line Feed
#define ASCII_CTRLJ 0x0A   // CTRL-J
#define ASCII_VT    0x0B   // Vertical Tab
#define ASCII_CTRLK 0x0B   // CTRL-K
#define ASCII_FF    0x0C   // Form Feed
#define ASCII_CTRLL 0x0C   // CTRL-L
#define ASCII_CR    0x0D   // Carriage Return
#define ASCII_CTRLM 0x0D   // CTRL-M
#define ASCII_SO    0x0E   // Shift Out
#define ASCII_CTRLN 0x0E   // CTRL-N
#define ASCII_SI    0x0F   // Shift In
#define ASCII_CTRLO 0x0F   // CTRL-O
#define ASCII_DLE   0x10   // Data Link Escape
#define ASCII_CTRLP 0x10   // CTRL-P
#define ASCII_DC1   0x11   // Device Control 1
#define ASCII_CTRLQ 0x11   // CTRL-Q
#define ASCII_XON   0x11   // Transmission On
#define ASCII_DC2   0x12   // Device Control 2
#define ASCII_CTRLR 0x12   // CTRL-R
#define ASCII_DC3   0x13   // Device Control 3
#define ASCII_XOFF  0x13   // Transmission Off
#define ASCII_CTRLS 0x13   // CTRL-S
#define ASCII_DC4   0x14   // Device Control 4
#define ASCII_CTRLT 0x14   // CTRL-T
#define ASCII_NAK   0x15   // Negative Acknowledge
#define ASCII_CTRLU 0x15   // CTRL-U
#define ASCII_SYN   0x16   // Synchronous Idle
#define ASCII_CTRLV 0x16   // CTRL-V
#define ASCII_ETB   0x17   // End-of-Transmission-Block
#define ASCII_CTRLW 0x17   // CTRL-W
#define ASCII_CAN   0x18   // Cancel
#define ASCII_CTRLX 0x18   // CTRL-X
#define ASCII_EM    0x19   // End of Medium
#define ASCII_CTRLY 0x19   // CTRL-Y
#define ASCII_SUB   0x1A   // Substitute
#define ASCII_CTRLZ 0x1A   // CTRL-Z
#define ASCII_ESC   0x1B   // Escape
#define ASCII_FS    0x1C   // File Separator
#define ASCII_GS    0x1D   // Group Separator
#define ASCII_RS    0x1E   // Record Separator
#define ASCII_US    0x1F   // Unit Separator
#define ASCII_SPC   0x20   // Space
#define ASCII_DEL   0x7F   // Delete

namespace fabgl {

// associates virtual key to ASCII code
struct VirtualKeyToASCII {
  VirtualKey vk;
  uint8_t    ASCII;
};

struct CodePage {
  uint16_t                  codepage;
  const VirtualKeyToASCII * convTable;  // last item has vk = VK_NONE (ending marker)
};

extern const CodePage CodePage437;
extern const CodePage CodePage1252;


struct CodePages {
  static int count() { return 2; }
  static CodePage const * get(uint16_t codepage, CodePage const * defaultValue = &CodePage437) {
    static const CodePage * codepages[] = { &CodePage437, &CodePage1252 };
    for (int i = 0; i < sizeof(CodePage) / sizeof(CodePage*); ++i)
      if (codepages[i]->codepage == codepage)
        return codepages[i];
    return defaultValue;
  }
};

/**
 * @brief Converts virtual key item to ASCII.
 *
 * This method converts the specified virtual key to ASCII, if possible.<br>
 * For example VK_A is converted to 'A' (ASCII 0x41), CTRL  + VK_SPACE produces ASCII NUL (0x00), CTRL + letter produces
 * ASCII control codes from SOH (0x01) to SUB (0x1A), CTRL + VK_BACKSLASH produces ASCII FS (0x1C), CTRL + VK_QUESTION produces
 * ASCII US (0x1F), CTRL + VK_LEFTBRACKET produces ASCII ESC (0x1B), CTRL + VK_RIGHTBRACKET produces ASCII GS (0x1D),
 * CTRL + VK_TILDE produces ASCII RS (0x1E) and VK_SCROLLLOCK produces XON or XOFF.
 *
 * @param item The virtual key to convert.
 * @param codepage Codepage used to convert language specific characters.
 *
 * @return The ASCII code of virtual key or -1 if virtual key cannot be translated to ASCII.
 */
int virtualKeyToASCII(VirtualKeyItem const & item, CodePage const * codepage);

} // end of namespace
