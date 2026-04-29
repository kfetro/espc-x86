/*
 * Based on FabGL - ESP32 Graphics Library
 * Original project by Fabrizio Di Vittorio
 * https://github.com/fdivitto/FabGL
 *
 * Original Copyright (c) 2019-2022 Fabrizio Di Vittorio
 *
 * Modifications and further development:
 * Copyright (c) 2026 Jesus Martinez-Mateo
 * Author: Jesus Martinez-Mateo <jesus.martinez.mateo@gmail.com>
 *
 * This file is part of a derived work from FabGL
 * and is distributed under the terms of the
 * GNU General Public License version 3 or later.
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

#include "core/i8086.h"
#include "core/i8259_pic.h"
#include "core/i8253_pit.h"
#include "core/i8042.h"
#include "core/mc146818.h"
#include "core/i8250_uart.h"
#include "core/i8237_dma.h"
#include "core/i8255_ppi.h"
#include "core/i8272_fdc.h"

#include "drivers/audio/sound_generator.h"
#include "drivers/comm/mcp23S17.h"

// Video card or graphics adapter
#include "video/video_system.h"
#include "video/scanout_context.h"

// Sound cards
#include "audio/adlib.h"
#include "audio/adlib_wavegen.h"

// Joystick
#include "drivers/joystick/gameport.h"
#include "drivers/joystick/gamepad.h"
#include "drivers/joystick/bt_driver.h"

#include "bios.h"

#include <stdint.h>

#define RAM_SIZE             1048576    // must correspond to bios MEMSIZE

// maximum number of serial ports
#define SERIALPORTS          2

// Core IBM PC
using core::i8237A;
using core::i8259;
using core::i8253;
using core::i8255;
using core::i8042;
using core::MC146818;
using core::i8250;
using core::i8272;

// Drivers Comm
using fabgl::MCP23S17;
using fabgl::SerialPort;

// Drivers Audio
using fabgl::SoundGenerator;
using fabgl::SineWaveformGenerator;

// Video card
using video::VideoSystem;
using video::ScanoutContext;

#ifdef FABGL_EMULATED
typedef void (*StepCallback)(void *);
#endif
  
typedef void (*SysReqCallback)(uint8_t reqId);

class Computer {

public:

   Computer();
  ~Computer();

  void setBaseDirectory(char const *value)     { m_baseDir = value; }

  void setDriveImage(int drive, char const *filename, int cylinders = 0, int heads = 0, int sectors = 0);
  
  bool diskChanged(int drive)                  { return m_diskChanged[drive]; }
  void resetDiskChanged(int drive)             { m_diskChanged[drive] = false; }

  void setBootDrive(int drive)                 { m_bootDrive = drive; }

  void setSysReqCallback(SysReqCallback value) { m_sysReqCallback = value; }
  
  void setCOM1(SerialPort *serialPort);
  void setCOM2(SerialPort *serialPort);

  void run();

  void pause()                         { m_paused = true;  }
  void resume()                        { m_paused = false; }
  bool paused() const                  { return m_paused; }

  void reboot()                        { m_reset = true; }

  uint32_t ticksCounter()              { return m_ticksCounter; }

#if LEGACY_IBMPC_XT_8088
  i8255 *getI8255()                    { return &m_i8255; }
#else
  i8042 *getI8042()                    { return &m_i8042; }
#endif

  MC146818 *getMC146818()              { return &m_MC146818; }

  uint8_t *memory()                    { return s_memory; }

  FILE *disk(int index)                { return m_disk[index]; }
  char const *diskFilename(int index)  { return m_diskFilename[index]; }
  uint64_t diskSize(int index)         { return m_diskSize[index]; }
  uint16_t diskCylinders(int index)    { return m_diskCylinders[index]; }
  uint8_t diskHeads(int index)         { return m_diskHeads[index]; }
  uint8_t diskSectors(int index)       { return m_diskSectors[index]; }

  void dumpMemory(char const *filename);
  void dumpInfo(char const *filename);

  #ifdef FABGL_EMULATED
  void setStepCallback(StepCallback value)  { m_stepCallback = value; }
  #endif

  // Audio controls
  void audio_toggleMute();
  void audio_volumeUp();
  void audio_volumeDown();

  // Video controls
  ScanoutContext *video_suspend(int mode) { return m_video.suspend(mode); }
  void video_resume() { m_video.resume(); }
  void video_snapshot();

private:

  static void runTask(void *pvParameters);

  void init();
  void reset();

  void tick();

  // I/O Ports
  static void writePort(void *context, int address, uint8_t value);
  static uint8_t readPort(void *context, int address);

  // Video Memory
  static void writeVideoMemory8(void *context, int address, uint8_t value);
  static void writeVideoMemory16(void *context, int address, uint16_t value);
  static uint8_t readVideoMemory8(void *context, int address);
  static uint16_t readVideoMemory16(void *context, int address);

  static bool interrupt(void *context, int num);

  static bool MC146818Interrupt(void *context);
  
  static bool COM1Interrupt(i8250 *source, void *context);
  static bool COM2Interrupt(i8250 *source, void *context);

  static bool keyboardInterrupt(void *context);
  static bool mouseInterrupt(void *context);

  static bool triggerReset(void *context);
  static void hostReq(void *context, uint8_t reqId);

  void autoDetectDriveGeometry(int drive);

  void printEquipmentWord();

  #ifdef FABGL_EMULATED
  StepCallback            m_stepCallback;
  #endif

  volatile bool           m_reset;
  volatile bool           m_paused;

  BIOS                    m_BIOS;

  // 0, 1 = floppy
  // >= 2 = hard disk
  char                   *m_diskFilename[DISKCOUNT];
  bool                    m_diskChanged[DISKCOUNT];
  FILE                   *m_disk[DISKCOUNT];
  uint64_t                m_diskSize[DISKCOUNT];
  uint16_t                m_diskCylinders[DISKCOUNT];
  uint8_t                 m_diskHeads[DISKCOUNT];
  uint8_t                 m_diskSectors[DISKCOUNT];

  // Main Memory (RAM)
  static uint8_t         *s_memory;

  // Intel 8237A Direct Memory Access (DMA)
  i8237A                  m_DMA;

  // Intel 8253 Programmable Interval Timers (PIT)
  // Timer 0: drives the system timer interrupt (IRQ0) through the PIC,
  //          providing the 18.2 Hz system tick.
  // Timer 2: gated by port 0x61 and feeds the PC speaker,
  //          generating square‑wave audio signals.
  i8253                   m_PIT;

  // Intel 8259 Programmable Interrupt Controllers (PIC)
  i8259                   m_PIC_master;
  i8259                   m_PIC_slave;

#if LEGACY_IBMPC_XT_8088
  // Intel i8255 Programmable Peripheral Interface (PPI)
  i8255                   m_i8255;
#else
  // 8042 PS/2 Keyboard Controller
  i8042                   m_i8042;
#endif

  TaskHandle_t            m_taskHandle;

  uint32_t                m_ticksCounter;

  VideoSystem             m_video;

  // Speaker/audio
  bool                    m_speakerDataEnable;
  SoundGenerator          m_soundGen;
  SineWaveformGenerator   m_waveGen;
  //SquareWaveformGenerator m_waveGen;

  // CMOS & RTC
  MC146818                m_MC146818;

  // extended I/O (MCP23S17)
  MCP23S17                m_MCP23S17;
  uint8_t                 m_MCP23S17Sel;

  uint8_t                 m_bootDrive;

  SysReqCallback          m_sysReqCallback;

  char const             *m_baseDir;
  
  // serial ports
  i8250                   m_COM1;
  i8250                   m_COM2;

  // Floppy Disk Controller (FDC)
  i8272                   m_FDC;

  // Sound card
  AdLib                  *m_adlib;
  AdLibWaveformGenerator *m_adlibGen;

  // Joystick
  GamePort                m_gameport;
  BTGamepad              *m_gamepad;
  BTGamepadDriver        *m_btDriver;

  uint8_t                 m_systemControl;

  uint8_t m_dipPB; // DIP switches (Port B)

  static bool FDC_IRQ6_Thunk(void *context);

  static void PIT_IRQ0(void *context, int timer, bool value);
};
