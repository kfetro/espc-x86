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

#pragma once

#include "drivers/input/ps2_controller.h"
#include "drivers/input/keyboard.h"
#include "drivers/input/mouse.h"

using fabgl::PS2Controller;
using fabgl::Keyboard;
using fabgl::Mouse;
using fabgl::MousePacket;

namespace core {

// Intel 8042 PS/2 Keyboard Controller
// Actually it is emulated how it is seen in the IBM AT

class i8042 {

public:

  typedef bool (*InterruptCallback)(void * context);
  typedef void (*HostReqCallback)(void * context, uint8_t reqId);

   i8042();
  ~i8042();

  void init();

  void reset();

  void setCallbacks(void * context, InterruptCallback keyboardInterrupt,
                                    InterruptCallback mouseInterrupt,
                                    InterruptCallback reset,
                                    HostReqCallback hostReq) {
    m_context           = context;
    m_keyboardInterrupt = keyboardInterrupt;
    m_mouseInterrupt    = mouseInterrupt;
    m_reset             = reset;
    m_hostReq           = hostReq;
  }

  void tick();
  void tickHostOnly();

  uint8_t read(int address);
  void write(int address, uint8_t value);

  Keyboard * keyboard()  { return m_keyboard; }
  Mouse * mouse()        { return m_mouse; }

  void enableMouse(bool value);

private:

  void execCommand();
  void updateCommandByte(uint8_t newValue);
  bool trigKeyboardInterrupt();
  bool trigMouseInterrupt();
  void checkHostReq(int scode2);

  PS2Controller      m_PS2Controller;
  Keyboard          *m_keyboard;
  Mouse             *m_mouse;

  void              *m_context;
  InterruptCallback  m_keyboardInterrupt;
  InterruptCallback  m_mouseInterrupt;
  InterruptCallback  m_reset;

  // Host request callback (Ctrl+Fx)
  HostReqCallback m_hostReq;

  // Key state
  bool m_ctrlDown = false;

  uint8_t            m_STATUS;
  uint8_t            m_DBBOUT;
  uint8_t            m_DBBIN;
  uint8_t            m_commandByte;
  bool               m_writeToMouse; // if True next byte on port 0 (0x60) is transferred to mouse port
  MousePacket        m_mousePacket;
  int                m_mousePacketIdx;

  // used when a command requires a parameter
  uint8_t            m_executingCommand; // 0 = none

  SemaphoreHandle_t  m_mutex;

  int                m_mouseIntTrigs;
  int                m_keybIntTrigs;

  bool               m_hostReqTriggered;
};

} // end of namespace
