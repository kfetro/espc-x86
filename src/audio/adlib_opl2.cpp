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

#include "audio/adlib_opl2.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const float AdLibOPL2::kFreqMulTbl[16] = {
  0.5f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f,
  8.f, 9.f, 10.f, 10.f, 12.f, 12.f, 15.f, 15.f
};

AdLibOPL2::AdLibOPL2()
{
}

void AdLibOPL2::init(int sampleRate)
{
  if (!m_tablesInited) {
    initTables();
    m_tablesInited = true;
  }
  m_sampleRate = (sampleRate > 0) ? sampleRate : 44100;
  reset();
}

void AdLibOPL2::setSampleRate(int sampleRate)
{
  if (sampleRate <= 0 || sampleRate == m_sampleRate)
    return;
  m_sampleRate = sampleRate;
  for (int c = 0; c < 9; ++c)
    updatePhaseIncr(c);
}

void AdLibOPL2::reset()
{
  m_waveSelEnable = false;
  for (int c = 0; c < 9; ++c)
    resetChannel(c);
}

void AdLibOPL2::setWaveformSelectEnabled(bool enabled)
{
  m_waveSelEnable = enabled;
  if (!m_waveSelEnable) {
    for (int c = 0; c < 9; ++c) {
      m_ch[c].mod.waveform = 0;
      m_ch[c].car.waveform = 0;
    }
  }
}

void AdLibOPL2::setChannelFreq(int ch, uint16_t fnum10, uint8_t block)
{
  if (ch < 0 || ch >= 9)
    return;
  m_ch[ch].fnum = fnum10 & 0x3FF;
  m_ch[ch].block = block & 0x07;
  updatePhaseIncr(ch);
}

void AdLibOPL2::setChannelKeyOn(int ch, bool on)
{
  if (ch < 0 || ch >= 9)
    return;
  if (on && !m_ch[ch].keyOn) {
    m_ch[ch].keyOn = true;
    m_ch[ch].menv.keyOn();
    m_ch[ch].cenv.keyOn();
  } else if (!on && m_ch[ch].keyOn) {
    m_ch[ch].keyOn = false;
    m_ch[ch].menv.keyOff();
    m_ch[ch].cenv.keyOff();
  }
}

void AdLibOPL2::setChannelFeedbackConn(int ch, uint8_t feedback, uint8_t conn)
{
  if (ch < 0 || ch >= 9)
    return;
  m_ch[ch].mod.feedback = feedback & 0x07;
  m_ch[ch].connection = conn & 0x01;
}

void AdLibOPL2::setOperatorMult(int ch, bool carrier, uint8_t multIndex)
{
  if (ch < 0 || ch >= 9)
    return;
  float m = kFreqMulTbl[multIndex & 0x0F];
  if (carrier)
    m_ch[ch].cmul = m;
  else
    m_ch[ch].mmul = m;
  updatePhaseIncr(ch);
}

void AdLibOPL2::setOperatorSustainMode(int ch, bool carrier, bool sustain)
{
  if (ch < 0 || ch >= 9)
    return;
  if (carrier)
    m_ch[ch].cenv.sustainMode = sustain;
  else
    m_ch[ch].menv.sustainMode = sustain;
}

void AdLibOPL2::setOperatorLevelTL(int ch, bool carrier, uint8_t tl6)
{
  if (ch < 0 || ch >= 9)
    return;
  uint16_t lvl = (uint16_t) (tl6 & 0x3F) << 5;
  if (carrier)
    m_ch[ch].clevel = lvl;
  else
    m_ch[ch].mlevel = lvl;
}

void AdLibOPL2::setOperatorAD(int ch, bool carrier, uint8_t att4, uint8_t dec4)
{
  if (ch < 0 || ch >= 9)
    return;
  Envelope &e = carrier ? m_ch[ch].cenv : m_ch[ch].menv;
  e.att = att4 & 0x0F;
  e.dec = dec4 & 0x0F;
  e.recompute();
}

void AdLibOPL2::setOperatorSR(int ch, bool carrier, uint8_t sus4, uint8_t rel4)
{
  if (ch < 0 || ch >= 9)
    return;
  Envelope &e = carrier ? m_ch[ch].cenv : m_ch[ch].menv;
  e.sus = sus4 & 0x0F;
  e.rel = rel4 & 0x0F;
  e.recompute();
}

