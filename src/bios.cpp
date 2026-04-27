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

#include "bios.h"
#include "roms/bios/bios_rom.h"
#include "computer.h"
#include "core/i8086.h"

#include <string.h>

using fabgl::i8086;

BIOS::BIOS()
  : m_memory(nullptr)
{
}

void BIOS::init(Computer *computer)
{
  m_computer   = computer;
  m_memory    = m_computer->memory();
#if LEGACY_IBMPC_XT_8088
  m_i8255     = m_computer->getI8255();
  m_keyboard  = m_i8255->keyboard();
  m_mouse     = m_i8255->mouse();
#else
  m_i8042     = m_computer->getI8042();
  m_keyboard  = m_i8042->keyboard();
  m_mouse     = m_i8042->mouse();
#endif
  m_MC146818  = m_computer->getMC146818();

  constexpr uint32_t BIOS_BASE   = 0xF0000;
  constexpr uint32_t BIOS_WINDOW = 0x10000; // 64 KB

  for (uint32_t offset = BIOS_BASE; offset < BIOS_BASE + BIOS_WINDOW; offset += sizeof(bios_rom)) {
    memcpy(m_memory + offset, bios_rom, sizeof(bios_rom));
  }
}

void BIOS::reset()
{
  // Update HDD count in BIOS Data Area (BDA)
  // BDA: 0040:0075h (BIOS_NUMHD)
  m_memory[BIOS_BDA_ADDR + BIOS_NUMHD] = (bool) (m_computer->disk(2))
                                       + (bool) (m_computer->disk(3));

  // Force the emulator to advertise TWO floppy drives (A: and B:)
  // via the Equipment Word at BDA 0040:0010h (physical 0x410).
  //   bit 0    : 1 = floppy drives present
  //   bits 6-7 : (#floppies - 1). For 2 floppies -> 01b.

  // Read current equipment word (little endian) from m_memory[]
  uint16_t equipment =  (uint16_t) m_memory[BIOS_BDA_EQUIPMENT]
                     | ((uint16_t) m_memory[BIOS_BDA_EQUIPMENT] << 8);

  // Clear floppy-related bits (bit0 and bits6-7)
  equipment &= (uint16_t) ~0x0001;      // clear bit 0
  equipment &= (uint16_t) ~(0x3u << 6); // clear bits 6-7

  // Force: floppy present + 2 drives
  equipment |= 0x0001;                 // bit 0 = 1 (floppy present)
  equipment |= (uint16_t) (0x1u << 6); // (2 - 1) = 1 -> bits6-7 = 01b

  // Write back to m_memory[] (little endian)
  m_memory[BIOS_BDA_EQUIPMENT] = (uint8_t) ( equipment       & 0xFF);
  m_memory[BIOS_BDA_EQUIPMENT] = (uint8_t) ((equipment >> 8) & 0xFF);
}

// drive:
//   0 = floppy 0 (get address from INT 1E)
//   1 = floppy 1 (get address from INT 1E)
//   2 = HD 0     (get address from INT 41)
//   3 = HD 1     (get address from INT 46)
uint32_t BIOS::getDriveMediaTableAddr(int drive)
{
  int intNum = drive < 2 ? 0x1e : (drive == 2 ? 0x41 : 0x46);
  uint16_t *intAddr = (uint16_t *) (m_memory + intNum * 4);
  return intAddr[0] + intAddr[1] * 16;
}

bool BIOS::checkDriveMediaType(int drive)
{
  if (drive < 2) {
    // FDD
    if (m_mediaType[drive] == mediaUnknown) {
      MediaType mt = mediaUnknown;
      int h = m_computer->diskHeads(drive);
      int t = m_computer->diskCylinders(drive);
      int s = m_computer->diskSectors(drive);
      if (h == 1 && t == 40 && s == 8)
        mt = floppy160KB;
      else if (h == 1 && t == 40 && s == 9)
        mt = floppy180KB;
      else if (h == 2 && t == 40 && s == 8)
        mt = floppy320KB;
      else if (h == 2 && t == 40 && s == 9)
        mt = floppy360KB;
      else if (h == 2 && t == 80 && s == 9)
        mt = floppy720KB;
      else if (h == 2 && t == 80 && s == 15)
        mt = floppy1M2K;
      else if (h == 2 && t == 80 && s == 18)
        mt = floppy1M44K;
      else if (h == 2 && t == 80 && s == 36)
        mt = floppy2M88K;
      setDriveMediaType(drive, mt);
    }
  } else {
    // HDD
    if (m_computer->disk(drive))
      setDriveMediaType(drive, HDD);
  }
  return m_mediaType[drive] != mediaUnknown;
}

