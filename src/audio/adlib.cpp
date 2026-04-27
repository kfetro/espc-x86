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

#include "audio/adlib.h"

#include "esp_timer.h"
#include <string.h>

AdLib::AdLib() :
  m_opl2(nullptr)
{
}

AdLib::~AdLib()
{
  delete m_opl2;
  m_opl2 = nullptr;
}

void AdLib::init(const AudioConfig &cfg)
{
  m_cfg = cfg;
  if (!m_opl2)
    m_opl2 = new AdLibOPL2();
  m_opl2->init(m_cfg.sampleRate);
  reset();
}

void AdLib::reset()
{
  memset(m_regs, 0, sizeof(m_regs));
  m_addr = 0;

  m_status = 0;
  m_timer1 = 0;
  m_timer2 = 0;
  m_timer1Enabled = false;
  m_timer2Enabled = false;
  m_timer1Running = false;
  m_timer1Overflow = false;
  m_timer1Armed = false;
  m_timer1StartUS = 0;

  memset(m_fnum, 0, sizeof(m_fnum));
  memset(m_block, 0, sizeof(m_block));
  memset(m_keyOn, 0, sizeof(m_keyOn));

  if (m_opl2)
    m_opl2->reset();
}

void AdLib::setSampleRate(int sampleRate)
{
  if ((sampleRate <= 0) || (sampleRate == m_cfg.sampleRate))
    return;

  // Updates the audio sample rate used by render()
  // This does not reset registers, it only reconfigures internal increments
  m_cfg.sampleRate = sampleRate;
  if (m_opl2)
    m_opl2->setSampleRate(sampleRate);
  replayRegsToCore();
}

uint8_t AdLib::read(uint16_t port)
{
  // Reads from AdLib ports, on real AdLib:
  // - 0x388 is a status register (busy flags / timers)
  // - 0x389 is typically not meaningful to read
  if (port == ADLIB_PORT_REGINDEX) {
    // Typical AdLib detection code polls 0x388 and checks bits changing due to timers.
    // We return a stable status with a "not busy" behavior:
    // - Some implementations use bit 7 as IRQ, others as busy. Games mainly want "ready".
/*
    // Minimal fake timer overflow for AdLib detection
    if (m_timer1Running && !m_timer1Overflow) {
      uint64_t now = esp_timer_get_time();

      // Trigger overflow after ~1 ms (cheap and sufficient)
      if (now - m_timer1StartUS > 0) {
        m_timer1Overflow = true;
        //m_status |= 0x40;  // IRQ/overflow bit
      }
    }
*/
    if (m_timer1Armed) {
      m_timer1Armed = false;
      m_timer1Overflow = true;
      m_status |= 0xC0;  // Timer 1 IRQ/overflow bit 
    }
    // Status/timer emulation
    // - bit 7: IRQ/busy (many games accept this as "present")
    // - bit 6: timer 1 overflow
    // - bit 5: timer 2 overflow
    return m_status;
  }
  return 0xFF;
}

void AdLib::write(uint16_t port, uint8_t value)
{
  switch(port) {

    // Register address latch
    case ADLIB_PORT_REGINDEX:
      m_addr = value;
      break;

    // Register write
    case ADLIB_PORT_REGDATA:
      m_regs[m_addr] = value;
      handleRegisterWrite(m_addr, value);
      break;

    default:
      // Ignore other ports
      break;
  }
}