void AdLibOPL2::setOperatorWaveform(int ch, bool carrier, uint8_t wf2)
{
  if (ch < 0 || ch >= 9)
    return;
  uint8_t wf = m_waveSelEnable ? (wf2 & 0x03) : 0;
  if (carrier)
    m_ch[ch].car.waveform = wf;
  else
    m_ch[ch].mod.waveform = wf;
}

void AdLibOPL2::generate(int16_t *out, size_t frames)
{
  while (frames) {
    int n = (frames > 512) ? 512 : (int) frames;
    memset(m_mix, 0, n * sizeof(int32_t));

    for (int c = 0; c < 9; ++c) {
      if (!m_ch[c].keyOn && m_ch[c].cenv.mode == 3 && m_ch[c].cenv.vol >= 4095.0f)
        continue;
      m_ch[c].generate(n, m_mix, m_s1, m_s2, m_expTbl, m_logSinTbl, m_attackTbl);
    }

    for (int i = 0; i < n; ++i) {
      int32_t v = (m_mix[i] * 32767) / 4096;
      if (v < -32768) v = -32768;
      if (v >  32767) v =  32767;
      out[i] = (int16_t)v;
    }

    out += n;
    frames -= n;
  }
}

void AdLibOPL2::Operator::genMod(const int32_t *vol,
                                 int n,
                                 int32_t *out,
                                 const uint16_t *expT,
                                 const uint16_t *logSinT)
{
  float p = phase;
  float dp = phaseIncr;
  int32_t w1 = last1;
  int32_t w  = last0;

  int feedbackShift = 31;
  if (feedback > 0)
    feedbackShift = 9 - feedback;

  for (int i = 0; i < n; ++i) {
    int32_t fb = (feedbackShift >= 31) ? 0 : ((w + w1) >> feedbackShift);
    int m = (int) (p + (float) fb);
    w1 = w;

    int l;
    int32_t s = 0;

    switch (waveform) {
      default:
      case 0:
        l = (int) logSinT[m & 511] + vol[i];
        if (l >= 7936) s = 0;
        else s = (int32_t)(expT[l & 0xFF] >> (l >> 8));
        if (m & 512) s = -s;
        break;
      case 1:
        if (m & 512) {
          l = (int) logSinT[m & 511] + vol[i];
          if (l >= 7936) s = 0;
          else s = (int32_t)(expT[l & 0xFF] >> (l >> 8));
        }
        break;
      case 2:
        l = (int) logSinT[m & 511] + vol[i];
        if (l >= 7936) s = 0;
        else s = (int32_t)(expT[l & 0xFF] >> (l >> 8));
        break;
      case 3:
        if (m & 256) {
          l = (int) logSinT[m & 255] + vol[i];
          if (l >= 7936) s = 0;
          else s = (int32_t)(expT[l & 0xFF] >> (l >> 8));
        }
        break;
    }

    w = s;
    p += dp;
    out[i] = w;
  }

  phase = fmodf(p, 1024.0f);
  last1 = w1;
  last0 = w;
}

void AdLibOPL2::Operator::genCar(const int32_t *vol,
                                 const int32_t *mod,
                                 int n,
                                 int32_t *out,
                                 const uint16_t *expT,
                                 const uint16_t *logSinT)
{
  float p = phase;
  float dp = phaseIncr;

  for (int i = 0; i < n; ++i) {
    int m = (int)(p + (float) mod[i]);
    int l;
    int32_t s = 0;

    switch (waveform) {
      default:
      case 0:
        l = (int)logSinT[m & 511] + vol[i];
        if (l <= 7935) s = (int32_t)(expT[l & 0xFF] >> (l >> 8));
        if (m & 512) s = -s;
        break;
      case 1:
        if (m & 512) {
          l = (int)logSinT[m & 511] + vol[i];
          if (l <= 7935) s = (int32_t)(expT[l & 0xFF] >> (l >> 8));
        }
        break;
      case 2:
        l = (int)logSinT[m & 511] + vol[i];
        if (l <= 7935) s = (int32_t)(expT[l & 0xFF] >> (l >> 8));
        break;
      case 3:
        if (m & 256) {
          l = (int)logSinT[m & 255] + vol[i];
          if (l <= 7935) s = (int32_t)(expT[l & 0xFF] >> (l >> 8));
        }
        break;
    }

    p += dp;
    out[i] += s;
  }

  phase = fmodf(p, 1024.0f);
}

