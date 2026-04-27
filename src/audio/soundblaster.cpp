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

#include "soundblaster.h"

SoundBlaster16::SoundBlaster16(uint16_t basePort)
: m_base(basePort) {
}

bool SoundBlaster16::init(int i2sBckPin, int i2sWsPin, int i2sDataPin, int sampleRate) {
  m_sampleRate = (sampleRate <= 0) ? 22050 : sampleRate;

  m_fifoMutex = xSemaphoreCreateMutex();
  if (m_fifoMutex == nullptr) {
    return false;
  }

  // Configure I2S (TX only) for external DAC like MAX98357A.
  // MAX98357A expects standard I2S. We'll output 16-bit mono (left channel).
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = m_sampleRate;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 6;
  cfg.dma_buf_len = 256;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = true;
  cfg.fixed_mclk = 0;

  i2s_pin_config_t pins = {};
  pins.bck_io_num = i2sBckPin;
  pins.ws_io_num = i2sWsPin;
  pins.data_out_num = i2sDataPin;
  pins.data_in_num = I2S_PIN_NO_CHANGE;

  if (i2s_driver_install(m_i2sPort, &cfg, 0, nullptr) != ESP_OK) {
    return false;
  }
  if (i2s_set_pin(m_i2sPort, &pins) != ESP_OK) {
    return false;
  }

  reset();

  m_audioTaskRunning = true;
  xTaskCreatePinnedToCore(
    &SoundBlaster16::audioTaskTrampoline,
    "SB_Audio",
    4096,
    this,
    2,
    &m_audioTaskHandle,
    1
  );

  return true;
}

void SoundBlaster16::reset() {
  // DSP reset behavior:
  // - After reset deassertion, DSP places 0xAA in read buffer.
  m_dspReset = true;
  m_dspReadyByte = 0xAA;
  m_readReady = true;
  m_writeReady = true;

  m_speakerOn = false;
  m_paused = false;

  m_timeConstant = 0;
  m_dspMajor = 4;
  m_dspMinor = 5;

  m_pendingSecondRead = false;
  m_pendingSecondByte = 0;

  m_cmdState = CmdState::Idle;
  m_currentCmd = 0;
  m_argCount = 0;
  m_expectedArgs = 0;

  m_playbackActive = false;
  m_autoInit = false;
  m_blockRemaining = 0;

  m_mixerAddress = 0;
  m_mixerRegs[0x22] = 0xFF;  // Master volume (simplified)
  m_mixerRegs[0x04] = 0xFF;  // Voice volume (simplified)

  // Flush FIFO.
  if (m_fifoMutex) {
    xSemaphoreTake(m_fifoMutex, portMAX_DELAY);
    m_fifoHead = m_fifoTail = 0;
    m_fifoCount = 0;
    xSemaphoreGive(m_fifoMutex);
  }
}

uint8_t SoundBlaster16::read(uint16_t port) {
  uint16_t off = port - m_base;

  switch (off) {
    case 0x0A:  // DSP Read Data (0x22A)
      return dspReadData();

    case 0x0C:  // DSP Write Buffer Status (0x22C) bit7=busy on real HW
      return m_writeReady ? 0x00 : 0x80;

    case 0x0E:  // DSP Read Buffer Status (0x22E) bit7=data available
      return m_readReady ? 0x80 : 0x00;

    case 0x04:  // Mixer Address (0x224)
      return m_mixerAddress;

    case 0x05:  // Mixer Data (0x225)
      return m_mixerRegs[m_mixerAddress];

    default:
      return 0xFF;
  }
}

void SoundBlaster16::write(uint16_t port, uint8_t value) {
  uint16_t off = port - m_base;

  switch (off) {
    case 0x06:  // DSP Reset (0x226)
      dspWriteReset(value);
      break;

    case 0x0C:  // DSP Write Command/Data (0x22C)
      dspWriteCommand(value);
      break;

    case 0x04:  // Mixer Address (0x224)
      m_mixerAddress = value;
      break;

    case 0x05:  // Mixer Data (0x225)
      m_mixerRegs[m_mixerAddress] = value;
      break;

    default:
      break;
  }
}

