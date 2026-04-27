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
#include <esp_timer.h>

/**
 * IBM PC GamePort (I/O port 0x201) emulator.
 * Axis bits (6,7) remain high while the RC-timing window is active.
 * Button bits (2,3) follow the standard PC joystick mapping: 0 = pressed.
 */

class GamePort {

public:

  GamePort()
    : m_startUS(0),
      m_delayX(1500),      // Neutral position timing
      m_delayY(1500),
      m_buttons(0x3C)      // Bits 2–5 high (no buttons pressed)
  {}

  // Called on OUT 0x201, resets the timing window for both axis.
  inline void start() {
    m_startUS = esp_timer_get_time();
  }

  // Called on IN 0x201, returns Atari-style 2-axis + 2-button joystick.
  inline uint8_t read() {
    uint32_t now = esp_timer_get_time();
    uint32_t elapsed = now - m_startUS;

    uint8_t v = 0;

    // Bit 6 = X axis RC discharge time
    if (elapsed < m_delayX)
      v |= 0x40;

    // Bit 7 = Y axis RC discharge time
    if (elapsed < m_delayY)
      v |= 0x80;

    // Bits 2..5 = joystick buttons (1 = released, 0 = pressed)
    v |= m_buttons;

    return v;
  }

  // Axis delays in microseconds. Range approx 200..3000 µs.
  inline void setAxis(int x_us, int y_us) {
    m_delayX = x_us;
    m_delayY = y_us;
  }

  // Button index: 0 or 1. Pressed = bit cleared.
  inline void setButton(int idx, bool pressed) {
    uint8_t mask = (idx == 0 ? 0x04 : 0x08);
    if (pressed)
      m_buttons &= ~mask;
    else
      m_buttons |= mask;
  }

private:

  uint32_t m_startUS;
  int m_delayX;
  int m_delayY;
  uint8_t m_buttons;

};