void BIOS::setDriveMediaType(int drive, MediaType media)
{
  m_mediaType[drive] = media;

  if (drive < 2) {
    // FDD

    // updates BIOS data area
    uint8_t knownMedia     = 0x10;        // default set bit 4 (known media)
    uint8_t doubleStepping = 0x00;        // reset bit 5 (double stepping)
    uint8_t dataRate       = 0x00;
    uint8_t defs           = 0x00;
    switch (media) {
      case floppy160KB:
      case floppy180KB:
      case floppy320KB:
      case floppy360KB:
        doubleStepping     = 0x20;        // set bit 5 (double stepping)
        dataRate           = 0b01000000;  // 300 KBS
        defs               = 0b00000100;  // Known 360K media in 1.2MB drive
        break;
      case floppy720KB:
        dataRate           = 0b10000000;  // 250 KBS
        defs               = 0b00000111;  // 720K media in 720K drive or 1.44MB media in 1.44MB drive
        break;
      case floppy1M2K:
        dataRate           = 0b00000000;  // 500 KBS
        defs               = 0b00000101;  // Known 1.2MB media in 1.2MB drive
        break;
      case floppy1M44K:
        dataRate           = 0b00000000;  // 500 KBS
        defs               = 0b00000111;  // 720K media in 720K drive or 1.44MB media in 1.44MB drive
        break;
      case floppy2M88K:
        dataRate           = 0b11000000;  // 1 MBS
        defs               = 0b00000111;  // right?
        break;
      case mediaUnknown:
      default:
        knownMedia         = 0x00;        // reset bit 4 (known media)
        break;
    }
    if (m_memory && drive < 2) {
      // BIOS data area
      m_memory[BIOS_BDA_ADDR + BIOS_DRIVE0MEDIATYPE + drive] = knownMedia | doubleStepping | dataRate | defs;
/*
      // INT 1Eh
      uint32_t maddr = getDriveMediaTableAddr(drive);
      m_memory[maddr + 0x04] = m_computer->diskSectors(drive);

      // original INT 1E (returned in ES:DI, int 13h, serv 08h)
      m_memory[m_origInt1EAddr + 0x04] = m_computer->diskSectors(drive);
      m_memory[m_origInt1EAddr + 0x0b] = m_computer->diskCylinders(drive) - 1;
*/
      // INT 1Eh
      uint32_t maddr = getDriveMediaTableAddr(drive);
      if (maddr != 0) {
        m_memory[maddr + 0x04] = m_computer->diskSectors(drive);
        m_memory[maddr + 0x0b] = m_computer->diskCylinders(drive) - 1;
      } else {
        printf("bios: Unable to update IVT\n");
      }
    }

  } else if (media == HDD) {

    // HDD

    // fill tables pointed by INT 41h or 46h
    uint32_t mtableAddr = getDriveMediaTableAddr(drive);
    *(uint16_t *) (m_memory + mtableAddr + 0x00) = m_computer->diskCylinders(drive);
    *(uint8_t *)  (m_memory + mtableAddr + 0x02) = m_computer->diskHeads(drive);
    *(uint8_t *)  (m_memory + mtableAddr + 0x0e) = m_computer->diskSectors(drive);

  }
}
/*
// convert packed BCD to decimal
static uint8_t BCDtoByte(uint8_t v)
{
  return (v & 0x0F) + (v >> 4) * 10;
}

// synchronize system ticks with RTC
void BIOS::syncTicksWithRTC()
{
  m_MC146818->updateTime();
  int ss = BCDtoByte(m_MC146818->reg(0x00));
  int mm = BCDtoByte(m_MC146818->reg(0x02));
  int hh = BCDtoByte(m_MC146818->reg(0x04));
  int totSecs = ss + mm * 60 + hh * 3600 + 1000;
  int64_t pitTicks = (int64_t)totSecs * PIT_TICK_FREQ;
  *(uint32_t *) (m_memory + BIOS_BDA_ADDR + BIOS_SYSTICKS) = (uint32_t) (pitTicks / 65536);
}
*/

// INT 11h - BIOS Equipment List
// Returns a 16-bit word describing system hardware.
// DOS uses this immediately after boot.
void BIOS::handleInt11h()
{
  // Bits are IBM-PC standard:
  // 0-1   : # of floppy drives (00b = 1 drive)
  // 2-3   : Reserved (RAM) 
  // 4-5   : Initial video mode (11b = MDA, 01b = CGA 40x25, 10b = CGA 80x25, 00b EGA)
  // 6     : Presence of a math coprocessor (1 = FPU present)
  // 7     : Reserved
  // 8-9   : # of serial ports (00b = 0 serial ports)
  // 10-11 : Reserved (old serial port encoding)
  // 12-13 : # of parallel ports (00b = 0 LPT ports)
  // 14-15 : Game/PCjr / reserved

  // The BIOS interrupt 11h always returns the word stored at 0040:0010h
  //uint16_t equipment =  (uint16_t) m_memory[BIOS_BDA_EQUIPMENT]
  //                   | ((uint16_t) m_memory[BIOS_BDA_EQUIPMENT + 1] << 8);
  uint16_t equipment = *(uint16_t *) &m_memory[BIOS_BDA_EQUIPMENT];
  i8086::setAX(equipment);
  printf("bios: int 11h (Equipment) AX=0x%04x\n", equipment);
}

