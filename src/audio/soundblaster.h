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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/i2s.h"

// Minimal Sound Blaster compatible device emulator.
// Focus: DSP reset, basic commands, 8-bit PCM playback using a "fake DMA" FIFO.
// This is NOT a cycle-accurate SB16 and does not implement real ISA DMA/IRQ.
// Intended usage: emulator intercepts IN/OUT to SB ports and calls read()/write().
// Audio samples are supplied by the emulator via pushDMA().

class SoundBlaster16 {

public:

  explicit SoundBlaster16(uint16_t basePort = 0x220);

  // Initialize the SB emulation and the I2S DAC (e.g., MAX98357A).
  // i2sBckPin: I2S BCLK
  // i2sWsPin:  I2S LRCLK/WS
  // i2sDataPin:I2S DATA out
  // sampleRate: initial output sample rate (e.g., 22050)
  bool init(int i2sBckPin, int i2sWsPin, int i2sDataPin, int sampleRate = 22050);

  // Reset SB DSP logic (as if hardware reset occurred).
  void reset();

  // Read from an SB-related I/O port (IN instruction handler).
  uint8_t read(uint16_t port);

  // Write to an SB-related I/O port (OUT instruction handler).
  void write(uint16_t port, uint8_t value);

  // Feed "DMA" data to the SB FIFO.
  // In a real ISA SB, the DSP consumes bytes via DMA; here you push them from the emulator.
  size_t pushDMA(const uint8_t *data, size_t len);

  // Optional: query playback state (debug/help).
  bool isPlaybackActive() const;

private:
  enum class CmdState : uint8_t {
    Idle,
    CollectArgs,
  };

  // DSP / port helpers.
  void dspWriteReset(uint8_t v);
  uint8_t dspReadData();
  void dspWriteCommand(uint8_t v);

  uint8_t expectedArgsFor(uint8_t cmd) const;
  void execCommand(uint8_t cmd, const uint8_t *args, uint8_t argc);

  // I2S helpers.
  void setSampleRate(int sr);

  // Audio task.
  static void audioTaskTrampoline(void *arg);
  void audioTask();

  // FIFO helpers.
  bool popFifoByte(uint8_t &out);

private:
  // I/O base.
  uint16_t m_base;

  // I2S output configuration.
  i2s_port_t m_i2sPort = I2S_NUM_0;
  int m_sampleRate = 22050;

  // DSP state.
  bool m_dspReset = false;
  bool m_readReady = false;
  bool m_writeReady = true;
  uint8_t m_dspReadyByte = 0xFF;

  uint8_t m_dspMajor = 4;
  uint8_t m_dspMinor = 5;

  bool m_pendingSecondRead = false;
  uint8_t m_pendingSecondByte = 0;

  uint8_t m_timeConstant = 0;

  bool m_speakerOn = false;
  bool m_paused = false;

  // Mixer (very simplified).
  uint8_t m_mixerAddress = 0;
  uint8_t m_mixerRegs[256] = {};

  // DSP command parser.
  CmdState m_cmdState = CmdState::Idle;
  uint8_t m_currentCmd = 0;
  uint8_t m_args[8] = {};
  uint8_t m_argCount = 0;
  uint8_t m_expectedArgs = 0;

  // Playback state.
  bool m_playbackActive = false;
  bool m_autoInit = false;
  uint32_t m_blockRemaining = 0;

  // FIFO for fake DMA.
  static constexpr size_t FIFO_SIZE = 8192;
  uint8_t m_fifo[FIFO_SIZE];
  size_t m_fifoHead = 0;
  size_t m_fifoTail = 0;
  size_t m_fifoCount = 0;
  SemaphoreHandle_t m_fifoMutex = nullptr;

  // Audio task state.
  TaskHandle_t m_audioTaskHandle = nullptr;
  volatile bool m_audioTaskRunning = false;
};
