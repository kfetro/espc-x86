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

#include "core/i8255_ppi.h"

using fabgl::PS2Preset;
using fabgl::KbdMode;

namespace core {

i8255::i8255()
  : m_portA(0),
    m_portB(0),
    m_portC(0),
    m_dirA(1),
    m_dirB(1),
    m_dirCU(1),
    m_dirCL(1),
    m_sw2LowNibble(0),
    m_sw2Bit5(false),
    m_keyboard(nullptr),
    m_mouse(nullptr),
    m_kbOBF(false),
    m_kbByte(0),
    m_breakPrefix(false),
    m_context(nullptr),
    m_keyboardIRQ(nullptr),
    m_sysReq(nullptr),
    m_keybIntTrigs(0),
    m_sysReqArmed(false)
{}

i8255::~i8255()
{
  // nothing to do
}

void i8255::init()
{
  if (!PS2Controller::initialized())
    m_PS2.begin(PS2Preset::KeyboardPort0_MousePort1, KbdMode::NoVirtualKeys);
  else
    m_PS2.keyboard()->enableVirtualKeys(false, false);

  m_keyboard = m_PS2.keyboard();
  m_mouse    = m_PS2.mouse();
  reset();
}

void i8255::reset() {
  m_dirA = m_dirB = m_dirCU = m_dirCL = 1;  // inputs (Mode 0)
  m_portA = m_portB = m_portC = 0x00;
  m_kbOBF = false;
  m_kbByte = 0;
  m_breakPrefix = false;
  m_keybIntTrigs = 0;
  m_sysReqArmed = false;
}

void i8255::setSW2(uint8_t sw2LowNibble, bool sw2Bit5) {
  m_sw2LowNibble = (sw2LowNibble & 0x0F);
  m_sw2Bit5 = sw2Bit5;
}

// -------------------------------- I/O mapping --------------------------------

uint8_t i8255::read(int address)
{
  switch (address & 3) {
    case 0: return readPortA();
    case 1: return readPortB();
    case 2: return readPortC();
    case 3: return 0xFF; // control is write-only on real 8255
  }
  return 0xFF;
}

void i8255::write(int address, uint8_t value)
{
  switch (address & 3) {
    case 0: writePortA(value); break;
    case 1: writePortB(value); break;
    case 2: writePortC(value); break;
    case 3: writeControl(value); break;
  }
}

// ------------------------------ Port operations -------------------------------

uint8_t i8255::readPortA()
{
  printf("ppi: read port A\n");
  if (m_dirA) {
    // On XT hardware, port A is a plain input; return 00h when no byte is present
    return m_kbOBF ? m_kbByte : 0x00;
  }
  return m_portA;
}

uint8_t i8255::readPortB()
{
  // Return the Port B latch. Machine will compose “live” bits on read of 0x61.
  return m_portB;
}

uint8_t i8255::readPortC()
{
  uint8_t upper = (m_dirCU ? 0xF0 : (m_portC & 0xF0));
  uint8_t lower = (m_dirCL ? 0x0F : (m_portC & 0x0F));
  uint8_t val = (upper | lower);

  // XT DIP switch mux: PB2 selects which SW2 info appears on Port C
  bool sel = (m_portB & 0x04) != 0; // PB2
  if (sel) {
    val = (val & 0xF0) | (m_sw2LowNibble & 0x0F);
  } else {
    val = (val & 0xFE) | (uint8_t)(m_sw2Bit5 ? 1 : 0);
  }
  return val;
}

void i8255::writePortA(uint8_t v) {
  if (!m_dirA) m_portA = v;
}

void i8255::writePortB(uint8_t v) {
  uint8_t old = m_portB;
  m_portB = v;
  ackKeyboardIfPB7Rising(old, v);
}

void i8255::writePortC(uint8_t v) {
  uint8_t outMask = (m_dirCU ? 0x00 : 0xF0) | (m_dirCL ? 0x00 : 0x0F);
  m_portC = (m_portC & ~outMask) | (v & outMask);
}

void i8255::writeControl(uint8_t v)
{
  if (v & 0x80) {
    // Mode set: implement Mode 0 only
    m_dirA  = (v & 0x10) ? 1 : 0;
    m_dirCU = (v & 0x08) ? 1 : 0;
    m_dirB  = (v & 0x02) ? 1 : 0;
    m_dirCL = (v & 0x01) ? 1 : 0;
  } else {
    // BSR: set/reset a bit on Port C
    uint8_t bit = (v >> 1) & 0x07;
    if (v & 1) m_portC |=  (1u << bit);
    else       m_portC &= ~(1u << bit);
  }
}

// -------------------------------- Keyboard -----------------------------------

void i8255::tick()
{
  if (!m_kbOBF) fetchKeyboardIfPossible();
  // one-shot IRQ1: only fire when we have a pending trigger
  if (m_keybIntTrigs > 0 && m_keyboardIRQ && m_keyboardIRQ(m_context)) --m_keybIntTrigs;
}

void i8255::fetchKeyboardIfPossible()
{
  if (!m_keyboard)
    return;

  if (!m_keyboard->scancodeAvailable())
    return;

  int sc2 = m_keyboard->getNextScancode();
  checkSysReq(sc2);
  uint8_t sc1 = Keyboard::convScancodeSet2To1(sc2);

  // Proper Set-2 -> Set-1 break handling (like i8042):
  //  - If sc1 == 0xF0 (break prefix), remember it and DO NOT signal a byte yet
  //  - Next non-0xF0 code is ORed with 0x80 and delivered
  if (sc1 == 0xF0) {
    m_breakPrefix = true;
    return; // no OBF yet (prefix only)
  }
  if (m_breakPrefix) {
    sc1 |= 0x80;      // mark break in Set-1
    m_breakPrefix = false;
  }

  m_kbByte = sc1;
  m_kbOBF  = true;
  ++m_keybIntTrigs;  // request exactly one IRQ1 for this byte
}

void i8255::onPort61Write(uint8_t oldValue, uint8_t newValue)
{
  ackKeyboardIfPB7Rising(oldValue, newValue);
}

void i8255::ackKeyboardIfPB7Rising(uint8_t oldB, uint8_t newB)
{
  // XT BIOS pulses PB7 (OR 80h, then AND 7Fh). The ACK is considered on the rising edge
  bool old7 = (oldB & 0x80) != 0;
  bool new7 = (newB & 0x80) != 0;
  if (!old7 && new7) {
    m_kbOBF = false;    // ACK clears current byte
    m_keybIntTrigs = 0; // ensure no further IRQs for the same byte
  }
}

// Same SysReq logic as i8042: release PrintScreen then release ALT
void i8255::checkSysReq(int sc2)
{
  if (!m_sysReq) return;
  if (m_kbByte == 0xF0) {
    if (sc2 == 0x84) {
      m_sysReqArmed = true;
    } else if (m_sysReqArmed && sc2 == 0x11) {
      m_sysReqArmed = false;
      m_sysReq(m_context);
    }
  }
}

// --------------------------------- Mouse -------------------------------------

void i8255::enableMouse(bool value)
{
  if (!m_mouse)
    return;

  if (value) {
    m_mouse->resumePort();
  } else {
    m_mouse->suspendPort();
  }
}

} // end of namespace
