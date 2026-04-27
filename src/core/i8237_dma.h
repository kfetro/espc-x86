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

/*
The Intel 8237A is a classic Direct Memory Access (DMA) controller used in early
IBM PC-class systems (PC/XT/AT). Its purpose is to move data between I/O devices
and system memory without making the CPU handle every byte. It can temporarily
take control of the system bus to transfer blocks efficiently, reducing CPU
overhead and improving throughput.

Key roles:
  - Offload bulk data movement (e.g., floppy, audio, serial/parallel, NICs).
  - Reduce CPU cycles spent on programmed I/O.
  - Provide predictable transfers via single, block, or demand modes.

Hardware overview

* Channels:
  - 4 independent DMA channels (0–3) per 8237A.
  - PC/XT: one 8237A (4 × 8-bit channels).
  - PC/AT: two cascaded 8237A controllers; channels 0–3 (8-bit), 5–7 (16-bit);
    channel 4 used for cascade between the two chips.

* Width:
  - 8237A natively handles 8-bit transfers. AT-class systems use a second
    controller and glue logic to support 16-bit channels on the 16-bit data bus.

* Bus control signals:
  - HRQ (Hold Request): DMA asks CPU for the bus.
  - HLDA (Hold Acknowledge): CPU grants bus to DMA.
  - DREQn / DACKn: Per-channel device request / acknowledge handshake.
  - MEMR / MEMW, IOR / IOW: Memory and I/O read/write strobes driven by DMA.
  - EOP / TC (End Of Process / Terminal Count): Signals the final transfer of a
    programmed block (internal on counter rollover or external via EOP).

* Addressing:
  - Each channel has 16-bit Current Address and 16-bit Current Count registers.
  - On IBM PCs, "page registers" external to the 8237A hold high address bits
    to extend beyond 64 KiB (effective address = (Page << 16) | BaseAddress).

DMA transfer types (programmed via the Mode Register)

1) Single Transfer (Cycle Steal):
   - Transfers one byte/word per bus grant, then releases the bus.
   - Minimizes CPU starvation.

2) Block Transfer (Burst):
   - Holds the bus to transfer the entire block in one burst.
   - Maximizes throughput; CPU may be stalled during the burst.

3) Demand Transfer:
   - Continues transferring while the device keeps DREQ asserted.
   - Stops on DREQ deassertion or when Terminal Count (TC) is reached.

4) Cascade:
   - A channel forwards requests to a secondary DMA controller (used on AT).

Transfer direction and address update

* Read (I/O → Memory): Device is the source (IOR active), memory is the sink (MEMW).
* Write (Memory → I/O): Memory is the source (MEMR), device is the sink (IOW).

* After each transfer, the memory address auto-increments or auto-decrements
  (programmable), and the count decrements. When the count underflows past 0x0000
  (i.e., after N+1 transfers if N-1 was programmed), the controller asserts TC.

Auto-initialize vs Single-shot

* Single-shot (Auto-init = 0):
  - After TC, the channel stops. Software must reprogram address/count.

* Auto-initialize (Auto-init = 1):
  - After TC, the controller reloads current address/count from base registers,
    enabling repeated buffer cycles without CPU intervention.

Priority and timing

• Priority:
  - Fixed priority by default: CH0 highest → CH3 lowest.
  - Optional rotating priority: After service, the serviced channel becomes
    lowest priority to share bandwidth fairly.

* Timing styles:
  - "Cycle stealing" (single mode) alternates DMA and CPU bus usage.
  - "Burst" (block mode) grants the bus to DMA for the whole block.

Registers (per chip; accessed via I/O ports on PC-compatible systems)

* Per-channel:
  - Base/Current Address (16-bit).
  - Base/Current Count (16-bit; program N-1 for N transfers).

* Global:
  - Command Register: Enables DMA, options (incl. memory-to-memory on some impl.).
  - Mode Register: Channel select, Read/Write/Verify, Single/Block/Demand/Cascade,
    auto-init, address increment/decrement.
  - Mask Register(s): Mask/unmask channels.
  - Request Register: Software-driven requests for testing/forcing transfers.
  - Status Register: TC flags and request status.

Note:
  - On PC/XT/AT, separate "page registers" provide A16–A23 for full memory
    addressing; these must be programmed alongside the 8237A’s 16-bit address.

Handshake flow (example: single-transfer, I/O → memory)

1) Device asserts DREQn to request service on channel n.
2) 8237A asserts HRQ to obtain the system bus from the CPU.
3) CPU completes the current bus cycle and asserts HLDA.
4) 8237A performs one DMA cycle:
   - Asserts IOR to read a byte/word from the device’s I/O port.
   - Asserts MEMW to write that data into RAM at CurrentAddress.
5) 8237A updates CurrentAddress (inc/dec) and decrements CurrentCount.
6) If CurrentCount underflows, TC is set; if auto-init is enabled, base values
   are reloaded. Otherwise, the channel becomes idle.
7) In single mode, 8237A releases the bus (CPU resumes). In block mode, it may
   keep the bus for the next transfer.
8) The device deasserts DREQn or the transfer ends at TC/EOP.

Programming sequence (high-level on a PC-like system)

1) Mask the channel (prevent activity while programming).
2) Clear the internal byte pointer flip-flop (resets low/high byte toggle).
3) Write Base Address (low byte, then high byte).
4) Clear the flip-flop again.
5) Write Base Count (low byte, then high byte). Remember: program N-1.
6) Program the Page Register for that channel (upper address bits).
7) Write the Mode Register (channel, direction, mode, auto-init, inc/dec).
8) Unmask the channel (enable).
9) Configure the device so it asserts DREQ when ready; set device I/O ports.
10) Optionally integrate with interrupts for completion (or poll TC/EOP).

Special considerations

* Verify mode:
  - Updates address/count without actual memory or I/O data movement
    (useful for timing/test).

* Memory-to-memory:
  - Supported by the 8237 family using two channels (one source, one destination),
    but not always enabled/connected in PC designs.

* 64 KiB boundaries:
  - Because the internal address is 16-bit, transfers must not cross a 64 KiB
    segment unless software splits the transfer or updates the page register.

* AT cascade:
  - On the IBM AT, channel 4 cascades to the secondary controller for 16-bit DMA
    (channels 5–7). Use the correct controller and page registers per channel.

Why it mattered

The 8237A enabled efficient I/O on early PCs by letting peripherals move data
with minimal CPU involvement. Concepts like channelized requests, bus arbitration
(HRQ/HLDA), auto-initialized ring buffers, and terminal count became foundational
to later DMA architectures in PCs and other systems.
*/

