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

// Limitations:
// - only 8086 mode
// - only single mode
// - don't care about input level or edge
// - don't care about buffered mode or unbuffered mode
// - don't support special fully nested mode
// - priority is fixed to IR0 = highest
// - don't support Poll command
// - don't support Special Mask

#pragma once

#include <stdint.h>

namespace core {

// Intel 8259 Programmable Interrupt Controller (PIC)
class i8259 {

public:

  void reset();

  void write(int addr, uint8_t value);
  uint8_t read(int addr);

  // Device->8259: a device reports interrupt to 8259
  bool signalInterrupt(int intnum);

  // 8259->CPU: 8259 reports interrupt to CPU
  bool pendingInterrupt()                               { return m_pendingInterrupt; }

  // 8259->CPU: 8259 reports interrupt number to CPU
  int pendingInterruptNum()                             { return m_baseVector | m_pendingIR; }

  // CPU->8259: CPU acks the pending interrupt to 8259
  void ackPendingInterrupt();


private:

  void performEOI();
  int getHighestPriorityBitNum(uint8_t value);
  void setPendingInterrupt();

  uint8_t m_state; // all zeros = ready, bit 0 = waiting for ICW2, bit 1 = waiting for ICW3, bit 2 = waiting for ICW4

  uint8_t m_baseVector; // bits 3..7 got from ICW2
  bool    m_autoEOI;

  uint8_t m_IRR;  // Interrupt Request Register
  uint8_t m_ISR;  // In-Service Register
  uint8_t m_IMR;  // Interrupt Mask Register

  // if true port 0 reads ISR, otherwise port 0 reads IRR
  bool    m_readISR;

  bool    m_pendingInterrupt;
  uint8_t m_pendingIR;
};

} // end of namespace
