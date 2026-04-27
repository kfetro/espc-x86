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

// Pure synth core used by AdLib
class AdLibOPL2 {

public:

  AdLibOPL2();

  void init(int sampleRate);
  void reset();
  void setSampleRate(int sampleRate);

  // Global options
  void setWaveformSelectEnabled(bool enabled);

  // Channel controls
  void setChannelFreq(int ch, uint16_t fnum10, uint8_t block);
  void setChannelKeyOn(int ch, bool on);
  void setChannelFeedbackConn(int ch, uint8_t feedback, uint8_t conn);

  // Operator controls (per channel, carrier/mod)
  void setOperatorMult(int ch, bool carrier, uint8_t multIndex);
  void setOperatorSustainMode(int ch, bool carrier, bool sustain);
  void setOperatorLevelTL(int ch, bool carrier, uint8_t tl6);
  void setOperatorAD(int ch, bool carrier, uint8_t att4, uint8_t dec4);
  void setOperatorSR(int ch, bool carrier, uint8_t sus4, uint8_t rel4);
  void setOperatorWaveform(int ch, bool carrier, uint8_t wf2);

  // Generate mono signed 16-bit PCM
  void generate(int16_t *out, size_t frames);

private:

  // From a1k0n/opl2 (approx multiplier table)
  static const float kFreqMulTbl[16];

  // Tables (generated once)
  bool     m_tablesInited = false;
  uint16_t m_expTbl[256];
  uint16_t m_logSinTbl[512];
  uint16_t m_attackTbl[36];

  int  m_sampleRate = 44100;
  bool m_waveSelEnable = false;

  struct Operator {
    uint8_t waveform = 0;
    uint8_t feedback = 0;
    float   phase = 0.0f;     // 0..1024
    float   phaseIncr = 0.0f; // per sample
    int32_t last0 = 0;
    int32_t last1 = 0;

    void genMod(const int32_t *vol,
                int n,
                int32_t *out,
                const uint16_t *expT,
                const uint16_t *logSinT);

    void genCar(const int32_t *vol,
                const int32_t *mod,
                int n,
                int32_t *out,
                const uint16_t *expT,
                const uint16_t *logSinT);
  };

  struct Envelope {
    uint8_t  att = 0, dec = 0, sus = 0, rel = 0;
    uint32_t attackPhase = 0;
    uint8_t  mode = 3; // 0 A,1 D,2 S,3 R
    uint32_t attackInc = 0;
    float    decayInc = 0.0f;
    float    releaseInc = 0.0f;
    uint16_t sustainLevel = 4095;
    bool     keyed = false;
    bool     sustainMode = true;
    float    vol = 4095.0f;

    void recompute();
    void keyOn();
    void keyOff();
    void generate(uint16_t level, int n, int32_t *out, const uint16_t *attackT);
  };

  struct Channel {
    Operator mod;
    Operator car;
    Envelope menv;
    Envelope cenv;

    uint16_t fnum = 0;
    uint8_t  block = 0;
    bool     keyOn = false;
    uint8_t  connection = 0;

    uint16_t clevel = 0;
    uint16_t mlevel = 0;
    uint16_t level  = 0;

    float cmul = 1.0f;
    float mmul = 1.0f;

    void generate(int n,
                  int32_t *mix,
                  int32_t *s1,
                  int32_t *s2,
                  const uint16_t *expT,
                  const uint16_t *logSinT,
                  const uint16_t *attackT);
  };

  Channel m_ch[9];

  // Scratch buffers
  int32_t m_s1[512];
  int32_t m_s2[512];
  int32_t m_mix[512];

  void initTables();
  void resetChannel(int c);
  void updatePhaseIncr(int c);
};
