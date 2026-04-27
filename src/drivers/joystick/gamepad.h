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

#include "drivers/joystick/gameport.h"

#include <math.h>

/**
 * BTGamepad:
 * Converts BLE HID joystick input (normalized [-1..+1])
 * into IBM GamePort timing values (RC windows).
 */

class BTGamepad {

public:

  BTGamepad(GamePort *port)
    : m_port(port),
      m_deadzone(0.12f),
      m_minUS(200),
      m_maxUS(3000)
  {}

  // Axis values from HID input: [-1.0 .. +1.0]
  void setAxis(float x, float y) {
    if (fabs(x) < m_deadzone) x = 0;
    if (fabs(y) < m_deadzone) y = 0;

    int x_us = mapAxis(x);
    int y_us = mapAxis(y);

    m_port->setAxis(x_us, y_us);
  }

  // buttonIndex: 0..1
  void setButton(int idx, bool pressed) {
    m_port->setButton(idx, pressed);
  }

private:

  GamePort *m_port;
  float m_deadzone;
  int m_minUS;
  int m_maxUS;

  // Maps -1..+1 into microsecond delays for RC simulation
  inline int mapAxis(float v) {
    float t = (v + 1.0f) * 0.5f;  // -> [0..1]
    return m_minUS + (int)(t * (m_maxUS - m_minUS));
  }

};
