<p align="center">
  https://aristoteles.dma.fi.upm.es/espc-86/ESPC.png
</p>

<h1 align="center">ESPC-86</h1>

<p align="center">
  <b>PC/XT 8086 ESP32-Emulation</b><br>
  An IBM PC/XT 8086 emulator for the ESP32
</p>

<p align="center">
  #what-is-espc-86What is ESPC‑86?</a> •
  #hardware •
  #featuresFeatures</a> •
  #installationInstallation</a> •
  #controls> •
  #licenseLicense</a>
</p>

---

## What is ESPC‑86?

**ESPC‑86** (PC/XT 8086 ESP32‑Emulation) is an emulator of a classic **IBM PC/XT (8086)** computer running on an **ESP32** microcontroller.

The project is specially designed for the **TTGo VGA32** board, providing:

- VGA video output (monitor or TV)
- PS/2 keyboard input
- PS/2 mouse input

With ESPC‑86, the ESP32 becomes a fully functional retro PC capable of running classic **DOS software**, recreating the experience of an early IBM‑compatible computer using inexpensive modern hardware.

---

## Hardware

ESPC‑86 is primarily developed and tested on the following board:

### ✅ Supported board

- **TTGo VGA32**
  - ESP32 microcontroller
  - VGA output
  - 2 × PS/2 ports (keyboard and mouse)
  - Optional SD card support (depending on configuration)

### 🔌 Peripherals

- VGA monitor or VGA‑to‑TV adapter
- PS/2 keyboard
- PS/2 mouse

---

## Features

- ✅ Emulation of an **IBM PC/XT (8086 CPU)**
- ✅ VGA video output
- ✅ PS/2 keyboard support
- ✅ PS/2 mouse support
- ✅ Designed for low‑cost ESP32 hardware
- ✅ Based on proven open‑source emulation cores
- ✅ Easy firmware installation using a web flasher

---

## Installation

The easiest way to install ESPC‑86 is using the **online firmware flasher**.

### 🌐 Online flasher (recommended)

1. Connect your **TTGo VGA32** board to your computer via USB
2. Open the following link in a Chromium‑based browser (Chrome, Edge, Brave, etc.):

   👉 **https://aristoteles.dma.fi.upm.es/espc-86/**

3. Follow the instructions on the webpage to flash the firmware

> ⚠️ Make sure no other application is using the serial port before flashing.

---

## Controls

### Keyboard

- Standard **PS/2 keyboard**
- Mapped to PC/XT keyboard scancodes

### Mouse

- Standard **PS/2 mouse**
- Used by supported software and operating systems

---

## Project Status

ESPC‑86 is under active development.

The project is:
- Usable
- Extensible
- Intended for experimentation, learning, and retro‑computing fun

Feedback, testing, and contributions are welcome.

---

## Origins and Credits

This project is a **fork and continuation of FabGL**, and includes code derived from **8086tiny**.

- **FabGL**  
  Copyright © Fabrizio Di Vittorio  
  https://github.com/fdivitto/FabGL

- **8086tiny**  
  Copyright © Adrian Cable  
  Licensed under the MIT License

ESPC‑86 extends, modifies, and integrates these works into a dedicated PC/XT emulator for ESP32.

---

## License

This project is released under the **GNU General Public License v3.0 (GPL‑3.0 or later)**.

See the `LICENSE` file for details.

---

## Author

**Jesus Martinez‑Mateo**  
📧 jesus.martinez.mateo@gmail.com

---

<p align="center">
  <i>Retro computing on modern microcontrollers</i>
</p>
