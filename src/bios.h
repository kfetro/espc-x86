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

#include "core/i8042.h"
#include "core/mc146818.h"
#include "core/i8255_ppi.h"

// XT (IBM 5150/5160)
// AT (IBM 5170)

// It uses Intel 8255 PPI (Programmable Peripheral Interface)
// instead of Intel 8044 (Keyboard Controller)
#define LEGACY_IBMPC_XT_8088  0

#define USE_SOUNDBLASTER   0

// 0 = floppy 0 (fd0, A:)
// 1 = floppy 1 (fd1, B:)
// 2 = hard disk 0 (hd0, C: or D:, depends by partitions)
// 3 = hard disk 1 (hd1)
constexpr int DISKCOUNT = 4;

#define PIT_TICK_FREQ 1193182

#define BIOS_SEG             0xF000
#define BIOS_OFF             0x0100
#define BIOS_ADDR            (BIOS_SEG * 16 + BIOS_OFF)

// BIOS Data Area

#define BIOS_BDA_SEG      0x40
#define BIOS_BDA_ADDR     (BIOS_BDA_SEG << 4)

#define BIOS_BDA_EQUIPMENT  0x0410 // Equipment word

#define BIOS_KBDSHIFTFLAGS1    0x17     // keyboard shift flags
#define BIOS_KBDSHIFTFLAGS2    0x18     // more keyboard shift flags
#define BIOS_KBDALTKEYPADENTRY 0x19     // Storage for alternate keypad entry
#define BIOS_KBDBUFHEAD        0x1a     // pointer to next character in keyboard buffer
#define BIOS_KBDBUFTAIL        0x1c     // pointer to first available spot in keyboard buffer
#define BIOS_KBDBUF            0x1e     // keyboard buffer (32 bytes, 16 keys, but actually 15)
#define BIOS_DISKLASTSTATUS    0x41     // diskette status return code
#define BIOS_SYSTICKS          0x6c     // system ticks (dword)
#define BIOS_CLKROLLOVER       0x70     // system tick rollover flag (24h)
#define BIOS_CTRLBREAKFLAG     0x71     // Ctrl-Break flag
#define BIOS_HDLASTSTATUS      0x74     // HD status return code
#define BIOS_NUMHD             0x75     // number of fixed disk drives
#define BIOS_DRIVE0MEDIATYPE   0x90     // media type of drive 0
#define BIOS_DRIVE1MEDIATYPE   0x91     // media type of drive 1
#define BIOS_KBDMODE           0x96     // keyboard mode and other shift flags
#define BIOS_KBDLEDS           0x97     // keyboard LEDs
#define BIOS_PRINTSCREENFLAG   0x100    // PRINTSCREEN flag

// Extended BIOS Data Area (EBDA)

#define EBDA_SEG               0x9fc0   // EBDA Segment, must match with same value in bios.asm
#define EBDA_ADDR              (EBDA_SEG << 4)

#define EBDA_DRIVER_OFFSET     0x22     // Pointing device device driver far call offset
#define EBDA_DRIVER_SEG        0x24     // Pointing device device driver far call segment
#define EBDA_FLAGS1            0x26     // Flags 1 (bits 0-2: recv data index)
#define EBDA_FLAGS2            0x27     // Flags 2 (bits 0-2: packet size, bit 7: device handler installed)
#define EBDA_PACKET            0x28     // Start of packet

using fabgl::PS2Controller;
using fabgl::Keyboard;
using fabgl::Mouse;
using core::i8255;
using core::i8042;
using core::MC146818;

enum MediaType {
  mediaUnknown,
  floppy160KB,
  floppy180KB,
  floppy320KB,
  floppy360KB,
  floppy720KB,
  floppy1M2K,
  floppy1M44K,
  floppy2M88K,

  HDD,
};

class Computer;

class BIOS {

public:

  BIOS();

  void init(Computer *computer);
  void reset();

  void handleInt11h();
  void handleInt12h();
  void handleInt13h();
  void handleInt14h();
  void handleInt15h();
  void handleInt16h();
  void handleInt17h();
  bool handleInt19h();
  void handleInt1Ah();

  void setDriveMediaType(int drive, MediaType media);

private:

  Computer  *m_computer;
  uint8_t   *m_memory;
  Keyboard  *m_keyboard;
  Mouse     *m_mouse;
#if LEGACY_IBMPC_XT_8088
  i8255     *m_i8255;
#else
  i8042     *m_i8042;
#endif
  MC146818  *m_MC146818;

  // address of bios.asm:media_drive_parameters (0, 1)
  // address of int41 (2), int46 (3)
  //uint32_t   m_mediaDriveParametersAddr[4];

  // media type for floppy (0,1) and HD (>=2)
  MediaType  m_mediaType[DISKCOUNT];

  //  void syncTicksWithRTC();

  void diskHandler_floppy();
  bool diskHandler_calcAbsAddr(int drive, uint32_t *pos, uint32_t *dest, uint32_t *count);
  void diskHandler_floppyExit(uint8_t err, bool setErrStat);
  void diskHandler_HD();
  void diskHandler_HDExit(uint8_t err, bool setErrStat);

  bool checkDriveMediaType(int drive);
  uint32_t getDriveMediaTableAddr(int drive);

  void pointingDeviceInterface();
};