// Single YM3812 register dispatcher
// Implemented registers are explicitly listed here,
// any non-listed register is effectively unimplemented.
void AdLib::handleRegisterWrite(uint8_t reg, uint8_t value)
{
  int ch;
  bool carrier;

  switch(reg) {

    case 0x00: // Test LSI
      break;

    // --- Global ---
    case 0x01: // Test / Waveform Select Enable
      // bit 5 : Enable waveforms 1-3
      m_opl2->setWaveformSelectEnabled((value & 0x20) != 0);
      break;

    // --- Control and detection ---
    case 0x02: // Timer 1
      m_timer1 = value;
      break;

    case 0x03: // Timer 2
      m_timer2 = value;
      break;

    case 0x04: // Timer control / IRQ reset
      // Bit 7: IRQ reset (commonly clears timer flags)
      // Bit 0: Timer1 start
      // Bit 1: Timer2 start
      // Bit 6: enable IRQ (varies by docs/impl)
      if (value & 0x80) {
        m_timer1Overflow = false;
        m_timer1Armed = false;
        // Clear bits 7..5 (IRQ + timer flags)
        m_status &= (uint8_t)~0xE0;
      }
      // Bit 0: start timer 1
      if (value & 0x01) {
        m_timer1Running = true;
        m_timer1StartUS = esp_timer_get_time();
        // Prince of Persia reads the status immediately after starting the timer.
        // To pass the detection, arm the overflow and raise it on the next status read.
        m_timer1Armed = true;
      } else {
        m_timer1Running = false;
        m_timer1Armed = false;
      }
      m_timer1Enabled = (value & 0x01) != 0;
      m_timer2Enabled = (value & 0x02) != 0;
      break;

    case 0x08: // CSM / Note Select
      break;

    // --- Operator groups ---
    case 0x20 ... 0x35: // Mult + Flags
      // bits 0..3 : Multiplicator
      // bit  5    : Sustain mode
      // Vibrato / KSR not implemented
      // These ranges include some unused holes; decodeOperatorReg() filters them
      if (!decodeOperatorReg(reg, ch, carrier))
        break;
      m_opl2->setOperatorMult(ch, carrier, value & 0x0F);
      m_opl2->setOperatorSustainMode(ch, carrier, (value & 0x20) != 0);
      break;

    case 0x40 ... 0x55: // Total Level
      // bits 0..5 : Attenuation level
      // KSL not implemented
      if (!decodeOperatorReg(reg, ch, carrier))
        break;
      m_opl2->setOperatorLevelTL(ch, carrier, value & 0x3F);
      break;

    case 0x60 ... 0x75: // Attack / Decay
      // bits 0..3 : Decay
      // bits 4..7 : Attack
      if (!decodeOperatorReg(reg, ch, carrier))
        break;
      m_opl2->setOperatorAD(ch, carrier, value >> 4, value & 0x0F);
      break;

    case 0x80 ... 0x95: // Sustain / Release
      // bits 0..3 : Release
      // bits 4..7 : Sustain
      if (!decodeOperatorReg(reg, ch, carrier))
        break;
      m_opl2->setOperatorSR(ch, carrier, value >> 4, value & 0x0F);
      break;

    // --- Frequency / key-on ---
    case 0xA0 ... 0xA8: // F-number
      ch = reg & 0x0F;
      m_fnum[ch] = (m_fnum[ch] & 0x300) | value;
      m_opl2->setChannelFreq(ch, m_fnum[ch], m_block[ch]);
      break;

    case 0xB0 ... 0xB8: // F-number + Block + Key-on
      ch = reg & 0x0F;
      m_fnum[ch] = (m_fnum[ch] & 0x0FF) | ((uint16_t)(value & 0x03) << 8);
      m_block[ch] = (value >> 2) & 0x07;
      m_keyOn[ch] = (value & 0x20) != 0;
      m_opl2->setChannelFreq(ch, m_fnum[ch], m_block[ch]);
      m_opl2->setChannelKeyOn(ch, m_keyOn[ch]);
      break;

    // --- Global ---
    case 0xBD: // Vibrato / Tremolo / Percusion
      // depth/vib/trem not implemented in this light core
      break;

    // --- Channel ---
    case 0xC0 ... 0xC8: // Feedback and Connection
    {
      // bit  0   : Connection (FM / additive)
      // bits 1-3 : Modulator feedback 
      ch = reg & 0x0F;
      uint8_t conn = value & 0x01;
      uint8_t fb   = (value >> 1) & 0x07;
      m_opl2->setChannelFeedbackConn(ch, fb, conn);
      break;
    }

    case 0xE0 ... 0xF5: // Waveform select
      // bits 0,1 : Waveform
      if (!decodeOperatorReg(reg, ch, carrier))
        break;
      m_opl2->setOperatorWaveform(ch, carrier, value & 0x03);
      break;

    default:
      break;
  }
}

bool AdLib::decodeOperatorReg(uint8_t reg, int &ch, bool &carrier)
{
  uint8_t base = reg & 0xE0;
  if (!(base == 0x20 || base == 0x40 || base == 0x60 || base == 0x80 || base == 0xE0))
    return false;

  uint8_t off = reg & 0x1F;
  static const int8_t offToOp[32] = {
    0, 1, 2, 3, 4, 5, -1, -1,
    6, 7, 8, 9, 10, 11, -1, -1,
    12, 13, 14, 15, 16, 17, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1
  };
  int8_t op = offToOp[off];
  if (op < 0)
    return false;

  static const int8_t opToCh[18] = {
    0,1,2,0,1,2,
    3,4,5,3,4,5,
    6,7,8,6,7,8
  };
  static const uint8_t isCar[18] = {
    0,0,0,1,1,1,
    0,0,0,1,1,1,
    0,0,0,1,1,1
  };

  ch = opToCh[op];
  carrier = isCar[op] != 0;
  return true;
}

void AdLib::replayRegsToCore()
{
  if (!m_opl2)
    return;

  for (int r = 0; r < 256; ++r) {
    // Skip timer regs
    if (r == 0x02 || r == 0x03 || r == 0x04)
      continue;
    handleRegisterWrite((uint8_t)r, m_regs[r]);
  }
}

size_t AdLib::render(int16_t *out, size_t frames)
{
  if (!out || frames == 0)
    return 0;

  if (m_opl2) {
    m_opl2->generate(out, frames);
  } else {
    memset(out, 0, frames * sizeof(int16_t));
  }

  // Apply master volume
  float vol = clampf(m_cfg.masterVolume, 0.0f, 1.0f);
  if (vol < 0.999f) {
    for (size_t i = 0; i < frames; ++i) {
      int32_t v = (int32_t)((float)out[i] * vol);
    if (v < -32768) v = -32768;
    if (v > 32767) v = 32767;
    out[i] = (int16_t) v;
    }
  }

  return frames;
}
