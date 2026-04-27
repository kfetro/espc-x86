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

#include "audio/adlib_wavegen.h"

#include <string.h>

#pragma GCC optimize ("O2")

AdLibWaveformGenerator::AdLibWaveformGenerator() :
  m_adlib(nullptr),
  m_r(0),
  m_w(0),
  m_task(nullptr),
  m_chunkFrames(256),
  m_lastSampleRate(0)
{
  memset((void *)m_ring, 0, sizeof(m_ring));
}

void AdLibWaveformGenerator::bind(AdLib * adlib)
{
  m_adlib = adlib;
  m_r = 0;
  m_w = 0;
  m_lastSampleRate = 0;
  memset((void *)m_ring, 0, sizeof(m_ring));
}

void AdLibWaveformGenerator::startProducerTask(int core, int prio, int chunkFrames)
{
  m_chunkFrames = chunkFrames > 0 ? chunkFrames : 256;
  if (m_task) {
    return;
  }
  xTaskCreatePinnedToCore(taskThunk, "adlib_audio", 4096, this, prio, &m_task, core);
}

void AdLibWaveformGenerator::stopProducerTask()
{
  if (m_task) {
    vTaskDelete(m_task);
    m_task = nullptr;
  }
}

void AdLibWaveformGenerator::setFrequency(int value)
{
  (void) value;
}

// Called from FabGL audio ISR path (DAC mode)
int AdLibWaveformGenerator::getSample()
{
  if (!enabled() || m_adlib == nullptr) {
    return 0;
  }

  // Pop one sample if available
  if (m_r == m_w) {
    return 0;
  }

  int8_t s = m_ring[m_r & mask()];
  m_r++;
  s = (int8_t) ((int)s * volume() / 127);
  return (int) s;
}

void AdLibWaveformGenerator::taskThunk(void * arg)
{
  ((AdLibWaveformGenerator *)arg)->taskBody();
}

void AdLibWaveformGenerator::pushSamples(const int16_t * in16, int count)
{
  for (int i = 0; i < count; ++i) {
    if (freeSpace() == 0) {
      return;
    }
    int8_t s8 = (int8_t) (in16[i] >> 8);
    m_ring[m_w & mask()] = s8;
    m_w++;
  }
}

// Runs in task context (NOT ISR).
void AdLibWaveformGenerator::taskBody()
{
  int16_t buf[512];

  while (true) {
    if (m_adlib == nullptr || !enabled()) {
      vTaskDelay(1);
      continue;
    }

    int sr = (int) sampleRate();
    if (sr > 0 && sr != m_lastSampleRate) {
      m_lastSampleRate = sr;
      m_adlib->setSampleRate(sr);
    }

    // Keep ring reasonably filled
    if (freeSpace() < (uint32_t)m_chunkFrames) {
      vTaskDelay(1);
      continue;
    }

    int frames = m_chunkFrames;
    if (frames > (int)(sizeof(buf) / sizeof(buf[0]))) {
      frames = (int)(sizeof(buf) / sizeof(buf[0]));
    }

    memset(buf, 0, frames * sizeof(int16_t));
    m_adlib->render(buf, (size_t)frames);
    pushSamples(buf, frames);
  }
}
