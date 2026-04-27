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

#include "video/video_system.h"
#include "video/dummy.h" // DummyVideoCard

#include <stdio.h>

namespace video {

VideoSystem::VideoSystem() :
  m_ram(nullptr),
  m_type(VideoAdapterType::CGA),
  m_adapter(nullptr),
  m_context(nullptr),
  m_prevContext(nullptr),
  m_prevMode(0),
  m_dummy(nullptr)
{
}

VideoSystem::~VideoSystem()
{
  // Ensure dummy is cleaned up first (if still active)
  if (m_dummy) {
    resume();
  }
}

void VideoSystem::init(uint8_t *ram)
{
  m_ram = ram;
  m_scanout.init();
}

void VideoSystem::active(VideoAdapterType type, VideoAdapter *adapter, ScanoutContext *context)
{
  // Keep the model simple and safe: do not change adapter while suspended
  if (m_dummy) {
    printf("video: Cannot change adapter while suspended!\n");
    return;
  }

  m_type    = type;
  m_adapter = adapter;
  m_context = context;

  // Initialize the adapter and bind scanout source to adapter context
  if (m_adapter) {
    // Adapter init may also set an initial mode internally via VideoScanout
    m_adapter->init(m_ram, &m_scanout);
  }
  if (m_context) {
    m_scanout.setSource(m_context);
  }
}

void VideoSystem::setScanout(ScanoutContext *context, uint8_t mode)
{
  m_scanout.stop();
  m_scanout.setSource(context);
  m_scanout.setMode(mode);
  m_scanout.run();
}

ScanoutContext *VideoSystem::suspend(int mode)
{
  // Non-nestable by design
  if (m_dummy) {
    printf("video: Unable to suspend system!\n");
    return (ScanoutContext *) m_dummy;
  }

  if (!m_adapter || !m_context) {
    printf("video: No active adapter!\n");
    return nullptr;
  }

  // Save current scanout state so we can resume exactly where we left off
  m_prevContext = m_context;
  m_prevMode    = m_scanout.getMode();   // requires getter in VideoScanout

  // Create temporary dummy video adapter,
  // the real adapter remains allocated and untouched.
  m_dummy = new DummyVideoCard();

  // Dummy init allocates its VRAM and may call setSource/setMode itself,
  // but we will enforce our desired scanout state right after.
  m_dummy->init(m_ram, &m_scanout);

  // Switch scanout to dummy context and force menu mode.
  setScanout((ScanoutContext *) m_dummy, (uint8_t) mode);

  return (ScanoutContext *) m_dummy;
}

bool VideoSystem::resume()
{
  if (!m_dummy) {
    printf("video: Unable to resume system!\n");
    return false;
  }

  // Restore previous adapter context + mode first,
  // so scanout stops reading dummy VRAM.
  setScanout(m_prevContext, m_prevMode);

  // Now it's safe to destroy dummy and free its memory
  delete m_dummy;
  m_dummy = nullptr;

  m_prevContext = nullptr;
  m_prevMode    = 0;

  return true;
}

} // namespace fabgl