namespace core {

// 8237A DMA Controller
class i8237A {

public:

   i8237A();
  ~i8237A();

  void init();
  void reset();

  // I/O ports
  uint8_t read(uint16_t address);
  void    write(uint16_t address, uint8_t value);

private:

  // 8237A DMA minimal stub

  // Direcciones y contadores por canal (16 bits, acceso por LSB/MSB alterno)
  uint16_t baseAddr[4]  = {0};
  uint16_t curAddr[4]   = {0};
  uint16_t baseCount[4] = {0};
  uint16_t curCount[4]  = {0};

  // Registros misceláneos
  uint8_t  mask         = 0x0F;   // todos enmascarados por defecto
  uint8_t  mode[4]      = {0};
  uint8_t  status       = 0;      // TC bits opcional
  uint8_t  temp         = 0;      // no lo usarás realmente

  // Flip‑flop global para la secuencia LSB/MSB
  bool     flipflop     = false;

  // Page registers (solo 4 usados típicamente: ch0, ch1, ch2, ch3)
  uint8_t  page[4]      = {0};    // puertos 0x87, 0x83, 0x81, 0x82 en PC/XT clásico

  inline void dmaFlipFlopReset() { flipflop = false; }

  uint8_t dmaReadWordLSBMSB(uint16_t &reg16);
  void    dmaWriteWordLSBMSB(uint16_t &reg16, uint8_t value);

};

} // end of namespace
