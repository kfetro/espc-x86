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

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_timer.h"
#include "esp_idf_version.h"

#if ESP_IDF_VERSION <= ESP_IDF_VERSION_VAL(3, 3, 5)
typedef nvs_handle nvs_handle_t;
#endif

namespace core {

// RTC (Motorola) MC146818 emulator

// On the PC/AT there are following connections:
//    /IRQ   -> IRQ8 (INT 70h)
//    CKFS   -> 5V  (hence CKOUT has the same frequency as OSC1)
//    PS     -> 5V
//    /RESET -> 5V
//    OSC1   -> 32768Hz clock
//    OSC2   -> NC
//    CKOUT  -> NC
//    SQW    -> NC

// I/O Access
//   port 0x70 (W)   : Register address port (bits 0-6)
//   port 0x71 (R/W) : Register read or write

class MC146818 {

public:

  typedef bool (*InterruptCallback)(void * context);

  MC146818();
  ~MC146818();

  void init(char const * NVSNameSpace);

  void setCallbacks(void * context, InterruptCallback interruptCallback) {
    m_context           = context;
    m_interruptCallback = interruptCallback;
  }

  void reset();

  void commit();

  uint8_t read(int address);
  void write(int address, uint8_t value);

  uint8_t & reg(int address) { return m_regs[address]; }

  void updateTime();

private:

  void enableTimers();

  void stopPeriodicTimer();
  void stopEndUpdateTimer();

  static void periodIntTimerFunc(void * args);
  static void endUpdateIntTimerFunc(void * args);

  nvs_handle_t       m_nvs;

  uint8_t            m_regs[64];
  uint8_t            m_regSel;

  void *             m_context;
  InterruptCallback  m_interruptCallback;

  esp_timer_handle_t m_periodicIntTimerHandle;
  esp_timer_handle_t m_endUpdateIntTimerHandle;

};

} // end of namespace
