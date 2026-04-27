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

#include "core/i8272_fdc.h"

namespace core {

i8272::i8272()
  : m_dor(0x00),
    m_msr(0x80),        // RQM=1, DIO=0 (CPU->FDC phase)
    m_lastDrive(0),
    m_cmdLen(0),
    m_rspLen(0),
    m_rspPos(0),
    m_irqPending(false),
    m_context(nullptr),
    m_irq6CB(nullptr)
{
  m_pcn[0] = m_pcn[1] = m_pcn[2] = m_pcn[3] = 0;
}

i8272::~i8272()
{
  // nothing to release
}

void i8272::init()
{
  reset();
}

void i8272::reset()
{
  m_dor       = 0x00;
  m_msr       = 0x80;   // RQM=1, DIO=0
  m_lastDrive = 0;
  m_pcn[0] = m_pcn[1] = m_pcn[2] = m_pcn[3] = 0;
  m_cmdLen    = 0;
  clearResponse();
  m_irqPending = false;
}

// ------------------------------ I/O API ---------------------------------

uint8_t i8272::read(int address)
{
  switch (address) {
    // MSR (Main Status Register)
    case 0x03F4:
      // If response exists -> DIO=1 (0xC0); else DIO=0 (0x80).
      return responseAvailable() ? 0xC0 : 0x80;

    // DATA (read phase: fetch result bytes)
    case 0x03F5:
      if (responseAvailable()) {
        uint8_t b = m_rspBuf[m_rspPos++];
        if (!responseAvailable()) {
          // Response drained -> back to command phase
          m_msr = 0x80; // RQM=1, DIO=0
          clearResponse();
        }
        return b;
      }
      return 0x00; // nothing to read

    // DIR (read) - disk change line; return "no change"
    case 0x3F7:
      return 0x00;

    // 0x3F0/0x3F1 and other reads (unused in this minimal stub)
    default:
      return 0x00;
  }
}

void i8272::write(int address, uint8_t value)
{
  switch (address) {
    // DOR (Digital Output Register)
    case 0x3F2:
      m_dor = value;
      if ((value & 0x04) == 0) {
        // FDC held in reset -> logical reset
        resetLogic();
      } else {
        // Enabled -> ready to accept commands (CPU->FDC)
        m_msr = 0x80; // RQM=1, DIO=0
      }
      break;

    // DATA (write phase: command/params)
    case 0x3F5:
      // CPU->FDC phase
      m_msr = 0x80; // RQM=1, DIO=0

      // Accumulate into command buffer (simple overflow guard)
      if (m_cmdLen < sizeof(m_cmdBuf))
        m_cmdBuf[m_cmdLen++] = value;

      // If we have enough bytes, execute
      if (m_cmdLen >= 1) {
        const uint8_t need = neededBytesFor(m_cmdBuf[0] & 0x1F);
        if (m_cmdLen >= need)
          execCommand(m_cmdBuf[0]);
      }
      break;

    // data-rate select (ignored in this minimal stub)
    case 0x3F7:
      (void)value;
      break;

    // 0x3F0/0x3F1 and other writes (ignored)
    default:
      (void)value;
      break;
  }
}

// ------------------------- Internal helpers -----------------------------

bool i8272::responseAvailable() const
{
  return m_rspPos < m_rspLen;
}

void i8272::clearResponse()
{
  m_rspLen = 0;
  m_rspPos = 0;
}

void i8272::resetLogic()
{
  m_cmdLen = 0;
  clearResponse();
  m_msr = 0x80;       // RQM=1, DIO=0
  m_irqPending = false;
}

uint8_t i8272::neededBytesFor(uint8_t opMasked)
{
  switch (opMasked) {
    case 0x03: return 1 /*opcode*/ + 2; // SPECIFY
    case 0x07: return 1 + 1;            // RECALIBRATE (drive)
    case 0x08: return 1 + 0;            // SENSE INTERRUPT STATUS
    case 0x04: return 1 + 1;            // SENSE DRIVE STATUS (HD+DS)
    case 0x0F: return 1 + 2;            // SEEK (HD+DS, CYL)
    default:   return 1;                // consume opcode only
  }
}

void i8272::signalIRQ6()
{
  if (m_irq6CB)
    m_irq6CB(m_context);
}

void i8272::execCommand(uint8_t cmd0)
{
  const uint8_t op = (cmd0 & 0x1F);

  switch (op) {
    case 0x03: { // SPECIFY (2 params) -> no result
      // Ignore parameters in this minimal stub
      clearResponse();
      m_msr = 0x80; // ready for another command
      break;
    }

    case 0x07: { // RECALIBRATE (1 param: drive)
      const uint8_t drive = m_cmdBuf[1] & 0x03;
      m_lastDrive = drive;
      m_pcn[drive] = 0; // back to track 0
      m_irqPending = true;
      signalIRQ6();
      clearResponse();
      m_msr = 0x80;
      break;
    }

    case 0x08: { // SENSE INTERRUPT STATUS -> return ST0, PCN
      // ST0: report "seek complete, no error", encode drive in low bits
      const uint8_t st0 = 0x20 | (m_lastDrive & 0x03);
      m_rspBuf[0] = st0;
      m_rspBuf[1] = m_pcn[m_lastDrive];
      m_rspLen = 2; m_rspPos = 0;
      m_irqPending = false;
      m_msr = 0xC0; // RQM=1, DIO=1 (FDC->CPU)
      break;
    }

    case 0x04: { // SENSE DRIVE STATUS (1 param HD+DS) -> return ST3
      const uint8_t drv = m_cmdBuf[1] & 0x03;
      uint8_t st3 = 0x20; // "ready"
      if (m_pcn[drv] == 0) st3 |= 0x10; // "track0"
      m_rspBuf[0] = st3;
      m_rspLen = 1; m_rspPos = 0;
      m_msr = 0xC0; // RQM=1, DIO=1
      break;
    }

    case 0x0F: { // SEEK (2 params: HD+DS, CYL) -> IRQ6 on completion
      const uint8_t drv = m_cmdBuf[1] & 0x03;
      const uint8_t cyl = m_cmdBuf[2];
      m_lastDrive = drv;
      m_pcn[drv] = cyl;
      m_irqPending = true;
      signalIRQ6();
      clearResponse();
      m_msr = 0x80;
      break;
    }

    default: {
      // Unsupported: consume and do nothing (keeps BIOS happy).
      clearResponse();
      m_msr = 0x80;
      break;
    }
  }

  // Command pipeline consumed
  m_cmdLen = 0;
}

uint8_t i8272::selectedDrive() const
{
  return (m_dor & 0x03);
}

} // end of namespace
