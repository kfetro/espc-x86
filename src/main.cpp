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

#include "computer.h"
#include "setup.h"

#include "host/sdcard.h"
#include "host/settings.h"

#include "esp_idf_version.h"
#include "esp_chip_info.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
#include "esp_efuse.h"
#endif
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_task_wdt.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "esp_psram.h"
#else
extern "C" {
#include "esp32/spiram.h"
}
#endif

#include <Arduino.h>

// UART pins for USB serial
#define UART_URX 3
#define UART_UTX 1

static Computer *computer = nullptr;
static Settings *settings = nullptr;

// noinit! Used to maintain datetime between reboots
__NOINIT_ATTR static timeval savedTimeValue;

void print_esp_chip_info()
{
  esp_chip_info_t chip_info;

  esp_chip_info(&chip_info);

  printf("esp32: --- Chip Info ---\n");
  printf("esp32: Chip model = %d (%s)\n", chip_info.model, (chip_info.model == CHIP_ESP32) ? "ESP32" : "Unknown");
  printf("esp32: Cores = %d\n", chip_info.cores);
  printf("esp32: Revision number = %d\n", chip_info.revision);

  printf("esp32: WiFi (2.4 GHz) = %s\n", (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "yes" : "no");
  printf("esp32: Bluetooth (classic) = %s\n", (chip_info.features & CHIP_FEATURE_BT) ? "yes" : "no");
  printf("esp32: Bluetooth (low energy) = %s\n", (chip_info.features & CHIP_FEATURE_BLE) ? "yes" : "no");
  printf("esp32: Embedded Flash = %s\n", (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "yes" : "no");
  printf("esp32: Embedded PSRAM = %s\n", (chip_info.features & CHIP_FEATURE_EMB_PSRAM) ? "yes" : "no");

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
  const uint32_t package = esp_efuse_get_pkg_ver();
#else
  const uint32_t package = chip_info.package;
#endif
  // 0=D0WDQ6 (Wroom-32), 1=D0WDQ5 (Wrover), 2=D2WD, 3=U4WDH, 4=S0WD, 5=PICO-D4 (TTGO VGA32)
  printf("esp32: Package = %d (%s)\n", package, (package == 5) ? "TTGO VGA32" : "Unknown");
}

void print_esp_mem_info()
{
  printf("esp32: --- Mem Info ---\n");

  const size_t spiram_size = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  if (spiram_size) {
    printf("esp32: PSRAM size = %lu KB\n", spiram_size / 1024);
  } else {
    printf("esp32: No PSRAM detected\n");
  }

  const size_t total_dram = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
  const size_t free_dram  = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

  printf("esp32: DRAM size = %lu KB (%lu KB free)\n", total_dram / 1024, free_dram / 1024);

#if 0 // Only for debug
  const uint32_t free_heap     = esp_get_free_heap_size();
  const uint32_t free_int_heap = esp_get_free_internal_heap_size();
  printf("esp32: Free Heap size = %lu KB\n", free_heap / 1024);
  printf("esp32: Free internal Heap size = %lu KB\n", free_int_heap / 1024);

  const size_t min_free_heap = esp_get_minimum_free_heap_size();
  printf("esp32: Minimum free Heap size = %lu KB\n", min_free_heap / 1024);
#endif
}

void esp_disable_wdt()
{
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)

  esp_task_wdt_config_t task_wdt_config = {
    .timeout_ms = 10000,
    .idle_core_mask = 0,
    .trigger_panic = false
  };

  printf("Disabling task watchdog timer...\n");
  esp_err_t ret = esp_task_wdt_reconfigure(&task_wdt_config);
  if (ret != ESP_OK) {
    printf("Warning! Unable to reconfigure task wdt (%d)\n", ret);
  }

#else

  disableCore0WDT();
  //delay(100); // experienced crashes without this delay!
  disableCore1WDT();

#endif
}

void esp_setup_memory()
{
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)

  if (!esp_psram_is_initialized()) {
    printf("ERROR! PSRAM not initialized\n");
  }
/*
    esp_err_t r;
    r = psram_enable(PSRAM_SPEED, PSRAM_MODE);
    if (r != ESP_OK) {
        printf("SPI RAM enabled but initialization failed. Bailing out.\n");
    } else {
        printf("SPI RAM mode: %s\n", PSRAM_SPEED == PSRAM_CACHE_F40M_S40M ? "flash 40m sram 40m" : \
                                   PSRAM_SPEED == PSRAM_CACHE_F80M_S40M ? "flash 80m sram 40m" : \
                                   PSRAM_SPEED == PSRAM_CACHE_F80M_S80M ? "flash 80m sram 80m" : "ERROR");
        printf("PSRAM initialized, cache is in %s mode.\n", \
                                  (PSRAM_MODE==PSRAM_VADDR_MODE_EVENODD)?"even/odd (2-core)": \
                                  (PSRAM_MODE==PSRAM_VADDR_MODE_LOWHIGH)?"low/high (2-core)": \
                                  (PSRAM_MODE==PSRAM_VADDR_MODE_NORMAL)?"normal (1-core)":"ERROR");
    }

    // esp_spiram_init_cache()
    //Enable external RAM in MMU
    cache_sram_mmu_set( 0, 0, SOC_EXTRAM_DATA_LOW, 0, 32, 128 );
    //Flush and enable icache for APP CPU
#if !CONFIG_FREERTOS_UNICORE
    DPORT_CLEAR_PERI_REG_MASK(DPORT_APP_CACHE_CTRL1_REG, DPORT_APP_CACHE_MASK_DRAM1);
    cache_sram_mmu_set( 1, 0, SOC_EXTRAM_DATA_LOW, 0, 32, 128 );
#endif
*/
#else

  // Note: we use just 2MB of PSRAM so the infamous PSRAM bug should not happen.
  // But to avoid gcc compiler hack (-mfix-esp32-psram-cache-issue) we enable PSRAM at runtime,
  // otherwise the hack slows down CPU too much (PSRAM_HACK is no more required).
  if (esp_spiram_init() != ESP_OK) {
    printf("ERROR! A board with PSRAM is required\n");
  }

  #ifndef BOARD_HAS_PSRAM
  esp_spiram_init_cache();
  #endif

#endif // ESP_IDF_VERSION
}