size_t SoundBlaster16::pushDMA(const uint8_t *data, size_t len) {
  if (data == nullptr || len == 0 || m_fifoMutex == nullptr) {
    return 0;
  }

  xSemaphoreTake(m_fifoMutex, portMAX_DELAY);

  size_t pushed = 0;
  while (pushed < len && m_fifoCount < FIFO_SIZE) {
    m_fifo[m_fifoHead] = data[pushed++];
    m_fifoHead = (m_fifoHead + 1) % FIFO_SIZE;
    m_fifoCount++;
  }

  xSemaphoreGive(m_fifoMutex);
  return pushed;
}

bool SoundBlaster16::isPlaybackActive() const {
  return m_playbackActive && !m_paused;
}

void SoundBlaster16::dspWriteReset(uint8_t v) {
  // Typical usage: write 1, delay, write 0.
  // When asserted: clear buffers.
  // When deasserted: set read buffer to 0xAA.
  if (v & 0x01) {
    m_dspReset = true;
    m_readReady = false;
    m_writeReady = true;
    m_dspReadyByte = 0;

    if (m_fifoMutex) {
      xSemaphoreTake(m_fifoMutex, portMAX_DELAY);
      m_fifoHead = m_fifoTail = 0;
      m_fifoCount = 0;
      xSemaphoreGive(m_fifoMutex);
    }

    m_cmdState = CmdState::Idle;
    m_currentCmd = 0;
    m_argCount = 0;
    m_expectedArgs = 0;
  } else {
    if (m_dspReset) {
      m_dspReset = false;
      m_dspReadyByte = 0xAA;
      m_readReady = true;
    }
  }
}

uint8_t SoundBlaster16::dspReadData() {
  if (m_readReady) {
    m_readReady = false;
    return m_dspReadyByte;
  }
  return 0xFF;
}

void SoundBlaster16::dspWriteCommand(uint8_t v) {
  if (m_cmdState == CmdState::Idle) {
    m_currentCmd = v;
    m_argCount = 0;
    m_expectedArgs = expectedArgsFor(v);

    if (m_expectedArgs == 0) {
      execCommand(v, nullptr, 0);
      m_cmdState = CmdState::Idle;
    } else {
      m_cmdState = CmdState::CollectArgs;
    }
    return;
  }

  if (m_cmdState == CmdState::CollectArgs) {
    m_args[m_argCount++] = v;
    if (m_argCount >= m_expectedArgs) {
      execCommand(m_currentCmd, m_args, m_expectedArgs);
      m_cmdState = CmdState::Idle;
    }
  }
}

uint8_t SoundBlaster16::expectedArgsFor(uint8_t cmd) const {
  // Minimal subset of DSP commands:
  // 0xE1: Get DSP version (0 args, returns 2 bytes via read port)
  // 0x40: Set time constant (1 arg)
  // 0x14: 8-bit single-cycle DMA output (2 args: length low/high)
  // 0x1C: 8-bit auto-init DMA output (2 args)
  // 0xE0: DSP identification (1 arg)
  switch (cmd) {
    case 0x40: return 1;
    case 0x14: return 2;
    case 0x1C: return 2;
    case 0xE0: return 1;
    default:   return 0;
  }
}

