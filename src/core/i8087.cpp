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

#include "core/i8086.h"
#include "core/i8087.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <limits>

#if FPU_DEBUG
#define FPU_PRINT(...) printf(__VA_ARGS__)
#else
#define FPU_PRINT(...) ((void)0)
#endif

namespace fabgl {

using fabgl::i8086;

i8087::i8087()
{
  reset();
}

void i8087::reset()
{
  fpu_sp = 0;
  for (int i = 0; i < 8; ++i) {
    fpu[i] = 0.0;
  }

  // Clear status word (no flags set)
  m_statusWord  = 0;

  // Typical x87 default control word:
  // all exceptions masked, round-to-nearest, 64-bit precision.
  m_controlWord = 0x037F;

  // Tag Word: all registers empty (11b per entry -> 0xFFFF)
  m_tagWord = 0xFFFF;

  // Clear FPU IP/DP/opcode
  m_fpuIP      = 0;
  m_fpuCS      = 0;
  m_fpuOpcode  = 0;
  m_fpuDP      = 0;
  m_fpuDS      = 0;
}

// ST(i) accessor
double& i8087::st(int i)
{
  return fpu[(fpu_sp + i) & 7];
}

// Pop ST(0) with underflow check
void i8087::pop_st0()
{
  // Check if stack is empty (ST(0) tag == 11b)
  if (getTag(0) == 3) {
    m_statusWord |= (1u << 0); // IE (Invalid Operation)
    FPU_PRINT("FPU: stack underflow\n");
    return;
  }
  setTagEmpty(0);
  fpu_sp = (fpu_sp + 1) & 7;
}

// Push copy into ST(0) with overflow check
void i8087::push_copy(double v)
{
  // Check if stack is full (no empty registers)
  bool full = true;
  for (int i = 0; i < 8; ++i) {
    if (getTag(i) == 3) { full = false; break; }
  }
  if (full) {
    m_statusWord |= (1u << 0); // IE (Invalid Operation)
    FPU_PRINT("FPU: stack overflow\n");
    return;
  }
  fpu_sp = (fpu_sp - 1) & 7;
  st(0) = v;
  setTagFromValue(0);
}

// Load m32 (float)
double i8087::load_m32(uint32_t ea)
{
  uint32_t u = 0;
  u |= (uint32_t) i8086::RMEM16((int)ea + 0) << 0;
  u |= (uint32_t) i8086::RMEM16((int)ea + 2) << 16;
  float f;
  memcpy(&f, &u, sizeof(f));
  return (double) f;
}

// Load m64 (double)
double i8087::load_m64(uint32_t ea)
{
  uint64_t u = 0;
  u |= (uint64_t) i8086::RMEM16((int)ea + 0) << 0;
  u |= (uint64_t) i8086::RMEM16((int)ea + 2) << 16;
  u |= (uint64_t) i8086::RMEM16((int)ea + 4) << 32;
  u |= (uint64_t) i8086::RMEM16((int)ea + 6) << 48;
  double d;
  memcpy(&d, &u, sizeof(d));
  return d;
}

// Store m32 (float)
void i8087::store_m32(uint32_t ea, double v)
{
  float f = (float)v;
  uint32_t u;
  memcpy(&u, &f, sizeof(u));
  i8086::WMEM16((int)ea + 0, (uint16_t)(u & 0xFFFF));
  i8086::WMEM16((int)ea + 2, (uint16_t)((u >> 16) & 0xFFFF));
}

// Store m64 (double)
void i8087::store_m64(uint32_t ea, double v)
{
  uint64_t u;
  memcpy(&u, &v, sizeof(u));
  i8086::WMEM16((int)ea + 0, (uint16_t)(u & 0xFFFF));
  i8086::WMEM16((int)ea + 2, (uint16_t)((u >> 16) & 0xFFFF));
  i8086::WMEM16((int)ea + 4, (uint16_t)((u >> 32) & 0xFFFF));
  i8086::WMEM16((int)ea + 6, (uint16_t)((u >> 48) & 0xFFFF));
}

// Load m80 (extended real, 80 bits) - approximate using double
double i8087::load_m80(uint32_t ea)
{
  // Approximate implementation:
  // read the low 64 bits (first 8 bytes) and reinterpret as double,
  // ignoring the top 16 bits.
  uint64_t u = 0;
  u |= (uint64_t) i8086::RMEM16((int)ea + 0) << 0;
  u |= (uint64_t) i8086::RMEM16((int)ea + 2) << 16;
  u |= (uint64_t) i8086::RMEM16((int)ea + 4) << 32;
  u |= (uint64_t) i8086::RMEM16((int)ea + 6) << 48;

  double d;
  memcpy(&d, &u, sizeof(d));
  return d;
}

// Store m80 (extended real, 80 bits) - approximate using double
void i8087::store_m80(uint32_t ea, double v)
{
  // Approximate implementation:
  // write 64-bit double into low 8 bytes, zero high 16 bits.
  uint64_t u;
  memcpy(&u, &v, sizeof(u));

  i8086::WMEM16((int)ea + 0, (uint16_t)(u & 0xFFFF));
  i8086::WMEM16((int)ea + 2, (uint16_t)((u >> 16) & 0xFFFF));
  i8086::WMEM16((int)ea + 4, (uint16_t)((u >> 32) & 0xFFFF));
  i8086::WMEM16((int)ea + 6, (uint16_t)((u >> 48) & 0xFFFF));
  i8086::WMEM16((int)ea + 8, 0);
}

// Integer loads for FILD
int16_t i8087::load_i16(uint32_t ea)
{
  return (int16_t) i8086::RMEM16((int)ea);
}

int32_t i8087::load_i32(uint32_t ea)
{
  uint32_t lo = i8086::RMEM16((int)ea + 0);
  uint32_t hi = i8086::RMEM16((int)ea + 2);
  return (int32_t)((hi << 16) | lo);
}

int64_t i8087::load_i64(uint32_t ea)
{
  uint64_t u = 0;
  u |= (uint64_t) i8086::RMEM16((int)ea + 0) << 0;
  u |= (uint64_t) i8086::RMEM16((int)ea + 2) << 16;
  u |= (uint64_t) i8086::RMEM16((int)ea + 4) << 32;
  u |= (uint64_t) i8086::RMEM16((int)ea + 6) << 48;
  return (int64_t)u;
}

// Integer stores for FIST/FISTP
void i8087::store_i16(uint32_t ea, int16_t v)
{
  i8086::WMEM16((int)ea, (uint16_t)v);
}

void i8087::store_i32(uint32_t ea, int32_t v)
{
  i8086::WMEM16((int)ea + 0, (uint16_t)(v & 0xFFFF));
  i8086::WMEM16((int)ea + 2, (uint16_t)((v >> 16) & 0xFFFF));
}

void i8087::store_i64(uint32_t ea, int64_t v)
{
  i8086::WMEM16((int)ea + 0, (uint16_t)((v >>  0) & 0xFFFF));
  i8086::WMEM16((int)ea + 2, (uint16_t)((v >> 16) & 0xFFFF));
  i8086::WMEM16((int)ea + 4, (uint16_t)((v >> 32) & 0xFFFF));
  i8086::WMEM16((int)ea + 6, (uint16_t)((v >> 48) & 0xFFFF));
}

// BCD load (FBLD) – 80-bit packed BCD (18 digits)
long double i8087::load_bcd80(uint32_t ea)
{
  // Read 10 bytes (80 bits) in little-endian order.
  // The format is 18 packed BCD digits (least significant digit first),
  // plus a sign byte at the end.
  uint8_t bcd[10];
  for (int i = 0; i < 10; ++i) {
    bcd[i] = i8086::RMEM8((int)ea + i);
  }
  // Determine sign: bit 7 of the last byte (byte 9) is the sign (1 = negative)
  bool negative = (bcd[9] & 0x80) != 0;
  // The last byte's lower 7 bits are unused (or should be zero)
  long double value = 0.0L;
  long double factor = 1.0L;
  for (int i = 0; i < 9; ++i) {
    uint8_t byte = bcd[i];
    uint8_t low  = byte & 0x0F;
    uint8_t high = (byte >> 4) & 0x0F;
    value += low * factor;
    factor *= 10.0L;
    value += high * factor;
    factor *= 10.0L;
  }
  if (negative) value = -value;
  // Convert to double (loss of precision but acceptable for emulation)
  return (double)value;
}

// BCD store (FBSTP) – 80-bit packed BCD (18 digits)
void i8087::store_bcd80(uint32_t ea, long double v)
{
  bool negative = false;
  if (v < 0.0L) {
    negative = true;
    v = -v;
  }
  // Clamp to 18-digit range: 10^18 - 1
  long double max_bcd = 1.0L;
  for (int i = 0; i < 18; ++i) max_bcd *= 10.0L;
  max_bcd -= 1.0L;
  if (v > max_bcd) v = max_bcd; // overflow -> clamp (real 8087 would set exception)
  uint64_t int_part = (uint64_t)v; // floor
  uint8_t bcd[10] = {0};
  // Convert integer part to BCD (little-endian, least significant digit first)
  for (int i = 0; i < 18; ++i) {
    uint8_t digit = int_part % 10;
    int_part /= 10;
    int byte_index = i / 2;
    int nibble = (i % 2) ? 4 : 0;
    bcd[byte_index] |= (digit << nibble);
  }
  // Set sign in the last byte (bit 7)
  if (negative) bcd[9] |= 0x80;
  // Write back
  for (int i = 0; i < 10; ++i) {
    i8086::WMEM8((int)ea + i, bcd[i]);
  }
}

// Tag Word helpers --------------------------------------------------------

void i8087::setTag(int stIndex, uint16_t tag)
{
  int phys  = (fpu_sp + stIndex) & 7;
  int shift = phys * 2;
  m_tagWord = (uint16_t)((m_tagWord & ~(3u << shift)) | ((tag & 3u) << shift));
}

uint16_t i8087::getTag(int stIndex)
{
  int phys  = (fpu_sp + stIndex) & 7;
  int shift = phys * 2;
  return (uint16_t)((m_tagWord >> shift) & 3u);
}

void i8087::setTagEmpty(int stIndex)
{
  setTag(stIndex, 3u);
}

void i8087::setTagFromValue(int stIndex)
{
  double v = st(stIndex);
  if (isnan(v) || isinf(v)) {
    setTag(stIndex, 2u); // special
  } else if (v == 0.0) {
    setTag(stIndex, 1u); // zero
  } else {
    setTag(stIndex, 0u); // valid
  }
}

// Comparison flags helper: set C0, C2, C3 in m_statusWord according to a ? b
void i8087::setCompareFlags(double a, double b)
{
  // Clear C0, C2, C3 (bits 8, 10, 14)
  m_statusWord &= ~((1u << 8) | (1u << 10) | (1u << 14));

  // Unordered (NaN)
  if (isnan(a) || isnan(b)) {
    m_statusWord |= (1u << 8) | (1u << 10) | (1u << 14); // C0=1,C2=1,C3=1
    return;
  }

  if (a < b) {
    // ST < SRC  -> C0=1, C2=0, C3=0
    m_statusWord |= (1u << 8);
  } else if (a > b) {
    // ST > SRC  -> C0=0, C2=0, C3=0
  } else {
    // equal     -> C0=0, C2=0, C3=1
    m_statusWord |= (1u << 14);
  }
}

// Exception flags helper: update IE, OE, UE, optionally PE
void i8087::updateExceptionsFromResult(double result, double original, bool precisionPossible)
{
  // IE: Invalid operation (NaN)
  if (isnan(result)) {
    m_statusWord |= (1u << 0); // IE
    return;
  }

  // OE: Overflow (infinite)
  if (isinf(result)) {
    m_statusWord |= (1u << 3); // OE
    return;
  }

  // UE: Underflow (subnormal non-zero)
  if (result != 0.0 && fpclassify(result) == FP_SUBNORMAL) {
      m_statusWord |= (1u << 4); // UE
    }

  // PE: Precision (only if rounding could have occurred and the value changed)
  if (precisionPossible && result != original) {
    m_statusWord |= (1u << 5); // PE
  }
}

// Convenience to set precision flag when rounding changes the value
void i8087::setPrecisionFlagIfNeeded(double original, double rounded) {
  if (original != rounded) {
    m_statusWord |= (1u << 5); // PE
  }
}

// ---------------------------------------------------------------------------
// Round according to Control Word (RC bits) with ties-to-even for nearest
// RC bits (m_controlWord >> 10) & 3:
//   00 = round to nearest
//   01 = round down (toward -inf)
//   02 = round up (toward +inf)
//   03 = truncate (toward 0)
// ---------------------------------------------------------------------------
double i8087::roundToMode(double x)
{
  uint16_t rc = (m_controlWord >> 10) & 0x3;

  switch (rc) {
    case 0: // round to nearest, ties to even
      return nearbyint(x);
    case 1: // round down (toward -inf)
      return floor(x);
    case 2: // round up (toward +inf)
      return ceil(x);
    case 3: // truncate (toward zero)
      return trunc(x);
    default:
      return x;
  }
}

// Mark DE (Denormal Operand) when the input operand is subnormal
void i8087::checkDenormalOperand(double x)
{
  if (fpclassify(x) == FP_SUBNORMAL) {
    // DE = bit 1 in status word
    m_statusWord |= (1u << 1);
  }
}

// 8087 main execute (former case 69)
void i8087::execute(uint8_t  raw_opcode_id,
                    uint8_t  i_mod,
                    uint8_t  i_reg,
                    uint8_t  i_rm,
                    uint32_t rm_addr,
                    uint8_t  i_w,
                    const uint8_t* opcode_stream)
{
  uint8_t opcode = raw_opcode_id; // D8..DF
  uint8_t mod    = i_mod;
  uint8_t reg    = i_reg;
  uint8_t rm     = i_rm;

  // Update FPU instruction pointer and opcode (approximate)
  m_fpuIP     = i8086::IP();
  m_fpuCS     = i8086::CS();
  m_fpuOpcode = raw_opcode_id;

  // -----------------------------------------------------------------------
  // FNINIT / FINIT  - Initialize FPU
  // -----------------------------------------------------------------------
  // FNINIT: DB E3  (opcode = 0xDB, mod = 3, reg = 4, rm = 3)
  if (opcode == 0xDB && mod == 3 && reg == 4 && rm == 3) {
    reset();
    return;
  }

  // -----------------------------------------------------------------------
  // FWAIT (not an ESC instruction, but provided for completeness)
  // In real hardware, FWAIT (0x9B) waits for FPU. In emulation it's a NOP.
  // This function is only called for ESC D8-DF, so FWAIT is not here.
  // The CPU core (i8086) must handle 0x9B as a no-op.
  // -----------------------------------------------------------------------

  // -----------------------------------------------------------------------
  // FNSTSW / FSTSW — Store FPU Status Word
  //   DD /7 (mod != 3)         : FNSTSW m16
  //   DF E0 (mod=3, reg=4,rm=0): FNSTSW AX
  // -----------------------------------------------------------------------
  if (opcode == 0xDD && reg == 7 && mod != 3) {
    i8086::WMEM16((int)rm_addr, (uint16_t)m_statusWord);
    return;
  }
  if (opcode == 0xDF && mod == 3 && reg == 4 && rm == 0) {
    i8086::setAX((uint16_t)m_statusWord);
    return;
  }

  // -----------------------------------------------------------------------
  // FCLEX / FNCLEX – Clear FPU exception flags
  //   D9 E2 (mod=3, reg=4, rm=2) : FCLEX
  //   DB E2 (mod=3, reg=4, rm=2) : FNCLEX
  // -----------------------------------------------------------------------
  if ((opcode == 0xD9 || opcode == 0xDB) && mod == 3 && reg == 4 && rm == 2) {
    // Clear all exception flags in status word:
    // IE (0), DE (1), ZE (2), OE (3), UE (4), PE (5)
    m_statusWord &= ~((1u << 0) | (1u << 1) | (1u << 2) | (1u << 3) | (1u << 4) | (1u << 5));
    return;
  }

  // -----------------------------------------------------------------------
  // FLDCW - Load FPU Control Word
  // -----------------------------------------------------------------------
  if (opcode == 0xD9 && reg == 5 && mod != 3) {
    uint16_t cw = i8086::RMEM16((int)rm_addr);
    m_controlWord = cw;
    return;
  }

  // FSTCW m16  —  D9 /7  (mod != 3)
  if (opcode == 0xD9 && reg == 7 && mod != 3) {
    i8086::WMEM16((int)rm_addr, m_controlWord);
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // FNOP: D9 D0
  if (opcode == 0xD9 && mod == 3 && reg == 2 && rm == 0) {
    return;
  }

  // -----------------------------------------------------------------------
  // FLD ST(i): ESC D9 /0, mod=3
  // -----------------------------------------------------------------------
  if (opcode == 0xD9 && reg == 0 && mod == 3) {
    uint16_t tag = getTag(rm);
    double v = st(rm);
    push_copy(v);
    // Keep the original tag of the source
    setTag(0, tag);
    return;
  }

  // FSTP ST(i): ESC DD /3, mod=3
  if (opcode == 0xDD && reg == 3 && mod == 3) {
    st(rm) = st(0);
    setTagFromValue(rm);
    pop_st0();
    return;
  }

  // -----------------------------------------------------------------------
  // FCHS / FABS / FTST / FXAM: ESC D9, reg=4, mod=3, rm selects operation
  // -----------------------------------------------------------------------
  if (opcode == 0xD9 && mod == 3 && reg == 4) {
    switch (rm) {
      case 0: // D9 E0: FCHS
        st(0) = -st(0);
        setTagFromValue(0);
        return;

      case 1: // D9 E1: FABS
        st(0) = fabs(st(0));
        setTagFromValue(0);
        return;

      case 4: // D9 E4: FTST ST(0) vs 0.0
      {
        m_statusWord &= ~((1u << 8) | (1u << 10) | (1u << 14));
        uint16_t tag = getTag(0);
        if (tag == 3u) {
          // Empty: C3=1, C2=0, C0=1
          m_statusWord |= (1u << 14); // C3
          m_statusWord |= (1u << 8);  // C0
        } else {
          setCompareFlags(st(0), 0.0);
        }
        return;
      }

      case 5: // D9 E5: FXAM ST(0)
      {
        // FXAM sets C0,C1,C2,C3 based on class of ST(0) and its sign.
        m_statusWord &= ~((1u << 8) | (1u << 9) | (1u << 10) | (1u << 14));

        uint16_t tag = getTag(0);
        double v = st(0);
        bool negative = signbit(v);

        bool C0 = false;
        bool C1 = false;
        bool C2 = false;
        bool C3 = false;

        if (negative)
          C1 = true;

        if (tag == 3u) {
          // Empty: C3=1, C2=0, C0=1
          C3 = true;
          C0 = true;
        } else {
          int cls = fpclassify(v);
          switch (cls) {
            case FP_NAN:
              C0 = true;               // 0,0,1
              break;
            case FP_INFINITE:
              C2 = true;
              C0 = true;               // 0,1,1
              break;
            case FP_ZERO:
              C3 = true;               // 1,0,0
              break;
            case FP_SUBNORMAL:
              // 0,0,0
              break;
            case FP_NORMAL:
            default:
              C2 = true;               // 0,1,0
              break;
          }
        }

        if (C0) m_statusWord |= (1u << 8);
        if (C1) m_statusWord |= (1u << 9);
        if (C2) m_statusWord |= (1u << 10);
        if (C3) m_statusWord |= (1u << 14);
        return;
      }

      default:
        break;
    }
  }

  // -----------------------------------------------------------------------
  // FINCSTP / FDECSTP / FENI / FDISI (non‑arithmetic)
  // -----------------------------------------------------------------------
  // FINCSTP: D9 F7 (opcode D9, mod=3, reg=6, rm=7)
  if (opcode == 0xD9 && mod == 3 && reg == 6 && rm == 7) {
    fpu_sp = (fpu_sp + 1) & 7;   // increment TOP pointer
    return;
  }
  // FDECSTP: D9 F6 (opcode D9, mod=3, reg=6, rm=6)
  if (opcode == 0xD9 && mod == 3 && reg == 6 && rm == 6) {
    fpu_sp = (fpu_sp - 1) & 7;   // decrement TOP pointer
    return;
  }
  // FENI: DB E0 (enable interrupts) – no‑op in emulation
  if (opcode == 0xDB && mod == 3 && reg == 4 && rm == 0) {
    // ignore
    return;
  }
  // FDISI: DB E1 (disable interrupts) – no‑op in emulation
  if (opcode == 0xDB && mod == 3 && reg == 4 && rm == 1) {
    // ignore
    return;
  }

  // FXCH ST(i): ESC D9 /1, mod=3
  if (opcode == 0xD9 && reg == 1 && mod == 3) {
    double t = st(0);
    st(0) = st(rm);
    st(rm) = t;

    uint16_t tag0 = getTag(0);
    uint16_t tagi = getTag(rm);
    setTag(0,  tagi);
    setTag(rm, tag0);
    return;
  }

  // -----------------------------------------------------------------------
  // FSTENV / FNSTENV - Store FPU Environment (16-bit, 14 bytes)
  // -----------------------------------------------------------------------
  if (opcode == 0xD9 && reg == 6 && mod != 3) {
    int ea = (int) rm_addr;
    i8086::WMEM16(ea +  0, m_controlWord);
    i8086::WMEM16(ea +  2, m_statusWord);
    i8086::WMEM16(ea +  4, m_tagWord);
    i8086::WMEM16(ea +  6, m_fpuIP);
    i8086::WMEM16(ea +  8, m_fpuCS);
    i8086::WMEM16(ea + 10, m_fpuDP);
    i8086::WMEM16(ea + 12, m_fpuDS);
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // -----------------------------------------------------------------------
  // FLDENV - Load FPU Environment (16-bit, 14 bytes)
  // -----------------------------------------------------------------------
  if (opcode == 0xD9 && reg == 4 && mod != 3) {
    int ea = (int) rm_addr;
    m_controlWord = i8086::RMEM16(ea +  0);
    m_statusWord  = i8086::RMEM16(ea +  2);
    m_tagWord     = i8086::RMEM16(ea +  4);
    m_fpuIP       = i8086::RMEM16(ea +  6);
    m_fpuCS       = i8086::RMEM16(ea +  8);
    m_fpuDP       = i8086::RMEM16(ea + 10);
    m_fpuDS       = i8086::RMEM16(ea + 12);
    return;
  }

  // -----------------------------------------------------------------------
  // FSAVE / FNSAVE - Save FPU State (16-bit, 14 + 8*10 = 94 bytes)
  // -----------------------------------------------------------------------
  if (opcode == 0xDD && reg == 6 && mod != 3) {
    int ea = (int) rm_addr;
    // Save environment (14 bytes)
    i8086::WMEM16(ea +  0, m_controlWord);
    i8086::WMEM16(ea +  2, m_statusWord);
    i8086::WMEM16(ea +  4, m_tagWord);
    i8086::WMEM16(ea +  6, m_fpuIP);
    i8086::WMEM16(ea +  8, m_fpuCS);
    i8086::WMEM16(ea + 10, m_fpuDP);
    i8086::WMEM16(ea + 12, m_fpuDS);
    // Save ST(0..7) as 80-bit values (approximate)
    int regsBase = ea + 14;
    for (int i = 0; i < 8; ++i) {
      double v = st(i); // logical ST(i)
      store_m80((uint32_t)(regsBase + i * 10), v);
    }
    // After FSAVE, x87 is initialized
    reset();
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // -----------------------------------------------------------------------
  // FRSTOR - Restore FPU State (16-bit)
  // -----------------------------------------------------------------------
  if (opcode == 0xDD && reg == 4 && mod != 3) {
    int ea = (int) rm_addr;
    // Restore environment
    m_controlWord = i8086::RMEM16(ea +  0);
    m_statusWord  = i8086::RMEM16(ea +  2);
    m_tagWord     = i8086::RMEM16(ea +  4);
    m_fpuIP       = i8086::RMEM16(ea +  6);
    m_fpuCS       = i8086::RMEM16(ea +  8);
    m_fpuDP       = i8086::RMEM16(ea + 10);
    m_fpuDS       = i8086::RMEM16(ea + 12);
    // Restore ST(0..7)
    int regsBase = ea + 14;
    fpu_sp = 0;
    for (int i = 0; i < 8; ++i) {
      double v = load_m80((uint32_t)(regsBase + i * 10));
      st(i) = v;
      setTagFromValue(i);
    }
    return;
  }

  // -----------------------------------------------------------------------
  // FLD1, FLDL2T, FLDL2E, FLDPI, FLDLG2, FLDLN2, FLDZ
  // -----------------------------------------------------------------------
  if (opcode == 0xD9 && mod == 3 && reg == 5) {
    double c = 0.0;
    switch (rm) {
      case 0: // FLD1
        c = 1.0;
        break;
      case 1: // FLDL2T
        c = log(10.0) / log(2.0);
        break;
      case 2: // FLDL2E
        c = 1.0 / log(2.0);
        break;
      case 3: // FLDPI
        c = 3.14159265358979323846;
        break;
      case 4: // FLDLG2
        c = log10(2.0);
        break;
      case 5: // FLDLN2
        c = log(2.0);
        break;
      case 6: // FLDZ
        c = 0.0;
        break;
      default:
        FPU_PRINT("8087 unimplemented: ESC D9 /5 rm=%d (FLD const)\n", rm);
        return;
    }
    push_copy(c);
    return;
  }

  // -----------------------------------------------------------------------
  // FBLD / FBSTP - BCD load/store (80-bit packed BCD)
  // -----------------------------------------------------------------------
  // FBLD m80: DF /4 (opcode DF, reg=4, mod != 3)
  if (opcode == 0xDF && reg == 4 && mod != 3) {
    long double v = load_bcd80(rm_addr);
    push_copy((double)v);
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }
  // FBSTP m80: DF /6 (opcode DF, reg=6, mod != 3)
  if (opcode == 0xDF && reg == 6 && mod != 3) {
    long double v = st(0);
    store_bcd80(rm_addr, v);
    pop_st0();
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // -----------------------------------------------------------------------
  // Transcendentals ESC D9, mod=3 (reg=6,7)
  // -----------------------------------------------------------------------
  if (opcode == 0xD9 && mod == 3) {
    // F2XM1: ST(0) = 2^ST(0) - 1
    if (reg == 6 && rm == 0) {
      double old = st(0);
      st(0) = pow(2.0, st(0)) - 1.0;
      updateExceptionsFromResult(st(0), old, true);
      setTagFromValue(0);
      return;
    }

    // FYL2X: ST(1) = ST(1) * log2(ST(0)); pop ST(0)
    if (reg == 6 && rm == 1) {
      double x = st(0);
      double y = st(1);
      checkDenormalOperand(x);
      checkDenormalOperand(y);
      double old = st(1);
      st(1) = y * (log(x) / log(2.0));
      updateExceptionsFromResult(st(1), old, true);
      setTagFromValue(1);
      pop_st0();
      return;
    }

    // FPTAN: D9 F2 (reg=6, rm=2)
    if (reg == 6 && rm == 2) {
      double x = st(0);
      checkDenormalOperand(x);
      double t = tan(x);
      push_copy(1.0);  // push 1.0 as second result
      st(1) = t;
      st(0) = 1.0;
      setTagFromValue(0);
      setTagFromValue(1);
      updateExceptionsFromResult(t, x, true);
      return;
    }

    // FPATAN: D9 F3 (reg=6, rm=3)
    if (reg == 6 && rm == 3) {
      double x = st(0);
      double y = st(1);
      checkDenormalOperand(x);
      checkDenormalOperand(y);
      double old = st(1);
      st(1) = atan2(y, x);
      updateExceptionsFromResult(st(1), old, true);
      setTagFromValue(1);
      pop_st0();
      return;
    }

    // FXTRACT: extract exponent and significand
    if (reg == 6 && rm == 4) {
      double x = st(0);
      int e = 0;
      double m = frexp(x, &e); // x = m * 2^e, 0.5 <= |m| < 1
      push_copy(m);            // push significand first
      st(0) = (double)e;       // exponent becomes new ST(0)
      setTagFromValue(0);
      setTagFromValue(1);
      return;
    }

    // FPREM1: partial remainder (approximate) with IEEE remainder
    if (reg == 6 && rm == 5) {
      double x = st(0);
      double y = st(1);
      if (y == 0.0) {
        m_statusWord |= (1u << 2); // ZE
        st(0) = NAN;
        return;
      }
      double q = roundToMode(x / y); // nearest integer
      double r = x - q * y;
      // Set C0, C1, C2, C3 according to specification (simplified)
      m_statusWord &= ~((1u<<8)|(1u<<9)|(1u<<10)|(1u<<14));
      if (r == 0.0) m_statusWord |= (1u<<14); // C3 exact
      if (fabs(q) != 0.0) {
        // C1 indicates if the quotient was rounded up (odd)
        if (fmod(fabs(q), 2.0) != 0.0) m_statusWord |= (1u<<9);
      }
      st(0) = r;
      setTagFromValue(0);
      updateExceptionsFromResult(r, x, true);
      return;
    }

    // FPREM: partial remainder (classic)
    if (reg == 7 && rm == 0) {
      double x = st(0);
      double y = st(1);
      if (y == 0.0) {
        m_statusWord |= (1u << 2); // ZE
        st(0) = NAN;
        return;
      }
      double q = trunc(x / y); // truncate toward zero
      double r = x - q * y;
      m_statusWord &= ~((1u<<8)|(1u<<9)|(1u<<10)|(1u<<14));
      if (r == 0.0) m_statusWord |= (1u<<14);
      if (fabs(q) != 0.0) {
        if (fmod(fabs(q), 2.0) != 0.0) m_statusWord |= (1u<<9);
      }
      st(0) = r;
      setTagFromValue(0);
      updateExceptionsFromResult(r, x, true);
      return;
    }

    // FYL2XP1: ST(1) = ST(1) * log2(ST(0)+1); pop ST(0)
    if (reg == 7 && rm == 1) {
      double x = st(0);
      double y = st(1);
      checkDenormalOperand(x);
      checkDenormalOperand(y);
      double old = st(1);
      st(1) = y * (log(x + 1.0) / log(2.0));
      updateExceptionsFromResult(st(1), old, true);
      setTagFromValue(1);
      pop_st0();
      return;
    }

    // FSQRT: ST(0) = sqrt(ST(0))
    if (reg == 7 && rm == 2) {
      double x = st(0);
      checkDenormalOperand(x);
      if (x < 0.0) {
        m_statusWord |= (1u << 0); // IE
        st(0) = NAN;
      } else {
        double old = st(0);
        st(0) = sqrt(x);
        updateExceptionsFromResult(st(0), old, true);
        setTagFromValue(0);
      }
      return;
    }

    // FSINCOS: ST(0)=cos(x), push ST(1)=sin(x)
    if (reg == 7 && rm == 3) {
      double x = st(0);
      checkDenormalOperand(x);
      double s = sin(x);
      double c = cos(x);
      push_copy(c);
      st(1) = s;
      st(0) = c;
      setTagFromValue(0);
      setTagFromValue(1);
      updateExceptionsFromResult(s, x, true);
      updateExceptionsFromResult(c, x, true);
      return;
    }

    // FRNDINT: D9 FC (reg=7, rm=4)
    if (reg == 7 && rm == 4) {
      double v  = st(0);
      double vr = roundToMode(v);
      setPrecisionFlagIfNeeded(v, vr);
      st(0) = vr;
      setTagFromValue(0);
      updateExceptionsFromResult(vr, v, false);
      return;
    }

    // FSCALE: ST(0) = ST(0) * 2^floor(ST(1))
    if (reg == 7 && rm == 5) {
      double x = st(0);
      double y = st(1);
      checkDenormalOperand(x);
      checkDenormalOperand(y);

      // Handle NaN and infinity in the exponent
      if (isnan(y)) {
        m_statusWord |= (1u << 0); // IE
        st(0) = NAN;
        setTagFromValue(0);
        return;
      }
      if (isinf(y)) {
        // If y is infinite, result depends on x and sign of y
        if (x == 0.0) {
          st(0) = NAN; // 0 * 2^inf -> invalid
          m_statusWord |= (1u << 0); // IE
        } else if (isinf(x)) {
          st(0) = x; // keep infinity
        } else {
          // y = +inf -> scale up to infinity, y = -inf -> scale down to 0
          st(0) = (y > 0) ? copysign(INFINITY, x) : 0.0;
        }
        setTagFromValue(0);
        updateExceptionsFromResult(st(0), x, true);
        return;
      }

      int n = (int) floor(y);
      double old = st(0);
      st(0) = ldexp(x, n);
      updateExceptionsFromResult(st(0), old, true);
      setTagFromValue(0);
      return;
    }

    // FSIN: ST(0) = sin(ST(0))
    if (reg == 7 && rm == 6) {
      double x = st(0);
      checkDenormalOperand(x);
      double old = st(0);
      st(0) = sin(x);
      updateExceptionsFromResult(st(0), old, true);
      setTagFromValue(0);
      return;
    }

    // FCOS: ST(0) = cos(ST(0))
    if (reg == 7 && rm == 7) {
      double x = st(0);
      checkDenormalOperand(x);
      double old = st(0);
      st(0) = cos(x);
      updateExceptionsFromResult(st(0), old, true);
      setTagFromValue(0);
      return;
    }
  }

  // -----------------------------------------------------------------------
  // FLD / FST / FSTP (MEM) - m32, m64, m80
  // -----------------------------------------------------------------------

  // FLD m32: D9 /0
  if (opcode == 0xD9 && reg == 0 && mod != 3) {
    double v = load_m32(rm_addr);
    push_copy(v);
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    checkDenormalOperand(v);
    return;
  }

  // FLD m64: DD /0
  if (opcode == 0xDD && reg == 0 && mod != 3) {
    double v = load_m64(rm_addr);
    push_copy(v);
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    checkDenormalOperand(v);
    return;
  }

  // FLD m80real: DB /5
  if (opcode == 0xDB && reg == 5 && mod != 3) {
    double v = load_m80(rm_addr);
    push_copy(v);
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    checkDenormalOperand(v);
    return;
  }

  // FST m32: D9 /2
  if (opcode == 0xD9 && reg == 2 && mod != 3) {
    store_m32(rm_addr, st(0));
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // FST m64: DD /2
  if (opcode == 0xDD && reg == 2 && mod != 3) {
    store_m64(rm_addr, st(0));
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // FST m80real: DD /6
  if (opcode == 0xDD && reg == 6 && mod != 3) {
    store_m80(rm_addr, st(0));
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // FSTP m32: D9 /3
  if (opcode == 0xD9 && reg == 3 && mod != 3) {
    store_m32(rm_addr, st(0));
    pop_st0();
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // FSTP m64: DD /3
  if (opcode == 0xDD && reg == 3 && mod != 3) {
    store_m64(rm_addr, st(0));
    pop_st0();
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // FSTP m80real: DB /7
  if (opcode == 0xDB && reg == 7 && mod != 3) {
    store_m80(rm_addr, st(0));
    pop_st0();
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // -----------------------------------------------------------------------
  // FILD - Load integer into FPU
  // -----------------------------------------------------------------------

  // FILD m16: DF /0
  if (opcode == 0xDF && reg == 0 && mod != 3) {
    int16_t v = load_i16(rm_addr);
    push_copy((double)v);
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // FILD m32: DB /0
  if (opcode == 0xDB && reg == 0 && mod != 3) {
    int32_t v = load_i32(rm_addr);
    push_copy((double)v);
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // FILD m64: DF /5
  if (opcode == 0xDF && reg == 5 && mod != 3) {
    int64_t v = load_i64(rm_addr);
    push_copy((double)v);
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // -----------------------------------------------------------------------
  // FIST / FISTP - Store integer
  // -----------------------------------------------------------------------

  // FIST m16: DF /2
  if (opcode == 0xDF && reg == 2 && mod != 3) {
    double v = st(0);
    double vr = roundToMode(v);
    setPrecisionFlagIfNeeded(v, vr);
    int16_t iv = (int16_t)vr;
    store_i16(rm_addr, iv);
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // FIST m16/m32: DB /2 (if i_w == 0 -> 16-bit, i_w == 1 -> 32-bit)
  if (opcode == 0xDB && reg == 2 && mod != 3) {
    double v  = st(0);
    double vr = roundToMode(v);
    setPrecisionFlagIfNeeded(v, vr);
    if (i_w == 0) {
      int16_t iv = (int16_t) vr;
      store_i16(rm_addr, iv);
    } else {
      int32_t iv = (int32_t) vr;
      store_i32(rm_addr, iv);
    }
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // FISTP m16: DF /3
  if (opcode == 0xDF && reg == 3 && mod != 3) {
    double v = st(0);
    double vr = roundToMode(v);
    setPrecisionFlagIfNeeded(v, vr);
    int16_t iv = (int16_t)vr;
    store_i16(rm_addr, iv);
    pop_st0();
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // FISTP m16/m32: DB /3
  if (opcode == 0xDB && reg == 3 && mod != 3) {
    double v  = st(0);
    double vr = roundToMode(v);
    setPrecisionFlagIfNeeded(v, vr);
    if (i_w == 0) {
      int16_t iv = (int16_t) vr;
      store_i16(rm_addr, iv);
    } else {
      int32_t iv = (int32_t) vr;
      store_i32(rm_addr, iv);
    }
    pop_st0();
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // FISTP m64: DF /7
  if (opcode == 0xDF && reg == 7 && mod != 3) {
    double v  = st(0);
    double vr = roundToMode(v);
    setPrecisionFlagIfNeeded(v, vr);
    int64_t iv = (int64_t) vr;
    store_i64(rm_addr, iv);
    pop_st0();
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // -----------------------------------------------------------------------
  // FIADD / FISUB / FISUBR / FIMUL / FIDIV / FIDIVR / FICOM / FICOMP
  // -----------------------------------------------------------------------
  if ((opcode == 0xDE || opcode == 0xDA) && mod != 3) {
    double src;
    if (opcode == 0xDE) {
      int16_t v16 = load_i16(rm_addr);
      src = (double)v16;
    } else {
      int32_t v32 = load_i32(rm_addr);
      src = (double)v32;
    }
    checkDenormalOperand(st(0));
    checkDenormalOperand(src);
    double old = st(0);
    switch (reg & 7) {
      case 0: // FIADD
        st(0) = st(0) + src;
        updateExceptionsFromResult(st(0), old, true);
        setTagFromValue(0);
        break;
      case 1: // FIMUL
        st(0) = st(0) * src;
        updateExceptionsFromResult(st(0), old, true);
        setTagFromValue(0);
        break;
      case 2: // FICOM
        setCompareFlags(st(0), src);
        break;
      case 3: // FICOMP
        setCompareFlags(st(0), src);
        pop_st0();
        break;
      case 4: // FISUB
        st(0) = st(0) - src;
        updateExceptionsFromResult(st(0), old, true);
        setTagFromValue(0);
        break;
      case 5: // FISUBR
        st(0) = src - st(0);
        updateExceptionsFromResult(st(0), old, true);
        setTagFromValue(0);
        break;
      case 6: // FIDIV
        if (src == 0.0) {
          m_statusWord |= (1u << 2); // ZE (Zero divide)
          st(0) = copysign(INFINITY, st(0));
          // No OE set
        } else {
          st(0) = st(0) / src;
          updateExceptionsFromResult(st(0), old, true);
        }
        setTagFromValue(0);
        break;
      case 7: // FIDIVR
        if (st(0) == 0.0) {
          if (src == 0.0) {
            m_statusWord |= (1u << 0); // IE
            st(0) = NAN;
          } else {
            m_statusWord |= (1u << 2); // ZE
            st(0) = copysign(INFINITY, src);
          }
        } else {
          st(0) = src / st(0);
          updateExceptionsFromResult(st(0), old, true);
        }
        setTagFromValue(0);
        break;
      default:
        FPU_PRINT("8087 unimplemented FIxxx: ESC %02X /%d (mod=%d rm=%d)\n", opcode, reg, mod, rm);
        break;
    }
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // -----------------------------------------------------------------------
  // Arithmetic REG-REG (D8/DC, mod=3) including FCOM/FCOMP/FCOMPP/FFREE/FxxP
  // -----------------------------------------------------------------------
  if (mod == 3) {
    // D8 mod=3 - ST0 op ST(i)
    if (opcode == 0xD8) {
      checkDenormalOperand(st(0));
      checkDenormalOperand(st(rm));
      double old = st(0);
      switch (reg & 7) {
        case 0: st(0) = st(0) + st(rm); break; // FADD
        case 1: st(0) = st(0) * st(rm); break; // FMUL
        case 2: setCompareFlags(st(0), st(rm)); break; // FCOM
        case 3: setCompareFlags(st(0), st(rm)); pop_st0(); break; // FCOMP
        case 4: st(0) = st(0) - st(rm); break; // FSUB
        case 5: st(0) = st(rm) - st(0); break; // FSUBR
        case 6: st(0) = st(0) / st(rm); break; // FDIV
        case 7: st(0) = st(rm) / st(0); break; // FDIVR
      }
      if ((reg & 7) != 2 && (reg & 7) != 3) {
        updateExceptionsFromResult(st(0), old, true);
        setTagFromValue(0);
      }
      return;
    }

    // DC mod=3 - ST(i) op ST0
    if (opcode == 0xDC) {
      checkDenormalOperand(st(0));
      checkDenormalOperand(st(rm));
      double old = st(rm);
      switch (reg & 7) {
        case 0: st(rm) = st(rm) + st(0); break;
        case 1: st(rm) = st(rm) * st(0); break;
        case 2: setCompareFlags(st(rm), st(0)); break;
        case 3: setCompareFlags(st(rm), st(0)); pop_st0(); break;
        case 4: st(rm) = st(rm) - st(0); break;
        case 5: st(rm) = st(0) - st(rm); break;
        case 6: st(rm) = st(rm) / st(0); break;
        case 7: st(rm) = st(0) / st(rm); break;
      }
      if ((reg & 7) != 2 && (reg & 7) != 3) {
        updateExceptionsFromResult(st(rm), old, true);
        setTagFromValue(rm);
      }
      return;
    }

    // FCOMPP: ESC DE D9 (mod=3, reg=3, rm=1)
    if (opcode == 0xDE && reg == 3 && rm == 1) {
      setCompareFlags(st(1), st(0));
      pop_st0(); // pop ST(0)
      pop_st0(); // pop ST(1)
      return;
    }

    // FADDP/FMULP/FSUBP/FSUBRP/FDIVP/FDIVRP ST(i), ST(0)
    if (opcode == 0xDE) {
      double old = st(rm);
      switch (reg & 7) {
        case 0: // FADDP ST(i), ST(0)
          st(rm) = st(rm) + st(0);
          break;
        case 1: // FMULP ST(i), ST(0)
          st(rm) = st(rm) * st(0);
          break;
        case 4: // FSUBP ST(i), ST(0)
          st(rm) = st(rm) - st(0);
          break;
        case 5: // FSUBRP ST(i), ST(0)
          st(rm) = st(0) - st(rm);
          break;
        case 6: // FDIVP ST(i), ST(0)
          st(rm) = st(rm) / st(0);
          break;
        case 7: // FDIVRP ST(i), ST(0)
          st(rm) = st(0) / st(rm);
          break;
        default:
          FPU_PRINT("8087 unimplemented: ESC DE /%d (FxxP) mod=%d rm=%d\n", reg, mod, rm);
          break;
      }
      updateExceptionsFromResult(st(rm), old, true);
      setTagFromValue(rm);
      pop_st0();
      return;
    }

    // FFREE ST(i): ESC DD /0 (mod=3)
    if (opcode == 0xDD && reg == 0 && mod == 3) {
      setTagEmpty(rm);
      return;
    }

    FPU_PRINT("8087 unimplemented: ESC %02X / %d (mod=%d rm=%d) reg-reg\n", opcode, reg, mod, rm);
    return;
  }

  // -----------------------------------------------------------------------
  // Arithmetic - MEMORY OPERANDS (D8 m32, DC m64), including FCOM/FCOMP
  // -----------------------------------------------------------------------

  // D8 m32real
  if (opcode == 0xD8 && mod != 3) {
    double m = load_m32(rm_addr);
    checkDenormalOperand(st(0));
    checkDenormalOperand(m);
    double old = st(0);
    switch (reg & 7) {
      case 0: st(0) = st(0) + m; break;
      case 1: st(0) = st(0) * m; break;
      case 2: setCompareFlags(st(0), m); break;
      case 3: setCompareFlags(st(0), m); pop_st0(); break;
      case 4: st(0) = st(0) - m; break;
      case 5: st(0) = m - st(0); break;
      case 6: st(0) = st(0) / m; break;
      case 7: st(0) = m / st(0); break;
    }
    if ((reg & 7) != 2 && (reg & 7) != 3) {
      updateExceptionsFromResult(st(0), old, true);
      setTagFromValue(0);
    }
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // DC m64real
  if (opcode == 0xDC && mod != 3) {
    double m = load_m64(rm_addr);
    checkDenormalOperand(st(0));
    checkDenormalOperand(m);
    double old = st(0);
    switch (reg & 7) {
      case 0: st(0) = st(0) + m; break;
      case 1: st(0) = st(0) * m; break;
      case 2: setCompareFlags(st(0), m); break;
      case 3: setCompareFlags(st(0), m); pop_st0(); break;
      case 4: st(0) = st(0) - m; break;
      case 5: st(0) = m - st(0); break;
      case 6: st(0) = st(0) / m; break;
      case 7: st(0) = m / st(0); break;
    }
    if ((reg & 7) != 2 && (reg & 7) != 3) {
      updateExceptionsFromResult(st(0), old, true);
      setTagFromValue(0);
    }
    m_fpuDP = (uint16_t)(rm_addr & 0xFFFF);
    m_fpuDS = (uint16_t)((rm_addr >> 4) & 0xFFFF);
    return;
  }

  // -----------------------------------------------------------------------
  // Any other 8087 opcode
  // -----------------------------------------------------------------------
  FPU_PRINT("8087 unimplemented: ESC %02X / %d (mod=%d rm=%d)\n", opcode, reg, mod, rm);
}

} // end of namespace
