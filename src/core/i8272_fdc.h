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

namespace core {

/**
 * Minimal NEC765/8272 FDC stub for BIOS init/POST.
 *
 * Implements exactly what a PC BIOS expects at boot:
 *  - DOR (3F2): enable/reset + drive/motor select
 *  - MSR (3F4): RQM/DIO
 *  - DATA (3F5): command/parameter/result pipe for a small command set
 *  - DIR  (3F7): disk-change (read) / data-rate (write) -- no-ops here
 *
 * Supported commands (cmd0 & 0x1F):
 *  03h SPECIFY            (2 params) -> no result
 *  07h RECALIBRATE        (1 param)  -> completes with IRQ6; BIOS calls 08h
 *  08h SENSE INT STATUS   (0 params) -> returns ST0, PCN
 *  04h SENSE DRIVE STAT   (1 param)  -> returns ST3 (ready/track0)
 *  0Fh SEEK               (2 params) -> completes with IRQ6; BIOS calls 08h
 *
 * Public API matches other devices (e.g., i8255):
 *  - init(), reset()
 *  - read(address), write(address, value)
 *  - setCallbacks(context, irq6CB)
 *
 * NOTE: read()/write() accept either absolute ports (0x3F0..0x3F7) or
 *       relative offsets 0..7 (they are normalized internally).
 */
class i8272 {

public:

  using InterruptCallback = bool (*)(void * context);   // IRQ6 callback

   i8272();
  ~i8272();

  // Initialize/reset FDC state. init() calls reset() internally.
  void init();
  void reset();

  // I/O accessors. Address can be absolute (0x3F0..0x3F7) or index 0..7.
  uint8_t read (int address);
  void    write(int address, uint8_t value);

  // Host (Machine) should set this to raise IRQ6 on command completion.
  void setCallbacks(void * context, InterruptCallback irq6CB) {
    m_context = context;
    m_irq6CB  = irq6CB;
  }

private:

  // ---- FDC state (per instance) ----
  uint8_t m_dor;        // 3F2 Digital Output Register
  uint8_t m_msr;        // 3F4 Main Status Register (we keep RQM/DIO logic)
  uint8_t m_lastDrive;  // last referenced drive
  uint8_t m_pcn[4];     // Present Cylinder Number per drive

  // Command/response staging (fixed buffers)
  uint8_t m_cmdBuf[8];
  uint8_t m_cmdLen;

  uint8_t m_rspBuf[8];
  uint8_t m_rspLen;
  uint8_t m_rspPos;

  bool    m_irqPending;

  // Callback to host PIC8259 IRQ6 line
  void *            m_context;
  InterruptCallback m_irq6CB;

  // ---- Helpers (now regular non-static methods) ----
  bool    responseAvailable() const;          // rspPos < rspLen
  void    clearResponse();                    // reset rspLen/rspPos
  void    resetLogic();                       // clear pipes, RQM=1
  uint8_t neededBytesFor(uint8_t opMasked);   // cmd length decoder
  void    execCommand(uint8_t cmd0);          // execute fully received cmd
  uint8_t selectedDrive() const;              // from DOR
  void    signalIRQ6();                       // raise IRQ6 if cb set

};

} // end of namespace