void SoundBlaster16::execCommand(uint8_t cmd, const uint8_t *args, uint8_t argc) {
  switch (cmd) {
    case 0xE1: {
      // Return major now, minor on the next read.
      m_dspReadyByte = m_dspMajor;
      m_readReady = true;
      m_pendingSecondRead = true;
      m_pendingSecondByte = m_dspMinor;
      break;
    }

    case 0x40: {
      if (argc == 1) {
        m_timeConstant = args[0];

        // Approx conversion:
        // sampleRate ~= 1000000 / (256 - timeConstant)
        int denom = 256 - (int)m_timeConstant;
        if (denom <= 0) denom = 1;
        int sr = 1000000 / denom;
        if (sr < 4000) sr = 4000;
        if (sr > 44100) sr = 44100;
        setSampleRate(sr);
      }
      break;
    }

    case 0xD1:  // Speaker on
      m_speakerOn = true;
      break;

    case 0xD3:  // Speaker off
      m_speakerOn = false;
      break;

    case 0xD0:  // Halt DMA
      m_paused = true;
      break;

    case 0xD4:  // Continue DMA
      m_paused = false;
      break;

    case 0xDA:  // Exit auto-init DMA
      m_autoInit = false;
      m_playbackActive = false;
      m_blockRemaining = 0;
      break;

    case 0x14: {
      // 8-bit single-cycle output, length.
      if (argc == 2) {
        uint16_t len = (uint16_t)args[0] | ((uint16_t)args[1] << 8);
        m_blockRemaining = (uint32_t)len + 1;
        m_autoInit = false;
        m_playbackActive = true;
      }
      break;
    }

    case 0x1C: {
      // 8-bit auto-init output, length.
      if (argc == 2) {
        uint16_t len = (uint16_t)args[0] | ((uint16_t)args[1] << 8);
        m_blockRemaining = (uint32_t)len + 1;
        m_autoInit = true;
        m_playbackActive = true;
      }
      break;
    }

    case 0xE0: {
      // Simplified DSP identification (enough for some detection routines).
      if (argc == 1) {
        m_dspReadyByte = (uint8_t)(args[0] ^ 0xFF);
        m_readReady = true;
      }
      break;
    }

    default:
      // Ignore unimplemented commands.
      break;
  }
}

void SoundBlaster16::setSampleRate(int sr) {
  m_sampleRate = sr;
  i2s_set_clk(m_i2sPort, m_sampleRate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}

void SoundBlaster16::audioTaskTrampoline(void *arg) {
  SoundBlaster16 *self = reinterpret_cast<SoundBlaster16*>(arg);
  self->audioTask();
  vTaskDelete(nullptr);
}

bool SoundBlaster16::popFifoByte(uint8_t &out) {
  if (m_fifoMutex == nullptr) {
    return false;
  }

  bool ok = false;
  xSemaphoreTake(m_fifoMutex, portMAX_DELAY);

  if (m_fifoCount > 0) {
    out = m_fifo[m_fifoTail];
    m_fifoTail = (m_fifoTail + 1) % FIFO_SIZE;
    m_fifoCount--;
    ok = true;
  }

  xSemaphoreGive(m_fifoMutex);
  return ok;
}

void SoundBlaster16::audioTask() {
  // Produce 16-bit mono samples from 8-bit FIFO.
  // SB 8-bit PCM is unsigned (0..255) with 128 as silence.
  const size_t chunkFrames = 256;
  int16_t out[chunkFrames];

  while (m_audioTaskRunning) {
    // Provide DSP version second byte if pending and no read is currently ready.
    // This is a practical approximation for host reading two bytes after 0xE1.
    if (!m_readReady && m_pendingSecondRead) {
      m_pendingSecondRead = false;
      m_dspReadyByte = m_pendingSecondByte;
      m_readReady = true;
    }

    for (size_t i = 0; i < chunkFrames; ++i) {
      uint8_t s = 128;

      if (m_speakerOn && m_playbackActive && !m_paused) {
        uint8_t b;
        if (popFifoByte(b)) {
          s = b;

          if (m_blockRemaining > 0) {
            m_blockRemaining--;
          }

          // Stop playback at end of single-cycle block.
          if (m_blockRemaining == 0 && !m_autoInit) {
            m_playbackActive = false;
          }

          // Auto-init runs continuously until exit; keep consuming FIFO.
          if (m_blockRemaining == 0 && m_autoInit) {
            m_blockRemaining = 0xFFFFFFFFu;
          }
        } else {
          // FIFO underrun: output silence.
          s = 128;
        }
      }

      int16_t v = (int16_t)((int)s - 128) << 8;

      // Very simple master volume control (not accurate but useful).
      uint8_t vol = m_mixerRegs[0x22];
      v = (int16_t)(((int32_t)v * (int32_t)vol) / 255);

      out[i] = v;
    }

    size_t written = 0;
    i2s_write(m_i2sPort, out, sizeof(out), &written, portMAX_DELAY);
  }
}