// INT 12h - Conventional Memory Size
// Returns memory size in KB in AX.
// PC standard = 640 KB.
// DOS relies on this to find upper memory limit.
void BIOS::handleInt12h()
{
  // Return 640 KB (0x280)
  i8086::setAX(0x0280);
}

// INT 13h - BIOS Disk Services
void BIOS::handleInt13h()
{
  // IBM PC convention:
  // DL = 00h..7Fh : floppy / removable class
  // DL = 80h..FFh : hard disks
  if (i8086::DL() < 0x80) {
    diskHandler_floppy();
  } else {
    diskHandler_HD();
  }
}

// INT 14h - Serial Port BIOS Services
void BIOS::handleInt14h()
{
  uint8_t AH = i8086::AH();
  uint8_t port = i8086::DL(); // COM number (0=COM1, 1=COM2...)

  // AH = 00h Initialize Serial Port
  printf("bios: int 14h (COM%d) AH=0x%02x\n", port + 1, AH);

  // We implement all functions as "OK but no real hardware" (dummy implementation)
  i8086::setFlagCF(0); // Success
  i8086::setAH(0x00);  // Line status: OK
  i8086::setAL(0x60);  // Modem status: "ready"
}

// INT 15h - BIOS Extended Services (minimal AT-style support)
// Many functions are part of AT BIOS (keyboard, mouse, A20, etc).
// We only implement the pointing device interface and reject
// unsupported functions safely.
// =========================================================
void BIOS::handleInt15h()
{
  uint8_t AH = i8086::AH();

  switch (AH)
  {
    // Pointing Device (Mouse) interface - already implemented
    case 0xC2:
      pointingDeviceInterface();
      return;

    // Keyboard intercept - not implemented
    case 0x4F:
      i8086::setAH(0x80);    // 0x80 = error / unsupported
      i8086::setFlagCF(1);
      return;

    // All other extended functions not implemented
    default:
      i8086::setAH(0x86);    // 0x86 = "function not supported"
      i8086::setFlagCF(1);
      return;
  }
}

// INT 16h - BIOS Keyboard Services (dummy implementation)
// This stub reports "no key available" for all functions.
// DOS will use INT 28h and repeatedly call INT 16h AH=01h
// while waiting for user input.
void BIOS::handleInt16h()
{
  uint8_t AH = i8086::AH();

  switch (AH) {
    case 0x00: // Read key (blocking)
      // Minimal implementation: no key available
      // Return AX = 0 and ZF = 1 to signal "empty buffer"
      i8086::setFlagZF(1);
      i8086::setAX(0x0000);
      break;

    case 0x01: // Check for keystroke (non-blocking)
      i8086::setFlagZF(1); // No key available
      break;

    case 0x02: // Get shift flags
    {
      // Return BIOS keyboard flags1 in AL
      uint8_t *flags = m_memory + BIOS_BDA_ADDR + BIOS_KBDSHIFTFLAGS1;
      i8086::setAL(*flags);
      break;
    }

    default: // Unsupported function -> return "no key"
      i8086::setFlagZF(1);
      i8086::setAX(0x0000);
      break;
  }
}

// INT 17h - BIOS Printer Services (dummy implementation)
void BIOS::handleInt17h()
{
  uint8_t AH = i8086::AH();
  uint8_t port = i8086::DL();  // LPT number (0=LPT1, 1=LPT2...)

  switch (AH) {

    // Send character to printer
    case 0x00:
      // AL = character to print
      // We do nothing but return "printer ready"
      i8086::setAH(0x90);  // Printer status: 1001 0000 = ready, no error, selected
      i8086::setFlagCF(0); // Success
      break;

    // Get printer status
    case 0x01:
      printf("bios: int 17h (LPT%d) AH=0x%02d (printer status)\n", port + 1, AH);
      i8086::setAH(0x90);  // Printer ready, no error
      i8086::setFlagCF(0);
      break;

    // Initialize printer
    case 0x02:
      printf("bios: int 17h (LPT%d) AH=0x%02d (init printer)\n", port + 1, AH);
      // Pretend initialization succeeded
      i8086::setAH(0x90);
      i8086::setFlagCF(0);
      break;

    // Any unsupported function
    default:
      i8086::setAH(0x90);  // Safe default
      i8086::setFlagCF(0);
      break;
  }
}

