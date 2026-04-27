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

#include "NimBLEDevice.h"
#include "NimBLEScan.h"
#include "NimBLEClient.h"
#include "NimBLEAdvertisedDevice.h"

#include "drivers/joystick/gamepad.h"

/**
 * BLE HID Gamepad Driver (NimBLE-Arduino 2.4.0)
 * - Actualizado para usar NimBLEAdvertisedDeviceCallbacks
 */
class BTGamepadDriver : 
  public NimBLEClientCallbacks, 
  public NimBLEScanCallbacks
{
public:
  BTGamepadDriver(BTGamepad *gp);

  void begin();
  void startScan();

  // Callbacks de Cliente (Conexión)
  void onConnect(NimBLEClient *pClient) override;
  void onDisconnect(NimBLEClient *pClient, int reason) override;

  // Callback de Escaneo (El nombre del método sigue siendo onResult)
  void onResult(const NimBLEAdvertisedDevice *dev) override;

  // Callback de Notificaciones (Estático)
  static void notifyCallback(
      NimBLERemoteCharacteristic *pRemoteCharacteristic,
      uint8_t *pData,
      size_t length,
      bool isNotify);

private:
  void connectTo(const NimBLEAdvertisedDevice *dev);
  void parseHID(const uint8_t *data, size_t len);

private:
  BTGamepad *m_gp;
  NimBLEClient *m_client;
  NimBLERemoteCharacteristic *m_char;

  bool m_connected;

  static BTGamepadDriver* s_instance;
};