// handle soft restart
void shutdownHandler()
{
  // save current datetime into Preferences
  gettimeofday(&savedTimeValue, NULL);
}

void updateDateTime()
{
  // Set timezone
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  // get datetime from savedTimeValue? (noinit section)
  if (esp_reset_reason() == ESP_RST_SW) {
    // adjust time taking account elapsed time since ESP32 started
    savedTimeValue.tv_usec += (int) esp_timer_get_time();
    savedTimeValue.tv_sec  += savedTimeValue.tv_usec / 1000000;
    savedTimeValue.tv_usec %= 1000000;
    settimeofday(&savedTimeValue, nullptr);
    return;
  }

  // set default time
  auto tm = (struct tm){
    .tm_sec  = 0,
    .tm_min  = 0,
    .tm_hour = 8,
    .tm_mday = 14,
    .tm_mon  = 7,
    .tm_year = 84,
    .tm_wday  = 0,
    .tm_yday  = 0, 
    .tm_isdst = -1
  };
  auto now = (timeval){ .tv_sec = mktime(&tm), .tv_usec = 0 };
  settimeofday(&now, nullptr);
}

// Sysreq pressed (Alt + Print Screen)
// Alternatively it works also for Ctrl + Alt + Back
void sysreq_callback(uint8_t reqId)
{
  switch(reqId) {

    // System configuration and setup
    case 1:
      settings->show();
      break;

    // Mount floppy
    case 2:
      settings->mountFloppy();
      break;

    // Mount hard disk
    case 3:
      settings->mountHardDisk();
      break;

    // Pause / Resume
    case 4:
      if (!computer->paused()) {
        computer->pause();
      } else {
        computer->resume();
      }
      break;

    // Mute speaker
    case 5:
      computer->audio_toggleMute();
      break;

    // Volume down
    case 6:
      computer->audio_volumeDown();
      break;

    // Volume up
    case 7:
      computer->audio_volumeUp();
      break;

    // Snapshot
    case 8:
      computer->video_snapshot();
      break;

    // Hard reset
    case 11:
      esp_restart();
      break;

    // Soft reboot
    case 12:
      computer->reboot();
      break;

    default:
      break;
  }
}

void setup_computer()
{
  printf("esp32: ESP-IDF version = %s\n", esp_get_idf_version());
  print_esp_chip_info();
  print_esp_mem_info();

  // Save some space reducing UI queue
  fabgl::BitmappedDisplayController::queueSize = 128;

  esp_disable_wdt();
  esp_setup_memory();

  esp_register_shutdown_handler(shutdownHandler);
  updateDateTime();

  sdcard_mount(SD_MOUNT_PATH);

  computer = new Computer;

#if 0 // Commented to keep serial port for debugging.
    auto serial1 = new SerialPort;
    serial1->setSignals(UART_URX, UART_UTX);
    computer->setCOM1(serial1);
#endif

  settings = new Settings(computer);

  computer->setSysReqCallback(sysreq_callback);
  computer->run();
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  printf("\nReset\n");
  setup_computer();
}

#if FABGLIB_VGAXCONTROLLER_PERFORMANCE_CHECK
namespace fabgl {
  extern volatile uint64_t s_vgapalctrlcycles;
}
using fabgl::s_vgapalctrlcycles;
#endif

void loop()
{
#if FABGLIB_VGAXCONTROLLER_PERFORMANCE_CHECK
  static uint32_t tcpu = 0, s1 = 0, count = 0;
  tcpu = computer->ticksCounter();
  s_vgapalctrlcycles = 0;
  s1 = fabgl::getCycleCount();
  delay(1000);
  printf("%d\tCPU: %d", count, computer->ticksCounter() - tcpu);
  printf("   Graph: %lld / %d   (%d%%)\n", s_vgapalctrlcycles, fabgl::getCycleCount() - s1, (int)((double)s_vgapalctrlcycles/240000000*100));
  ++count;
#else
  //vTaskDelete(NULL);
  vTaskDelay(1);
#endif
}