bool BIOS::handleInt19h()
{
  // Boot unit: HD0 (80h)
  i8086::setDL(0x80);

  // Prepare INT 13h / AH=02h (to reed 1 sector)
  i8086::setAH(0x02);   // Reed sectors
  i8086::setAL(0x01);   // 1 sector
  i8086::setCH(0x00);   // cilindre 0
  i8086::setCL(0x01);   // sector 1
  i8086::setDH(0x00);   // head 0

  // Destiny address = 0000:7C00
  i8086::setES(0x0000);
  i8086::setBX(0x7C00);

  // INT 13h
  handleInt13h();

  // Check CF (output value)
  if (i8086::flagCF()) {
      // If fails call INT 18h
      return false;
  }

  printf("bios: Boot Ok\n");

  // Jump to boot sector
  i8086::setCS(0x0000);
  i8086::setIP(0x7C00);
  return true;
}

// INT 1Ah - BIOS Time-of-Day and RTC Services (minimal)
// This implementation supports the most common functions
// used by DOS during boot and runtime.
// It uses the MC146818 RTC already present in FabGL.
// =========================================================
void BIOS::handleInt1Ah()
{
  uint8_t AH = i8086::AH();

  switch (AH) {

    // AH = 00h - Get System Time
    // Returns:
    //   CX:DX = number of clock ticks since midnight (18.2 Hz)
    //   AL = midnight flag (0 or 1)
    case 0x00:
    {
      uint32_t ticks = *(uint32_t *) (m_memory + BIOS_BDA_ADDR + BIOS_SYSTICKS);
      i8086::setCX((ticks >> 16) & 0xFFFF);
      i8086::setDX(ticks & 0xFFFF);
      i8086::setAL(m_memory[BIOS_BDA_ADDR + BIOS_CLKROLLOVER]);
      i8086::setFlagCF(0);
      break;
    }

    // AH = 01h - Set System Time
    case 0x01:
    {
      uint32_t ticks = ((uint32_t)i8086::CX() << 16) | i8086::DX();
      *(uint32_t *) (m_memory + BIOS_BDA_ADDR + BIOS_SYSTICKS) = ticks;
      m_memory[BIOS_BDA_ADDR + BIOS_CLKROLLOVER] = 0;
      i8086::setFlagCF(0);
      break;
    }

    // AH = 02h - Get RTC Time (BCD)
    case 0x02:
      m_MC146818->updateTime();
      i8086::setCH(m_MC146818->reg(0x04));  // hour BCD
      i8086::setCL(m_MC146818->reg(0x02));  // min  BCD
      i8086::setDH(m_MC146818->reg(0x00));  // sec  BCD
      i8086::setDL(0);  // daylight savings flag
      i8086::setFlagCF(0);
      break;

    // AH = 04h - Get RTC Date (BCD)
    case 0x04:
      m_MC146818->updateTime();
      i8086::setCH(m_MC146818->reg(0x07)); // century? (depends)
      i8086::setCL(m_MC146818->reg(0x08)); // year
      i8086::setDH(m_MC146818->reg(0x09)); // month
      i8086::setDL(m_MC146818->reg(0x0A)); // day
      i8086::setFlagCF(0);
      break;

    // Any unsupported function -> return error
    default:
      i8086::setAH(0x86);   // Unsupported
      i8086::setFlagCF(1);
      break;
  }
}

