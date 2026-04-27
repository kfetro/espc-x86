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
#include <stddef.h>

#include "drivers/audio/sound_generator.h"
#include "audio/adlib.h"

// AdLibWaveformGenerator adapts AdLib::render() (int16 mono PCM) to FabGL's
// WaveformGenerator interface (int8 mono samples).
//
// This allows AdLib audio to be mixed together with other generators (e.g. PC
// speaker) by fabgl::SoundGenerator and sent to the internal ESP32 DAC (TTGO VGA32 jack).
class AdLibWaveformGenerator : public fabgl::WaveformGenerator {

public:

  AdLibWaveformGenerator();

  // Binds this generator to an AdLib device instance.
  void bind(AdLib *adlib);

  void startProducerTask(int core = 1, int prio = 5, int chunkFrames = 256);
  void stopProducerTask();

  // Not used by AdLib, but required by WaveformGenerator interface.
  void setFrequency(int value) override;

  // Returns a single signed 8-bit sample (-128..127).
  int getSample() override;

private:

  static constexpr int kRingSize = 2048; // must be power of two

  AdLib *m_adlib;

  // Ring buffer of signed 8-bit samples
  volatile uint32_t m_r;
  volatile uint32_t m_w;
  int8_t m_ring[kRingSize];

  // Producer task
  TaskHandle_t m_task;
  int m_chunkFrames;
  int m_lastSampleRate;

  static void taskThunk(void * arg);
  void taskBody();

  inline uint32_t mask() const { return kRingSize - 1; }
  inline uint32_t available() const { return (m_w - m_r); }
  inline uint32_t freeSpace() const { return kRingSize - available(); }

  void pushSamples(const int16_t * in16, int count);
};
