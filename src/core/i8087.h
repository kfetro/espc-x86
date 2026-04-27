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

namespace fabgl {

// 8087 Math Coprocessor emulation class.
// This class is invoked from the 8086 core when an ESC D8–DF opcode is decoded.
class i8087 {
public:
  i8087();

  // 8087 Math Coprocessor (ESC D8–DF)
  // Parameters:
  // - raw_opcode_id : opcode D8..DF
  // - i_mod, i_reg, i_rm : ModR/M fields
  // - rm_addr : effective address for memory operands (linear address)
  // - i_w : operand size flag (0 = 16-bit, 1 = 32-bit) used by FIST/FISTP
  // - opcode_stream : pointer to opcode bytes (for debug output)
  void execute(uint8_t  raw_opcode_id,
               uint8_t  i_mod,
               uint8_t  i_reg,
               uint8_t  i_rm,
               uint32_t rm_addr,
               uint8_t  i_w,
               const uint8_t* opcode_stream);

  // Reset FPU stack and internal state.
  void reset();

private:
  // FPU stack: ST(0) = fpu[(fpu_sp) & 7]
  double   fpu[8];
  int      fpu_sp;

  // Minimal FPU Status, Control and Tag Words.
  //  - m_statusWord: exception flags and C0/C2/C3 compare flags.
  //  - m_controlWord: current control word (loaded by FLDCW).
  //  - m_tagWord    : 2 bits per physical register (0..7):
  //        00 = valid, 01 = zero, 10 = special (NaN/INF), 11 = empty.
  uint16_t m_statusWord;
  uint16_t m_controlWord;
  uint16_t m_tagWord;

  // FPU instruction/data pointers and last opcode (16-bit model).
  // Used by FSAVE/FRSTOR and FLDENV/FSTENV/FNSTENV.
  uint16_t m_fpuIP;      // last ESC opcode IP offset
  uint16_t m_fpuCS;      // last ESC opcode CS segment
  uint16_t m_fpuOpcode;  // last ESC opcode (low 8 bits)

  uint16_t m_fpuDP;      // last data pointer offset
  uint16_t m_fpuDS;      // last data pointer segment (approximate)

  // ST(i) accessor
  double& st(int i);

  // Pop ST(0)
  void pop_st0();

  // Push a copy into ST(0)
  void push_copy(double v);

  // Float/double loads and stores
  double load_m32(uint32_t ea);
  double load_m64(uint32_t ea);
  void   store_m32(uint32_t ea, double v);
  void   store_m64(uint32_t ea, double v);

  // Approximate 80-bit real support:
  // Treats the 80-bit memory operand as if it were a 64-bit double stored
  // in the low 8 bytes, ignoring the top 16 bits. This is NOT fully IEEE-754
  // compliant, but can be replaced later with a real 80-bit implementation.
  double load_m80(uint32_t ea);
  void   store_m80(uint32_t ea, double v);

  // Integer loads for FILD
  int16_t  load_i16(uint32_t ea);
  int32_t  load_i32(uint32_t ea);
  int64_t  load_i64(uint32_t ea);

  // Integer stores for FIST/FISTP
  void store_i16(uint32_t ea, int16_t v);
  void store_i32(uint32_t ea, int32_t v);
  void store_i64(uint32_t ea, int64_t v);

  // Tag Word helpers
  void     setTag(int stIndex, uint16_t tag);  // 0=valid,1=zero,2=special,3=empty
  uint16_t getTag(int stIndex);
  void     setTagEmpty(int stIndex);
  void     setTagFromValue(int stIndex);

  // Comparison flags helper (C0,C2,C3)
  void setCompareFlags(double a, double b);

  // Exception flags helper (IE, OE, UE, PE)
  void updateExceptionsFromResult(double result);

  // Round according to Control Word (RC bits)
  double roundToMode(double x);

  // Mark DE (Denormal Operand) when x is subnormal
  void checkDenormalOperand(double x);
};

} // end of namespace
