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

#include "soc/frc_timer_reg.h"

#include <stdint.h>

// PIT (timers) frequency in Hertz
#define PIT_TICK_FREQ        1193182

// FRC1 timer prescaler and frequency
#define PIT_FRC1_PRESCALER   FRC_TIMER_PRESCALER_16
#define PIT_FRC1_FREQUENCY   5000000                // 80000000 / 16 = 5000000

// FRC1 timer has 23 bits (8388608 values)
constexpr int FRC1TimerMax = 8388607;

namespace core {

// Intel 8253 Programmable Interval Timers (PIT)
// pin connections of PIT8253 on the IBM XT:
//   gate-0 = gate-1 = +5V
//   gate-2 = TIM2GATESPK
//   clk-0  = clk-1  = clk-2 = 1193182 Hz
//   out-0  = IRQ0
//   out-1  = RAM refresh
//   out-2  = speaker
class i8253 {

public:

  typedef void (*PITCallback)(void *context, int timer, bool value);

  struct TimerInfo {
    // Defined by Control Word
    bool    BCD;          // BCD mode
    int8_t  mode;         // Timer mode
    int8_t  RL;           // Read/Load mode

    int32_t count;        // Current (decreasing) timer counter
    int32_t reload;       // Reload value when count is zero
    bool    out;          // Channel output
    bool    gate;         // date (1 = timer running)
    int32_t latch;        // Latched timer count (-1 = not latched)

    int32_t resetHolding; // Holding area for timer reset count
    bool    LSBToggle;    // true: Read load LSB, false: Read load MSB
    bool    running;      // counting down in course
    bool    ctrlSet;      // control word set
  };

   i8253();
  ~i8253();

  void init(void *context, PITCallback callback);

  void reset();

  void tick();

  void write(int reg, uint8_t value);
  uint8_t read(int reg);

  bool getOut(int channel)  { return m_timer[channel].out; }
  bool getGate(int channel) { return m_timer[channel].gate; }

  void setGate(int channel, bool value);

  TimerInfo const & timerInfo(int timer) { return m_timer[timer]; }

private:

  void changeOut(int channel, bool value);

  TimerInfo    m_timer[3];
  
  // callbacks
  void        *m_context;
  PITCallback  m_callback;
  
  uint32_t     m_lastTickTime;
  uint32_t     m_acc;

  // --- FRC1 Timer ---

  // prescaler: FRC_TIMER_PRESCALER_1, FRC_TIMER_PRESCALER_16, FRC_TIMER_PRESCALER_256
  // 80Mhz / prescaler = timer frequency
  inline void FRC1Timer_init(int prescaler) {
    REG_WRITE(FRC_TIMER_LOAD_REG(0), 0);
    REG_WRITE(FRC_TIMER_CTRL_REG(0), prescaler | FRC_TIMER_ENABLE);
  }

  inline uint32_t FRC1Timer() {
    return FRC1TimerMax - REG_READ(FRC_TIMER_COUNT_REG(0)); // make timer count up
  }

};

} // end of namespace