void BIOS::diskHandler_floppy()
{
  const uint8_t drive   = i8086::DL();
  const uint8_t service = i8086::AH();

  // Only two floppies (A: and B:) exist
  if (drive > 1) {
    diskHandler_floppyExit(0x01, true);
    return;
  }

  switch (service) {

    // Reset Diskette System
    case 0x00:
      diskHandler_floppyExit(0x00, true);
      break;

    // Read Diskette Status
    case 0x01:
      // Return and clear BIOS last status for floppies
      diskHandler_floppyExit(m_memory[BIOS_BDA_ADDR + BIOS_DISKLASTSTATUS], false);
      m_memory[BIOS_BDA_ADDR + BIOS_DISKLASTSTATUS] = 0;
      break;

    case 0x02: // Read Diskette Sectors
    case 0x03: // Write Diskette Sectors
    case 0x04: // Verify Diskette Sectors
    {
      if (m_computer->disk(drive) == nullptr) {
        // Drive present, but no media inserted
        i8086::setAL(0);                    // 0 sectors transferred
        diskHandler_floppyExit(0xAA, true); // not ready
        break;
      }

      if (!checkDriveMediaType(drive)) {
        diskHandler_floppyExit(6, true);
        break;
      }
      uint32_t pos, dest, count;
      if (!diskHandler_calcAbsAddr(drive, &pos, &dest, &count)) {
        diskHandler_floppyExit(4, true);  // sector not found
        return;
      }
      fseek(m_computer->disk(drive), pos, 0);
      size_t sects = i8086::AL();
      if (service != 0x04) {
const int64_t start = esp_timer_get_time();
        sects = service == 0x02 ?
                  fread(m_memory + dest, 1, count, m_computer->disk(drive)) :
                  fwrite(m_memory + dest, 1, count, m_computer->disk(drive));
const int64_t end = esp_timer_get_time();
const double seconds = (end - start) / 1e6;
const double mb = count / (1024.0 * 1024.0);
//printf("bench: read %.2f MB in %.2f s = %.3f MB/s\n", mb, seconds, mb / seconds);
        sects /= 512;
      }
      i8086::setAL(sects);
      diskHandler_floppyExit(sects == 0 ? 4 : 0, true);
      m_computer->resetDiskChanged(drive);
      return;
    }

    // Format Diskette Track
    case 0x05:
    {
      int sectsCountToFormat = i8086::AL();
      int track              = i8086::CH();
      int head               = i8086::DH();
      uint32_t tableAddr     = i8086::ES() * 16 + i8086::BX();

      int SPT = m_computer->diskSectors(drive);
      int tracksCount = m_computer->diskCylinders(drive);

      uint8_t fillByte = m_memory[getDriveMediaTableAddr(drive) + 8];

      uint8_t * buf = (uint8_t *) malloc(512);
      memset(buf, fillByte, 512);

      for (int i = 0; i < sectsCountToFormat; ++i) {
        int ttrack  = m_memory[tableAddr++];
        int thead   = m_memory[tableAddr++];
        int tsect   = m_memory[tableAddr++];
        int tsectSz = 128 << m_memory[tableAddr++];
        if (ttrack != track || thead > 1 || tsect > SPT || tsectSz != 512 || track >= tracksCount) {
          // error
          free(buf);
          diskHandler_floppyExit(0x04, true);
          return;
        }
        fseek(m_computer->disk(drive), 512 * ((track * 2 + head) * SPT + (tsect - 1)), 0);
        fwrite(buf, 1, 512, m_computer->disk(drive));
      }
      free(buf);
      diskHandler_floppyExit(0x00, true);
      m_computer->resetDiskChanged(drive);
      return;
    }

    // Read Drive Parameters
    case 0x08:
    {
      i8086::setAX(0x0000);
      i8086::setBH(0x00);
      checkDriveMediaType(drive);
      switch (m_mediaType[drive]) {
        case floppy160KB:
        case floppy180KB:
        case floppy320KB:
          // @TODO: check this
          i8086::setBL(0x01);
          break;
        case floppy360KB:
          i8086::setBL(0x01);
          break;
        case floppy720KB:
          i8086::setBL(0x03);
          break;
        case floppy1M2K:
          i8086::setBL(0x02);
          break;
        case floppy1M44K:
          i8086::setBL(0x04);
          break;
        case floppy2M88K:
          i8086::setBL(0x05);
          break;
        case mediaUnknown:
        default:
          i8086::setBL(0x04);
          break;
      }
      if (m_computer->disk(drive)) {
        i8086::setCH(m_computer->diskCylinders(drive) - 1);
        i8086::setCL(m_computer->diskSectors(drive));
        i8086::setDH(m_computer->diskHeads(drive) - 1);
      } else {
        i8086::setCH((uint8_t) 79); // max cylinder (low 8 bits)
        i8086::setCL(18);           // max sector (1..63)
        i8086::setDH((uint8_t) 1);  // max head
      }
      // Number of floppy drives installed
      i8086::setDL(2);
/*
      // Pointer to Diskette Parameters table for the maximum media type supported on the specified drive
      i8086::setES(BIOS_SEG);
      i8086::setDI(m_origInt1EAddr - BIOS_SEG * 16);
*/
      // Pointer to Diskette Parameter Table (INT 1Eh)
      uint32_t tbl  = getDriveMediaTableAddr(drive);
      if (tbl  != 0) {
        i8086::setES((uint16_t) (tbl  >> 4));
        i8086::setDI((uint16_t) (tbl  & 0x0F));
      } else {
        i8086::setES(0x0000);
        i8086::setDI(0x0000);
      }

      // Keep BIOS last status = 0
      m_memory[BIOS_BDA_ADDR + BIOS_DISKLASTSTATUS] = 0;
      //diskHandler_floppyExit(0, true);
      break;
    }

    // Read Drive Type
    case 0x15:
      diskHandler_floppyExit(0, true);
      i8086::setAH(m_computer->disk(drive) ? 0x02 : 0x00);
      break;

    // Detect Media Change
    case 0x16:
      // If no disk inserted, report "changed" so DOS knows media is not stable.
      if ((m_computer->disk(drive) == nullptr) || m_computer->diskChanged(drive)) {
        diskHandler_floppyExit(0x06, true);
      } else {
        diskHandler_floppyExit(0x00, true);
      }
      break;

    // Set Diskette Type
    case 0x17:
      switch (i8086::AL()) {

        // 320K/360K
        case 0x01:
          diskHandler_floppyExit(m_mediaType[drive] != floppy360KB &&
                                 m_mediaType[drive] != floppy320KB &&
                                 m_mediaType[drive] != floppy180KB &&
                                 m_mediaType[drive] != floppy160KB, true);
          break;

        // 360K
        case 0x02:
          diskHandler_floppyExit(m_mediaType[drive] != floppy360KB, true);
          break;

        // 1.2MB
        case 0x03:
          diskHandler_floppyExit(m_mediaType[drive] != floppy1M2K, true);
          break;

        // 720KB
        case 0x04:
          diskHandler_floppyExit(m_mediaType[drive] != floppy720KB, true);
          break;

        // error
        default:
          diskHandler_floppyExit(1, true);
          break;

      }
      m_computer->resetDiskChanged(drive);
      break;

    // Set Media Type for Format
    case 0x18:
    {
      // check if proposed media type matches with current
      int propTracks = i8086::CH();
      int propSPT    = i8086::CL();
      int tracks     = m_computer->diskCylinders(drive) - 1;
      int SPT        = m_computer->diskSectors(drive);
      if (propTracks == tracks && propSPT == SPT) {
        // match ok
        diskHandler_floppyExit(0x00, true);
/*
        i8086::setES(BIOS_SEG);
        i8086::setDI(m_origInt1EAddr - BIOS_SEG * 16);
*/
        // Pointer to Diskette Parameter Table (INT 1Eh)
        uint32_t maddr = getDriveMediaTableAddr(drive);
        if (maddr != 0) {
          i8086::setES(maddr >> 4);
          i8086::setDI(maddr & 0x0F);
        } else {
          i8086::setES(0x0000);
          i8086::setDI(0x0000);
        }
      } else {
        // not supported
        diskHandler_floppyExit(0x0c, true);
        printf("bios: int 13h, FDD, 18h: unsupported media type, t=%d (%d), s=%d (%d)\n", propTracks, tracks, propSPT, SPT);
      }
      m_computer->resetDiskChanged(drive);
      break;
    }

    default:
      diskHandler_floppyExit(1, true);
      printf("bios: Unhandled int 13h (AH=0x%02x)\n", service);
      break;
  }
}

