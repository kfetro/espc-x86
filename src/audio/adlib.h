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

#include "audio/adlib_opl2.h"

#include <stddef.h>
#include <stdint.h>

// IO Ports
#define ADLIB_PORT_REGINDEX  0x388
#define ADLIB_PORT_REGDATA   0x389

// Lightweight AdLib (YM3812/OPL2-like) device.
// - Implements port I/O behavior for 0x388 (index) and 0x389 (data)
// - Keeps a 256-byte register file
// - Generates signed 16-bit mono PCM in render()
//
// NOTE: This is NOT a cycle-accurate OPL2 emulator.
// The synth core is a "super-light" FM approximation ported from a1k0n/opl2.
class AdLib {

  public:

  // Audio output format: signed 16-bit mono at sampleRate
  struct AudioConfig {
    int sampleRate = 44100;
    float masterVolume = 0.2f; // 0.0 .. 1.0
  };

   AdLib();
  ~AdLib();

  // Initializes device state and audio configuration
  void init(const AudioConfig &cfg);

  // Resets registers and internal runtime state
  void reset();

  void setSampleRate(int sampleRate);

  // IO Ports
  uint8_t read(uint16_t port);
  void write(uint16_t port, uint8_t value);

  // Generates audio samples into 'out' as signed 16-bit mono PCM.
  size_t render(int16_t *out, size_t frames);

  uint8_t selectedRegister() const { return m_addr; }
  const uint8_t *registers() const { return m_regs; }

private:

  // YM3812 Registers (addresses 0x00..0xFF)
  uint8_t m_regs[256];
  uint8_t m_addr;

  uint8_t m_status; // Status Register
  uint8_t m_timer1; // Timer 1
  uint8_t m_timer2; // Timer 2
  bool m_timer1Enabled;
  bool m_timer2Enabled;

  // Fake OPL timer emulation (minimal, used mainly for detection)
  bool m_timer1Running;
  bool m_timer1Overflow;
  uint64_t m_timer1StartUS;
  bool     m_timer1Armed;

  AudioConfig m_cfg;

  // Cached channel state derived from YM regs
  uint16_t m_fnum[9];
  uint8_t  m_block[9];
  bool     m_keyOn[9];

  // Synthesis backend
  AdLibOPL2 *m_opl2;

  // Single unified register handler (your requested structure)
  void handleRegisterWrite(uint8_t reg, uint8_t value);

  // Replay all implemented registers to backend (after SR changes)
  void replayRegsToCore();

  // Helpers
  static bool decodeOperatorReg(uint8_t reg, int &ch, bool &carrier);

  static inline float clampf(float x, float a, float b) {
    return (x < a) ? a : (x > b) ? b : x;
  }
};
