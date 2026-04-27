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

#include "drivers/input/ps2_controller.h"
#include "drivers/input/keyboard.h"
#include "drivers/input/mouse.h"

#include <stdint.h>

/*
  Intel i8255 (PPI - Programmable Peripheral Interface)
  -----------------------------------------------------
  What it is:
    - The i8255 is a classic programmable I/O peripheral from Intel used to add
      general-purpose digital I/O (GPIO) to microprocessor systems.
    - It exposes three 8-bit ports (Port A, Port B, Port C) plus a Control register.
    - Each port can be configured as input or output, and Port C can be split
      into upper/lower nibbles with additional handshake/control roles.
    - Widely used in early PCs (IBM PC/XT) for keyboard interfacing, speaker control,
      and various system control signals.

  Memory/IO mapping (typical PC/XT):
    - 0x60 : Port A
    - 0x61 : Port B (also mixed with system control bits in XT motherboard logic)
    - 0x62 : Port C
    - 0x63 : Control register
    (Note: Mapping is system-specific; above is for IBM PC/XT. In general,
     these 4 consecutive addresses map to A/B/C/Control.)

  Ports overview:
    - Port A (PA7..PA0): 8-bit data port; can be input or output.
    - Port B (PB7..PB0): 8-bit data port; can be input or output.
    - Port C (PC7..PC0): 8-bit port; upper nibble (PC7..PC4) and lower nibble (PC3..PC0)
                         can be configured independently. In handshake modes,
                         certain bits act as control/status lines (e.g., STB#, IBF, OBF, ACK#, INTR).
    - Control register: Programs port directions, modes, and can do single-bit set/reset
                        operations on Port C (Bit Set/Reset mode).

  Operating modes:
    - Mode 0 (Basic I/O):
        * Simple input or output on each port without handshaking.
        * Port A, Port B, and each nibble of Port C can be independently configured.
        * No automatic strobes or interrupts; software polls or writes as needed.

    - Mode 1 (Strobed I/O with handshake, for Port A and/or Port B):
        * Adds handshake signals via Port C bits for input or output.
        * Typical signals in Mode 1:
            - For input: STB# (strobe in), IBF (input buffer full), INTR (interrupt request).
            - For output: OBF (output buffer full), ACK# (acknowledge), INTR.
        * Direction (input or output) is chosen per group:
            - Group A: Port A + PC upper nibble (PC7..PC4)
            - Group B: Port B + PC lower nibble (PC3..PC0)

    - Mode 2 (Bidirectional strobed bus, only for Port A / Group A):
        * Port A becomes a bidirectional bus with full handshake using PC upper nibble.
        * Useful for microprocessor-to-peripheral parallel links where both sides
          transmit and receive under hardware handshake.

  Grouping concept:
    - Group A = Port A + Port C upper (PC7..PC4).
    - Group B = Port B + Port C lower (PC3..PC0).
    - The control word configures each group’s mode and direction.

  Control word format (I/O mode set):
    - Written to the Control register with D7 = 1 to indicate "I/O mode set".
    - Bit layout (D7..D0):
        D7 = 1  --> Select I/O mode set (not BSR)
        D6..D5 = Mode for Group A (Port A): 00=Mode0, 01=Mode1, 1x=Mode2
        D4 = Port A direction: 1=input, 0=output
        D3 = Port C upper (PC7..PC4) direction: 1=input, 0=output
        D2 = Mode for Group B (Port B): 0=Mode0, 1=Mode1
        D1 = Port B direction: 1=input, 0=output
        D0 = Port C lower (PC3..PC0) direction: 1=input, 0=output
    - Example (Mode 0, all outputs): 1000 0000b = 0x80 (PA out, PB out, PC upper/lower out)
    - Example (Mode 0, all inputs):  1001 1111b = 0x9F (PA in, PB in, PC upper/lower in)

  Bit Set/Reset (BSR) mode for Port C:
    - The Control register also supports BSR to set/clear a single Port C bit without
      affecting the rest of the configuration.
    - BSR format (D7 = 0):
        D7 = 0  --> Select BSR operation
        D6..D4 = Don’t care (commonly 0)
        D3..D1 = Bit select (0..7) for PC0..PC7
        D0 = 1=set, 0=reset
    - Example: Set PC2  => 0000 0101b = 0x05
               Reset PC2=> 0000 0100b = 0x04
    - Important: BSR does NOT change port modes/directions; it only affects the chosen PC bit.

  Reset state:
    - After reset, all ports default to input mode for safety (tri-stated inputs).
    - Software must program the control word before driving outputs.

  Electrical/behavioral notes:
    - Ports are 5V TTL-compatible (original parts); check the specific clone/CMOS variant
      for drive strength and voltage.
    - In output mode, writing to Port A/B/C updates the output latch.
    - In input mode, reads return the current pin state (no effect on latches).
    - In Mode 1/2, specific Port C bits become handshake/status; the CPU should treat them
      as read-only status or write-only control depending on direction/mode.
    - Interrupt lines (INTR) are generated according to handshake state when enabled
      in Mode 1/2; actual interrupt wiring depends on the host system.

  Typical programming sequence (Mode 0, simple I/O):
    1) Write I/O mode control word to Control register (D7=1).
    2) If output ports are used, write data to Port A/B/C as needed.
    3) If input ports are used, read Port A/B/C; poll or handle via external logic.

  Example use cases:
    - IBM PC/XT:
        * Port A used for keyboard data capture (with software-driven timing).
        * Port B bit(s) tied to speaker and gate control (0x61 mixed function).
        * Port C used for status/control (NMI mask, keyboard clock/data sensing).
      (Exact wiring is motherboard-specific, but the 8255 is the central PPI.)

  Common pitfalls:
    - Forgetting to set the control word first (ports remain as inputs).
    - Overwriting the entire Control register when intending only to toggle one PC bit;
      use BSR for single-bit operations to avoid disturbing configurations.
    - Misconfiguring Mode 1/2 and assuming Port C bits are general-purpose when they’re
      actually reserved for handshake in those modes.
    - Assuming PC upper/lower direction follows Port A/B automatically; it is configured
      independently via the control word.

  Minimal pseudo-code sketch (I/O mode, Mode 0):
    // io_write(CTRL, 0x80);  // D7=1; Mode 0; PA out, PC upper out, PB out, PC lower out
    // io_write(PORT_A, 0x55);
    // uint8_t v = io_read(PORT_B);

  Summary:
    - The i8255 provides flexible 24-bit parallel I/O with optional handshake modes.
    - Configure groups (A/B) and Port C nibble directions via the control word.
    - Use BSR to set/reset individual Port C bits safely.
    - In early PCs, this chip was the backbone for simple peripherals and system control.
*/

