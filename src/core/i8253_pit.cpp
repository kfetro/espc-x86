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

#include "core/i8253_pit.h"

#include <stdio.h>
#include <string.h>

namespace core {

i8253::i8253()
{
  FRC1Timer_init(PIT_FRC1_PRESCALER);
}

i8253::~i8253()
{
}

void i8253::init(void *context, PITCallback callback)
{
  m_context  = context;
  m_callback = callback;

  //TODO reset();
}

void i8253::reset()
{
  for (int i = 0; i < 3; i++) {
    memset(&m_timer[i], 0, sizeof(TimerInfo));
    m_timer[i].mode      = 3;
    m_timer[i].RL        = 3;
    m_timer[i].latch     = -1;
    m_timer[i].LSBToggle = true;
  }
  m_lastTickTime = FRC1Timer();
  m_acc = 0;
}

void i8253::write(int reg, uint8_t value)
{
  // just in case ticks are needed
  tick();

  if (reg == 3) {// Write Control Register
    
    // Control Word
    //   7 6   5 4   3 2 1   0
    // +-----+-----+-------+---+
    // | SC  |  RL |  MODE |BCD|
    // +-----+-----+-------+---+
    // bit  0   : BCD (0=binary/normal, 1=BSD)
    // bits 1-3 : Mode
    //              0 = Interrupt on terminal count (Generar pulso único)
    //              1 = Hardware Retriggerable One-Shot (not used)
    //              2 = Rate Generator (Timer del sistema)
    //              3 = Square Wave Generator (Altavoz del PC)
    //              4 = Software Triggered Strobe (not used)
    //              5 = Hardware Triggered Strobe (not used)
    // bits 4-5 : Read/Load (RL) (00=Latch counter, 01=LSB, 10=MSB, 11 = LSB+MSB)
    // bits 6-7 : Select Counter (SC) (00=Timer 0, 01=Timer 1, 10=Timer 2)

    const uint8_t SC = (value >> 6) & 0x03; // channel
    const uint8_t RL = (value >> 4) & 0x03;

    if (value >> 6 == 3) {
      printf("i8253: read back. Required 8254?\n");
    }

    auto &timer = m_timer[SC];

    if (RL == 0) {
      // counter latching operation (doesn't change BCD or mode)
      timer.latch     = timer.count;
      timer.LSBToggle = true;
      timer.ctrlSet   = false;
    } else {
      // Read/Load
      timer.BCD     = (value & 1) == 1;
      timer.mode    = (value >> 1) & 0x07;
      timer.RL      = RL;
      timer.ctrlSet = true;
      if (RL == 3)
        timer.LSBToggle = true;
    }

  } else {
    // Write timers registers
    const int channel = reg;
    auto &timer = m_timer[channel];
    
    bool writeLSB = false;

    switch (timer.RL) {
      case 1:
        writeLSB = true;
        break;
      case 3:
        writeLSB = timer.LSBToggle;
        timer.LSBToggle = !timer.LSBToggle;
        break;
    }

    if (writeLSB) {
      // LSB
      timer.resetHolding = (timer.resetHolding & 0xFF00) | value;
    } else {
      // MSB
      timer.resetHolding = (timer.resetHolding & 0x00FF) | (((int) value) << 8);
      timer.reload       = timer.resetHolding;
      if (timer.ctrlSet) {
        timer.count   = (uint16_t) (timer.reload - 1);
        timer.ctrlSet = false;
      }
    }

    // OUT: with mode 0 it starts low, other modes it starts high
    changeOut(channel, timer.mode != 0);
  }
}

uint8_t i8253::read(int reg)
{
  // just in case ticks are needed
  tick();

  uint8_t value = 0;

  if (reg < 3) {
    // read timers registers
    auto &timer = m_timer[reg];

    int readValue = (timer.latch != -1) ? timer.latch : timer.count;

    bool readLSB = false;
    if (timer.RL == 1) {
      readLSB = true;
    } else if (timer.RL == 3) {
      readLSB = timer.LSBToggle;
      timer.LSBToggle = !timer.LSBToggle;
    }

    if (readLSB) {
      value = readValue & 0xFF;
    } else {
      value = (readValue >> 8) & 0xFF;
      timer.latch = -1;
    }
  }
  return value;
}

void i8253::setGate(int channel, bool value)
{
  // just in case ticks are needed
  tick();

  auto &timer = m_timer[channel];

  switch (timer.mode) {
    case 0:
    case 2:
    case 3:
      // running when gate is high
      timer.running = value;
      break;
    case 1:
    case 5:
      // switch to running when gate changes to high
      if (timer.gate == false && value == true)
        timer.running = true;
      break;
  }
  switch (timer.mode) {
    case 2:
    case 3:
      if (value == false)
        changeOut(channel, true);
      break;
  }
  if (!timer.gate && value)
    timer.count = timer.reload;
  timer.gate = value;
}

void i8253::changeOut(int channel, bool value)
{
  if (value != m_timer[channel].out) {
    m_timer[channel].out = value;
    m_callback(m_context, channel, value);
  }
}

void i8253::tick()
{
  uint32_t now = FRC1Timer();
  int32_t diff = now - m_lastTickTime;
  if (diff < 0)
    diff = FRC1TimerMax - m_lastTickTime + now;

  constexpr uint32_t BITS = 10;
  constexpr uint32_t INC  = (PIT_TICK_FREQ << BITS) / PIT_FRC1_FREQUENCY;
  
  m_acc += INC * diff;
  int ticks = m_acc >> BITS;
  m_acc &= (1 << BITS) - 1;
  
  m_lastTickTime = now;
  
  if (ticks == 0)
    return;
    
  if (ticks > 65535) {
    m_acc += (ticks - 65535) << BITS;
    ticks = 65535;
  }

  for (int channel = 0; channel < 3; channel++) {

    auto &timer = m_timer[channel];

    if (timer.running) {

      // modes 4 or 5: end of ending low pulse?
      if ((timer.mode >= 4) && !timer.out) {
        // mode 4, end of low pulse
        changeOut(channel, true);
        timer.running = false;
        timer.count   = 65535;
        continue;
      }

      timer.count -= ticks;

      // in mode 3 each tick subtract 2 instead of 1
      if (timer.mode == 3)
        timer.count -= ticks;

      if (timer.count <= 0) {
        // count terminated
        timer.count += (timer.reload == 0) ? 65536 : timer.reload;
        switch (timer.mode) {
          case 0:
          case 1:
            // at the end OUT goes high
            changeOut(channel, true);
            break;
          case 2:
            changeOut(channel, false);
            break;
          case 3:
            changeOut(channel, !timer.out);
            break;
        }
      } else {
        // count running
        switch (timer.mode) {
          case 1:
          case 4:
          case 5:
            // start low pulse
            changeOut(channel, false);
            break;
          case 2:
            changeOut(channel, true);
            break;
        }
      }
    }
  }
}

} // end of namespace
