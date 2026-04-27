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

// VideoMode

/** \ingroup Enumerations
 * @brief Specifies a video mode
 */
enum class VideoMode {
  None,     /**< Video mode has not been set. */
  VGA,      /**< VGA display. */
  CVBS,     /**< Composite display. */
  I2C,      /**< I2C display. */
  SPI,      /**< SPI display. */
};

/**
 * @brief This class helps to know which is the current video output (VGA or Composite)
 */
struct CurrentVideoMode {

  static VideoMode get()           { return s_videoMode; }
  static void set(VideoMode value) { s_videoMode = value; }

  private:
    static VideoMode s_videoMode;
};

// TODO in a hal_video.cpp must be initialized:
// VideoMode CurrentVideoMode::s_videoMode = VideoMode::None;
