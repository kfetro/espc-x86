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

#include <esp_wifi.h>

// --- gpio_stream ---

// Defines ESP32 XTAL frequency in Hz
#define FABGLIB_XTAL 40000000

// Optional feature. Use b/a coeff to fine tune frequency.
// Unfortunately output is not very stable when enabled!
#define FABGLIB_USE_APLL_AB_COEF 0

// --- Serial Port ---

#define FABGLIB_TERMINAL_FLOWCONTROL_RXFIFO_MIN_THRESHOLD 20
#define FABGLIB_TERMINAL_FLOWCONTROL_RXFIFO_MAX_THRESHOLD 300

// --- Keyboard ---

// Stack size of the task that converts scancodes to Virtual Keys Keyboard
#define FABGLIB_DEFAULT_SCODETOVK_TASK_STACK_SIZE 1500

// Priority of the task that converts scancodes to Virtual Keys Keyboard
#define FABGLIB_SCODETOVK_TASK_PRIORITY 5

// Size of virtualkey queue
#define FABGLIB_KEYBOARD_VIRTUALKEY_QUEUE_SIZE 32

// Optional feature. Enables Keyboard.virtualKeyToString() method
#define FABGLIB_HAS_VirtualKeyO_STRING 1

// --- Mouse ---

// Size of mouse events queue
#define FABGLIB_MOUSE_EVENTS_QUEUE_SIZE 64

// --- Display ---
// Size of display controller primitives queue
#define FABGLIB_DEFAULT_DISPLAYCONTROLLER_QUEUE_SIZE 1024

// Size (in bytes) of primitives dynamic buffers.
// Used by primitives like drawPath and fillPath to contain path points.
#define FABGLIB_PRIMITIVES_DYNBUFFERS_SIZE 512

// Defines the underline position starting from character bottom (0 = bottom of the character)
#define FABGLIB_UNDERLINE_POSITION 0

// --- VGA Base ---
// To reduce memory overhead the viewport is allocated as few big buffers.
// This parameter defines the maximum number of these big buffers.
#define FABGLIB_VIEWPORT_MEMORY_POOL_COUNT 128

// Size (in bytes) of largest block to maintain free
#define FABGLIB_MINFREELARGESTBLOCK 40000

// --- VGA Palette ---

// vga_palette, vga_direct
// Core to use for CPU intensive tasks like VGA signals generation in VGATextController or VGAXXController
#define FABGLIB_VIDEO_CPUINTENSIVE_TASKS_CORE (WIFI_TASK_CORE_ID ^ 1) // using the same core of WiFi may cause flickering

// Stack size of primitives drawing task of paletted based VGA controllers
#define FABGLIB_VGAPALETTEDCONTROLLER_PRIMTASK_STACK_SIZE 1200

// Priority of primitives drawing task of paletted based VGA controllers
#define FABGLIB_VGAPALETTEDCONTROLLER_PRIMTASK_PRIORITY 5

// debug options
#define FABGLIB_VGAXCONTROLLER_PERFORMANCE_CHECK     0
#define FABGLIB_CVBSCONTROLLER_PERFORMANCE_CHECK     0