// CH       : low 8 bits of track number
// CL 6...7 : high 2 bits of track number
// CL 0...5 : sector number
// DH       : head number
// AL       : number of sectors to read
// ES:BX    : destination address
bool BIOS::diskHandler_calcAbsAddr(int drive, uint32_t *pos, uint32_t *dest, uint32_t *count)
{
  int sectorsPerTrack = m_computer->diskSectors(drive);
  int heads           = m_computer->diskHeads(drive);
  int track  = i8086::CH() | (((uint16_t)i8086::CL() & 0xc0) << 2);
  int sector = i8086::CL() & 0x3f;
  int head   = i8086::DH();
  if (sector > sectorsPerTrack)
    return false;
  *pos   = 512 * ((track * heads + head) * sectorsPerTrack + (sector - 1));
  *dest  = i8086::ES() * 16 + i8086::BX();
  *count = i8086::AL() * 512;
  return true;
}

void BIOS::diskHandler_floppyExit(uint8_t err, bool setErrStat)
{
  i8086::setAH(err);
  i8086::setFlagCF(err ? 1 : 0);
  if (setErrStat)
    m_memory[BIOS_BDA_ADDR + BIOS_DISKLASTSTATUS] = err;
}

void BIOS::diskHandler_HD()
{
  int drive   = (i8086::DL() & 1) + 2;  // 2 = HD0, 3 = HD1
  int service =  i8086::AH();

  if (m_computer->disk(drive) == nullptr || i8086::DL() > 0x81) {
    // invalid drive
    diskHandler_HDExit(0x80, true);
    return;
  }

  switch (service) {

    // Reset Fixed Disk System
    case 0x00:
      diskHandler_HDExit(checkDriveMediaType(drive) ? 0x00 : 0x80, true);
      return;

    // Read Disk Status
    case 0x01:
      diskHandler_HDExit(m_memory[BIOS_BDA_ADDR + BIOS_HDLASTSTATUS], false);
      m_memory[BIOS_BDA_ADDR + BIOS_HDLASTSTATUS] = 0; // this function resets BIOS_HDLASTSTATUS
      return;

    // Read Fixed Disk Sectors
    case 0x02:
    // Write Fixed Disk Sectors
    case 0x03:
    // Verify Fixed Disk Sectors
    case 0x04:
    {
      if (!checkDriveMediaType(drive)) {
        diskHandler_HDExit(0x80, true);
        return;
      }
      uint32_t pos, dest, count;
      if (!diskHandler_calcAbsAddr(drive, &pos, &dest, &count)) {
        diskHandler_HDExit(4, true);  // sector not found
        return;
      }
      fseek(m_computer->disk(drive), pos, 0);
      size_t sects = i8086::AL();
      if (service != 0x04) {
        sects = service == 0x02 ?
                  fread(m_memory + dest, 1, count, m_computer->disk(drive)) :
                  fwrite(m_memory + dest, 1, count, m_computer->disk(drive));
        sects /= 512;
      }
      i8086::setAL(sects);
      diskHandler_HDExit(sects == 0 ? 4 : 0, true);
      return;
    }

    // Format Disk Cylinder
    case 0x05:
      diskHandler_HDExit(0x00, true);
      return;

    // Read Drive Parameters
    case 0x08:
      i8086::setAL(0x00);
      if (checkDriveMediaType(drive)) {
        uint16_t maxUsableCylNum  = m_computer->diskCylinders(drive) - 1;
        uint8_t  maxUsableSecNum  = m_computer->diskSectors(drive);
        uint8_t  maxUsableHeadNum = m_computer->diskHeads(drive) - 1;
        i8086::setCH(maxUsableCylNum & 0xff);                                       // Maximum usable cylinder number (low 8 bits)
        i8086::setCL(((maxUsableCylNum >> 2) & 0xc0) |                              // Bits 7-6 = Maximum usable cylinder number (high 2 bits)
                     (maxUsableSecNum & 0x3f));                                     // Bits 5-0 = Maximum usable sector number
        i8086::setDH(maxUsableHeadNum);                                             // Maximum usable head number
        i8086::setDL((bool)(m_computer->disk(2)) + (bool)(m_computer->disk(3)));      // Number of drives (@TODO: correct? Docs not clear)
        // Address of Fixed Disk Parameters table
        // *** note: some texts tell ES:DI should return a pointer to parameters table. IBM docs don't. Actually
        //           returning ES:DI may crash old MSDOS versions!
        //i8086::setES(BIOS_SEG);
        //i8086::setDI(getDriveMediaTableAddr(drive) - BIOS_SEG * 16);
/*
        printf("CH = %02X CL = %02X DH = %02X DL = %02X\n", i8086::CH(), i8086::CL(), i8086::DH(), i8086::DL());
        printf("ES = %04X  DI = %04X\n", i8086::ES(), i8086::DI());
        printf("ES:DI [00-01] = %d\n", (int) *(uint16_t*)(m_memory + i8086::ES() * 16 + i8086::DI() + 0x00));
        printf("ES:DI    [02] = %d\n", (int) *(uint8_t*)(m_memory + i8086::ES() * 16 + i8086::DI() + 0x02));
        printf("ES:DI    [0e] = %d\n", (int) *(uint8_t*)(m_memory + i8086::ES() * 16 + i8086::DI() + 0x0e));
*/
        diskHandler_HDExit(0x00, true);
      } else {
        i8086::setCX(0x0000);
        i8086::setDX(0x0000);
        diskHandler_HDExit(0x80, true);
      }
      return;

    // Initialize Drive Parameters
    case 0x09:
    // Seek to Cylinder
    case 0x0c:
    // Recalibrate Drive
    case 0x11:
    // Controller Internal Diagnostic
    case 0x14:
      diskHandler_HDExit(checkDriveMediaType(drive) ? 0x00 : 0x80, true);
      return;

    // Test for Drive Ready
    case 0x10:
      diskHandler_HDExit(checkDriveMediaType(drive) ? 0x00 : 0xAA, true);
      return;

    // Read Disk Type
    case 0x15:
      if (checkDriveMediaType(drive)) {
        diskHandler_HDExit(0x00, true);
        i8086::setAH(0x03);// drive present
        auto sectors = m_computer->diskSize(drive) / 512;
        i8086::setDX(sectors & 0xffff);
        i8086::setCX(sectors >> 16);
      } else {
        i8086::setAX(0x0000);
        i8086::setCX(0x0000);
        i8086::setDX(0x0000);
        diskHandler_HDExit(0x00, true); // yes, it is 0x00!
      }
      return;

    default:
      printf("bios: Unhandled int 13h (AH=0x%02x)\n", service);
      diskHandler_HDExit(1, true);
      return;
  }
}

