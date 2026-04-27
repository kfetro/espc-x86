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

#include "core/i8237_dma.h"

#include <stdio.h>
#include <string.h>

namespace core {

i8237A::i8237A()
{

}

i8237A::~i8237A()
{

}

void i8237A::init()
{
  flipflop = false;
}

void i8237A::reset()
{
  flipflop = false;
}

uint8_t i8237A::read(uint16_t address)
{
  switch (address) {

    // 8237A DMA Controller
    case 0x000 ... 0x007:
    {
      int ch = (address >> 1) & 0x3;       // canal 0..3
      bool isAddr = (address & 1) == 0;    // par=Address, impar=Count
      return isAddr ? dmaReadWordLSBMSB(curAddr[ch])
                    : dmaReadWordLSBMSB(curCount[ch]);
    }

    case 0x008: // Status
      return status;  // 0 basta para el POST de registros

    case 0x00A: // Mask register (lectura aceptada por clones)
      return mask;

    case 0x00B: // Mode
      // Si quisieras, podrías cachear “último canal escrito” para devolver su mode.
      return 0;

    // Page registers
    case 0x0083:
      return page[0];  // o page[1]; indistinto para el POST

    case 0x0081:
      return page[2];

    case 0x0082:
      return page[3];

    case 0x0087:
      return page[0];

    default:
      printf("dma8237a: Unhandled read port %04x\n", address);
      return 0xFF;
  }
}

void i8237A::write(uint16_t address, uint8_t value)
{
  switch (address) {

    // 0x00/01 CH0 Addr/Count
    // 0x02/03 CH1 Addr/Count
    // 0x04/05 CH2 Addr/Count
    // 0x06/07 CH3 Addr/Count
    case 0x000 ... 0x007:
    {
      int ch = (address >> 1) & 0x3;       // canal 0..3
      bool isAddr = (address & 1) == 0;    // par=Address, impar=Count
      if (isAddr)
        dmaWriteWordLSBMSB(curAddr[ch], value);
      else
        dmaWriteWordLSBMSB(curCount[ch], value);
      break;
    }

    case 0x008: // DMA_CMD / Status (escritura: no-op)
      break;

    case 0x009: // DMA_REQ (no usado)
      break;

    case 0x00A: // Single Channel Mask
      mask = value & 0x0F;
      break;

    case 0x00B: // Mode Register
      mode[value & 0x03] = value;
      break;

    case 0x00C: // Clear Flip-Flop
      dmaFlipFlopReset();
      break;

    case 0x00D: // Master Reset
      dmaFlipFlopReset();
      mask = 0x0F;
      memset(mode, 0, sizeof(mode));
      // (Puedes limpiar contadores si quieres, pero el POST no lo exige)
      break;

    case 0x00E: // Clear mask bit (canal en bits 1..0)
      mask &= ~(1 << (value & 0x03));
      break;

    case 0x00F: // Write mask
      mask = value & 0x0F;
      break;

    // DMA Page Registers (typically in XT)
    // 0x83: CH0 y CH1 comparten página
    case 0x0083:
      page[0] = value;  // CH0
      page[1] = value;  // CH1
      break;

    case 0x0081:
      page[2] = value;  // CH2
      break;

    case 0x0082:
      page[3] = value;  // CH3
      break;

    // 0x87: a veces “unused on PC”, pero si lo tocan, lo reflejamos en CH0 por compatibilidad
    case 0x0087:
      page[0] = value;
      break;

    default:
      printf("dma8237a: Unhandled write port %04x = %02x\n", address, value);
      break;
  }
}

uint8_t i8237A::dmaReadWordLSBMSB(uint16_t &reg16)
{
  uint8_t res = !flipflop ? (reg16 & 0x00FF) : (reg16 >> 8);
  flipflop = !flipflop;
  return res;
}

void i8237A::dmaWriteWordLSBMSB(uint16_t &reg16, uint8_t value)
{
  if (!flipflop) {
    reg16 = (reg16 & 0xFF00) | value;  // LSB
  } else {
    reg16 = (reg16 & 0x00FF) | (uint16_t(value) << 8);  // MSB
  }
  flipflop = !flipflop;
}

} // end of namespace
