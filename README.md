![ESPC-x86](doc/ESPC.png)

# ESPC-x86

<p align="center">
  <b>PC/XT 8086 ESP32-Emulation</b><br>
  An IBM PC/XT 8086 emulator for the ESP32
</p>

<p align="center">
  #what-is-espc-x86 •
  #hardware •
  #features •
  #installation •
  #controls •
  #licenseLicense
</p>

---

## What is ESPC‑x86?

**ESPC‑x86** (PC/XT 8086 ESP32‑Emulation) is an emulator of a classic **IBM PC/XT (8086)** computer running on an **ESP32** microcontroller.

The project is specially designed for the **LilyGo TTGo VGA32** board. With ESPC‑x86, the ESP32 becomes a fully functional retro PC capable of running classic **DOS software** and **Games**, recreating the experience of an early IBM‑compatible computer using inexpensive modern hardware.

## Hardware

ESPC‑86 is primarily developed and tested on the following board:

### Supported board

- **LilyGo TTGo VGA32** https://lilygo.cc/en-us/products/fabgl-vga32
  - ESP32-WROVER-E module with 4MB Flash, 8MB PSRAM, 520KB SRAM
  - Micro-USB for power and programming/debug
  - VGA video output
  - 2 x PS/2 ports (keyboard and mouse)
  - Micro SD card
  - Buzzer connection 2.0mm 2-pin terminal https://es.aliexpress.com/item/1005005699690954.html
  - Audio jack 3.5mm connector

## Features

- Custom [GLaBIOS](https://github.com/640-KB/GLaBIOS) PC/XT compatible BIOS.
- Intel 8086 processor (CPU) emulation compatible with PC/XT era software.
- Intel 8087 math coprocessor (FPU) emulation (experimental)
- PS/2 keyboard and mouse support.
- CGA graphics card emulation (40×25 and 80×25 text modes, 320x200 4‑color and 640x200 2‑color graphics modes).
- Hercules/monochrome (HGC/MDA) graphics card emulation with 720x348 monochrome graphics mode.
- Tandy graphics emulation supporting 160x200 and 320x200 graphics modes with 16 colors.
- Partial EGA graphics card emulation (up to 128 KB VRAM, 320x200 16-colors graphics mode).
- PC Speaker / buzzer sound emulation.
- AdLib (OPL2) sound card emulation.

## Installation

The easiest way to install ESPC‑x86 is using the **online firmware flasher**.

### Web installer (recommended)

1. Connect your **TTGo VGA32** board to your computer via USB
2. Open the following link in a Chromium‑based browser (Chrome, Edge, Brave, etc.):
   https://aristoteles.dma.fi.upm.es/espc-86/
3. Follow the instructions on the webpage to flash the firmware

> Make sure no other application is using the serial port before flashing.

### Windows installation

1. Download and open the [Flash Download Tools](https://www.espressif.com/en/support/download/other-tools) from Espressif.
2. Select **ESP32** as chip type and choose **Develop** as work mode.
3. Select the correct **COM port**.
4. Load the firmware `.bin` file at address `0x0` and start flashing.

## Controls

### Keyboard

- **Ctrl+F1: Main OSD Menu** (System configuration and machine setup)
- **Ctrl+F2 / F3: Mount Floppy/Hard Disk** (Opens a file browser to mount images, supports .zip files)
- **Ctrl+F5: Mute**
- **Ctrl+F6 / F7: Volume Down/Up** (Displays an OSD level indicator in text mode)
- **Ctrl+F11: Hard Reset** (Physical reset button simulation)
- **Ctrl+F12: Soft Reboot** (Warm boot / Ctrl+Alt+Del)

> **Note on Mounting:** After mounting a floppy image, the OSD closes and returns to the emulation. However, mounting a hard disk image will automatically trigger a system reboot to apply the changes.

## Project Status

ESPC‑x86 is under active development.

The project is:
- **Usable**, featuring an intuitive interface and a straightforward setup.
- **Extensible**, allowing for easy integration of new features and hardware components.
- **Intended** for experimentation, learning, and retro‑computing fun.

Feedback, testing, and contributions are welcome.

## References and Credits

This project is a **fork and continuation of FabGL**, and includes code derived from **8086tiny**.

- [FabGL](https://github.com/fdivitto/FabGL) Copyright (c) Fabrizio Di Vittorio http://www.fabgl.com
- [8086tiny](https://github.com/adriancable/8086tiny) Copyright (c) Adrian Cable, licensed under the MIT License.
- [GLaBIOS](https://github.com/640-KB/GLaBIOS)

ESPC‑86 extends, modifies, and integrates these works into a dedicated PC/XT emulator for ESP32.

## License

This project is released under the **GNU General Public License v3.0 (GPL‑3.0 or later)**.

See the `LICENSE` file for details.

## Author

**Jesus Martinez‑Mateo**  
jesus.martinez.mateo@gmail.com

---

<p align="center">
  <i>Retro computing on modern microcontrollers</i>
</p>