void AdLibOPL2::Envelope::recompute()
{
  attackInc = 1u << att;
  decayInc = (float) (1u << dec) / 768.0f;
  releaseInc = (float) (1u << rel) / 768.0f;
  sustainLevel = (uint16_t)(sus << 7);
}

void AdLibOPL2::Envelope::keyOn()
{
  attackPhase = 0;
  mode = 0;
  keyed = true;
  vol = 4095.0f;
}

void AdLibOPL2::Envelope::keyOff()
{
  keyed = false;
}

void AdLibOPL2::Envelope::generate(uint16_t level, int n, int32_t *out, const uint16_t *attackT)
{
  int offset = 0;
  float v = vol;
  uint16_t susLvl = sustainLevel;

  while (offset < n) {
    if (mode == 0) {
      while (offset < n && attackPhase < 8192u * 36u) {
        v = (float) attackT[attackPhase >> 13];
        attackPhase += attackInc;
        out[offset++] = (int32_t)v + (int32_t)level;
      }
      if (attackPhase >= 8192u * 36u) {
        mode = 1;
        v = 0.0f;
      }
    } else if (mode == 1) {
      while (offset < n) {
        out[offset++] = (int32_t)v + (int32_t)level;
        v += decayInc;
        if (v >= susLvl) {
          v = (float)susLvl;
          mode = 2;
          break;
        }
      }
    } else if (mode == 2) {
      if (!keyed || !sustainMode) {
        mode = 3;
        continue;
      }
      while (offset < n) {
        out[offset++] = (int32_t)susLvl + (int32_t)level;
      }
    } else {
      while (offset < n) {
        out[offset++] = (int32_t)v + (int32_t)level;
        v += releaseInc;
        if (v > 4095.0f)
          v = 4095.0f;
      }
    }
  }

  vol = v;
}

void AdLibOPL2::Channel::generate(int n,
                                  int32_t *mix,
                                  int32_t *s1,
                                  int32_t *s2,
                                  const uint16_t *expT,
                                  const uint16_t *logSinT,
                                  const uint16_t *attackT)
{
  menv.generate(mlevel, n, s1, attackT);
  mod.genMod(s1, n, s1, expT, logSinT);

  cenv.generate((uint16_t)(clevel + level), n, s2, attackT);

  if (connection == 0) {
    car.genCar(s2, s1, n, mix, expT, logSinT);
  } else {
    for (int i = 0; i < n; ++i)
      mix[i] += s1[i];
    static int32_t zeroMod[512];
    car.genCar(s2, zeroMod, n, mix, expT, logSinT);
  }
}

void AdLibOPL2::initTables()
{
  for (int i = 0; i < 256; ++i) {
    float v = 2.0f * powf(2.0f, 1.0f - i / 256.0f) * 1024.0f + 0.5f;
    if (v < 0) v = 0;
    if (v > 65535) v = 65535;
    m_expTbl[i] = (uint16_t)v;
  }
  for (int i = 0; i < 512; ++i) {
    float s = sinf((i + 0.5f) * (float) M_PI / 512.0f);
    if (s < 1e-12f) s = 1e-12f;
    float v = -logf(s) / logf(2.0f) * 256.0f + 0.5f;
    if (v < 0) v = 0;
    if (v > 65535) v = 65535;
    m_logSinTbl[i] = (uint16_t)v;
  }
  int x = 512;
  for (int i = 0; i < 36; ++i) {
    m_attackTbl[i] = (uint16_t)(8 * x);
    x -= (x >> 3) + 1;
    if (x < 0) x = 0;
  }
}

void AdLibOPL2::resetChannel(int c)
{
  m_ch[c] = Channel();
  updatePhaseIncr(c);
}

void AdLibOPL2::updatePhaseIncr(int c)
{
  float f_scale = 14313180.0f / (288.0f * (float) m_sampleRate);
  uint16_t fnum = m_ch[c].fnum & 0x3FF;
  uint8_t block = m_ch[c].block & 0x07;
  float incr = ((float) (fnum << block)) / 1024.0f;
  m_ch[c].car.phaseIncr = incr * m_ch[c].cmul * f_scale;
  m_ch[c].mod.phaseIncr = incr * m_ch[c].mmul * f_scale;
}