void BIOS::diskHandler_HDExit(uint8_t err, bool setErrStat)
{
  i8086::setAH(err);
  i8086::setFlagCF(err ? 1 : 0);
  if (setErrStat)
    m_memory[BIOS_BDA_ADDR + BIOS_HDLASTSTATUS] = err;
}

// Implements all services of "INT 15 Function C2h"
// inputs:
//    AL : subfunction
//    .. : depends by the subfunction
// outputs:
//    AH : 0 = success, >0 = error (see "INT 15h Function C2h - Pointing Device Interface")
//    CF : 0 = successful, 1 = unsuccessful
//    .. : depends by the subfunction
void BIOS::pointingDeviceInterface()
{
  if (m_mouse->isMouseAvailable()) {

    i8086::setAH(0x00);
    i8086::setFlagCF(0);

    switch (i8086::AL()) {

      // Enable/disable pointing device
      // inputs:
      //    AL : 0x00
      //    BH : 0 = disable, 1 = enable
      case 0x00:
#if LEGACY_IBMPC_XT_8088
        m_i8255->enableMouse(i8086::BH());
#else
        m_i8042->enableMouse(i8086::BH());
#endif
        break;

      // Reset pointing device
      // inputs:
      //    AL : 0x01
      // outputs:
      //    BH : Device ID
      case 0x01:
#if LEGACY_IBMPC_XT_8088
        m_i8255->enableMouse(false);             // mouse disabled
#else
        m_i8042->enableMouse(false);             // mouse disabled
#endif
        m_mouse->setSampleRate(100);             // 100 reports/second
        m_mouse->setResolution(2);               // 4 counts/millimeter
        m_mouse->setScaling(1);                  // 1:1 scaling
        i8086::setBH(m_mouse->deviceID() & 0xff);
        break;

      // Set sample rate
      // inputs:
      //    AL : 0x02
      //    BH : Sample rate
      case 0x02:
        m_mouse->setSampleRate(i8086::BH());
        break;

      // Set resolution
      // inputs:
      //    AL : 0x03
      //    BH : Resolution value
      case 0x03:
        m_mouse->setResolution(i8086::BH());
        break;

      // Read device type
      // inputs:
      //    AL : 0x04
      case 0x04:
        i8086::setBH(m_mouse->deviceID() & 0xff);
        break;

      // Initialize pointing device interface
      // inputs:
      //    AL : 0x05
      //    BH : Data package size (1-8, in bytes)
      //         note: this value is acqually ignored because we get actual packet size from Mouse object
      case 0x05:
      {
#if LEGACY_IBMPC_XT_8088
        m_i8255->enableMouse(false);              // mouse disabled
#else
        m_i8042->enableMouse(false);              // mouse disabled
#endif
        m_mouse->setSampleRate(100);              // 100 reports/second
        m_mouse->setResolution(2);                // 4 counts/millimeter
        m_mouse->setScaling(1);                   // 1:1 scaling
        uint8_t * EBDA = m_memory + EBDA_ADDR;
        EBDA[EBDA_DRIVER_OFFSET] = 0x0000;
        EBDA[EBDA_DRIVER_SEG]    = 0x0000;
        EBDA[EBDA_FLAGS1]        = 0x00;
        EBDA[EBDA_FLAGS2]        = m_mouse->getPacketSize(); // instead of i8086::BH()!!
        break;
      }

      // Set scaling or get status
      // inputs:
      //    AL : 0x06
      //    BH : subfunction
      case 0x06:
        switch (i8086::BH()) {
          // Set scaling factor to 1:1
          // inputs:
          //    BH : 0x01
          case 0x01:
            m_mouse->setScaling(1);
            break;
          // Set scaling factor to 2:1
          // inputs:
          //    BH : 0x02
          case 0x02:
            m_mouse->setScaling(2);
            break;
          default:
            // not implements
            printf("Pointing device function 06:%02X not implemented\n", i8086::BH());
            i8086::setAH(0x86);
            i8086::setFlagCF(1);
            break;
        }
        break;

      // Set pointing device handler address
      // inputs:
      //    AL = 0x07
      //    ES:BX : Pointer to application-program's device driver
      case 0x07:
      {
        uint8_t * EBDA = m_memory + EBDA_ADDR;
        *(uint16_t *) (EBDA + EBDA_DRIVER_OFFSET) = i8086::BX();
        *(uint16_t *) (EBDA + EBDA_DRIVER_SEG)    = i8086::ES();
        EBDA[EBDA_FLAGS2] |= 0x80;  // set handler installed flag
        break;
      }

      default:
        // not implements
        printf("Pointing device function %02X not implemented\n", i8086::AL());
        i8086::setAH(0x86);
        i8086::setFlagCF(1);
        break;
    }

  } else {
    // mouse not available
    i8086::setAH(0x03);   // 0x03 = interface error
    i8086::setFlagCF(1);
  }
}
