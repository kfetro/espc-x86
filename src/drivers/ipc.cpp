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

#include "drivers/ipc.h"
#include "fabglconf.h"

#include "esp_ipc.h"

namespace fabgl {

int CoreUsage::s_busiestCore = FABGLIB_VIDEO_CPUINTENSIVE_TASKS_CORE;

struct esp_intr_alloc_args {
  int             source;
  int             flags;
  intr_handler_t  handler;
  void *          arg;
  intr_handle_t * ret_handle;
  TaskHandle_t    waitingTask;
};

void esp_intr_alloc_pinnedToCore_callback(void * arg)
{
  auto args = (esp_intr_alloc_args*) arg;

  esp_intr_alloc(args->source, args->flags, args->handler, args->arg, args->ret_handle);
}

void esp_intr_alloc_pinnedToCore(int source, int flags, intr_handler_t handler, void * arg, intr_handle_t * ret_handle, int core)
{
  esp_intr_alloc_args args = {
    source,
	flags,
	handler,
	arg,
	ret_handle,
	xTaskGetCurrentTaskHandle()
  };

  esp_ipc_call_blocking(core, esp_intr_alloc_pinnedToCore_callback, &args);
}

} // end of namespace
