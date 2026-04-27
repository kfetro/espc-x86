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

#include "drivers/joystick/bt_driver.h"

BTGamepadDriver* BTGamepadDriver::s_instance = nullptr;

BTGamepadDriver::BTGamepadDriver(BTGamepad *gp)
  : m_gp(gp),
    m_client(nullptr),
    m_char(nullptr),
    m_connected(false)
{
  s_instance = this;
}

void BTGamepadDriver::begin()
{
  NimBLEDevice::init("ESP32-XTJOY");
  startScan();
}

void BTGamepadDriver::startScan()
{
  NimBLEScan *scan = NimBLEDevice::getScan();

  scan->setScanCallbacks(this);

  scan->setActiveScan(true);
  scan->setInterval(45);
  scan->setWindow(30);

  scan->start(4, false);
}

void BTGamepadDriver::onResult(const NimBLEAdvertisedDevice* dev)
{
  // HID Service UUID 0x1812
  if (dev->isAdvertisingService(NimBLEUUID((uint16_t) 0x1812))) {
    NimBLEDevice::getScan()->stop();
    connectTo(dev);
  }
}

void BTGamepadDriver::connectTo(const NimBLEAdvertisedDevice *dev)
{
  m_client = NimBLEDevice::createClient();
  m_client->setClientCallbacks(this);

  if (!m_client->connect(dev)) {
    startScan();
    return;
  }

  NimBLERemoteService *hid = m_client->getService(NimBLEUUID((uint16_t) 0x1812));
  if (!hid) {
    m_client->disconnect();
    startScan();
    return;
  }

  m_char = hid->getCharacteristic(NimBLEUUID((uint16_t) 0x2A4D));
  if (m_char && m_char->canNotify()) {
    m_char->subscribe(true, notifyCallback);
  }
}

void BTGamepadDriver::onConnect(NimBLEClient *client)
{
  m_connected = true;
}

void BTGamepadDriver::onDisconnect(NimBLEClient *client, int reason)
{
  m_connected = false;
  startScan();
}

void BTGamepadDriver::notifyCallback(
    NimBLERemoteCharacteristic *chr,
    uint8_t *data,
    size_t len,
    bool isNotify)
{
  if (s_instance)
    s_instance->parseHID(data, len);
}

void BTGamepadDriver::parseHID(const uint8_t *d, size_t len)
{
  if (len < 3)
    return;

  float x = (d[1] / 127.5f) - 1.0f;
  float y = (d[2] / 127.5f) - 1.0f;

  m_gp->setAxis(x, y);

  m_gp->setButton(0, d[0] & 0x01);
  m_gp->setButton(1, d[0] & 0x02);
}