using fabgl::PS2Controller;
using fabgl::Keyboard;
using fabgl::Mouse;

namespace core {

// ----------------------------------------------------------------------------
// Intel 8255 PPI (XT) - PC/XT-accurate emulation for FabGL PCEmulator
// ----------------------------------------------------------------------------
// Replaces AT i8042 with XT 8255 PPI ports (0x60..0x63). Keyboard scancodes
// are delivered on Port A (0x60) in Set-1. Host must ACK by pulsing PB7 (bit7
// of Port B / 0x61), as on real XT hardware.

class i8255 {

public:

  using InterruptCallback = bool (*)(void * context);
  using SysReqCallback    = bool (*)(void * context);

  i8255();
  ~i8255();

  void init();                   // initialize FabGL PS/2 controller
  void reset();                  // reset PPI state (all inputs)

  // Map I/O: address=0..3 => 0x60..0x63
  uint8_t read (int address);
  void    write(int address, uint8_t value);

  // Must be called by Machine after OUT 0x61 to detect PB7 ACK edge
  void onPort61Write(uint8_t oldValue, uint8_t newValue);

  // Periodic service: fetch scancodes and issue a one-shot IRQ1 per byte
  void tick();

  // BIOS helpers compatibility
  void setCallbacks(void * context, InterruptCallback keyboardIRQ, SysReqCallback sysReqCB) {
    m_context     = context;
    m_keyboardIRQ = keyboardIRQ;
    m_sysReq      = sysReqCB;
  }
  Keyboard * keyboard() { return m_keyboard; }
  Mouse    * mouse()    { return m_mouse; }
  void       enableMouse(bool value);

  // Configure XT DIP switch SW2
  void setSW2(uint8_t sw2LowNibble /*SW2[1..4]*/, bool sw2Bit5);

private:

  void writeControl(uint8_t v);
  uint8_t readPortA();
  uint8_t readPortB();
  uint8_t readPortC();
  void    writePortA(uint8_t v);
  void    writePortB(uint8_t v);
  void    writePortC(uint8_t v);

  // Keyboard helpers
  void fetchKeyboardIfPossible();
  void checkSysReq(int sc2);
  void ackKeyboardIfPB7Rising(uint8_t oldB, uint8_t newB);

  // --- PPI state ---
  uint8_t m_portA;
  uint8_t m_portB;
  uint8_t m_portC;

  uint8_t m_dirA;   // 1=input, 0=output
  uint8_t m_dirB;
  uint8_t m_dirCU;  // PC7..PC4
  uint8_t m_dirCL;  // PC3..PC0

  // XT DIP wiring
  uint8_t m_sw2LowNibble;
  bool    m_sw2Bit5;

  // --- FabGL input devices ---
  PS2Controller m_PS2;
  Keyboard   * m_keyboard;
  Mouse      * m_mouse;

  // Pending keyboard byte (XT Set-1) and OBF flag
  bool    m_kbOBF;
  uint8_t m_kbByte;

  // Break-prefix state (Set-2 0xF0 seen -> next Set-1 byte gets OR 0x80)
  bool    m_breakPrefix;

  // Callbacks
  void               * m_context;
  InterruptCallback    m_keyboardIRQ;
  SysReqCallback       m_sysReq;

  // Interrupt one-shot (count of pending triggers)
  int m_keybIntTrigs;

  bool m_sysReqArmed; // SysReq sequence tracker
};

} // end of namespace
