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

#include "host/sdcard.h"

// SD card
#include "sdmmc_cmd.h"
#include "driver/gpio.h"
#if USE_SDMMC
#include "driver/sdmmc_host.h"
#else
#include "driver/sdspi_host.h"
#endif

// FAT file system with VFS
#include "esp_err.h"
#include "esp_vfs_fat.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ESP32 TTGO-VGA32
#define SDCARD_PIN_MISO 2
#define SDCARD_PIN_MOSI 12
#define SDCARD_PIN_CLK  14
#define SDCARD_PIN_CS   13

// Typical allocation unit size is 16 KB for better write performance
#define ALLOCATION_UNIT_SIZE 16384
// Reduced here to 4 KB to save RAM
//#define ALLOCATION_UNIT_SIZE 4096

static char *g_mount_point = NULL;
static sdmmc_card_t *g_card = NULL;

int sdcard_mount(const char *mount_point)
{
  esp_err_t ret;
  sdmmc_card_t *card;

  if (g_card) {
    printf("sdc: SD already mounted!\n");
    return SDCARD_ERROR;
  }

  // Duplicate the string to ensure the mount point persists
  // even if the original buffer is freed or modified
  // g_mount_point = mount_point;
  g_mount_point = strdup(mount_point);
  if (!g_mount_point) {
    printf("sdc: ERROR! Out of memory\n");
    return SDCARD_ERROR;
  }

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {};
  mount_config.format_if_mount_failed = false;
  mount_config.max_files = 5;
  mount_config.allocation_unit_size = ALLOCATION_UNIT_SIZE;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)

/* After upgrading from ESP-IDF 4.x to 5.x, SD card SPI performance
   became extremely slow, unless external pull-ups are added.
   Only the MISO pin (GPIO12) requires pull-up for performance improvement.
   Internal pull-ups via gpio_pullup_en() are ineffective,
   but gpio_config() with explicit pull-up configuration works,
   though with a critical discrepancy in GPIO dump reporting.
   https://github.com/espressif/esp-idf/issues/16909
 */
  gpio_config_t miso_config = {
    .pin_bit_mask = (1ULL << SDCARD_PIN_MISO),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&miso_config);

#endif

#if USE_SDMMC

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.slot = SDMMC_HOST_SLOT_1;
  //host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
  //host.max_freq_khz = SDMMC_FREQ_DDR50;
  //host.flags &= ~SDMMC_HOST_FLAG_DDR;
  host.flags = SDMMC_HOST_FLAG_1BIT;
  //host.max_freq_khz = SDMMC_FREQ_DEFAULT;
  host.max_freq_khz = SDMMC_FREQ_PROBING;

  // Note: SDMMC does not use MOSI/MISO/CLK/CS like SPI
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = 1;
  slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  printf("sdc: Mounting at path %s\n", mount_point);
  ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
  if (ret != ESP_OK) {
    printf("sdc: ERROR! Unable to mount (%s)\n", esp_err_to_name(ret));
    free(g_mount_point);
    g_mount_point = NULL;
    return SDCARD_ERROR;
  }

#else // USE_SDMMC

  // SPI Host setup
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SPI2_HOST;
  host.max_freq_khz = 20000; // Try 40MHz instead of default

  // Configure SPI bus
  spi_bus_config_t bus_config = {};
  bus_config.mosi_io_num = (gpio_num_t) SDCARD_PIN_MOSI;
  bus_config.miso_io_num = (gpio_num_t) SDCARD_PIN_MISO;
  bus_config.sclk_io_num = (gpio_num_t) SDCARD_PIN_CLK;
  bus_config.quadwp_io_num = -1;
  bus_config.quadhd_io_num = -1;
  bus_config.max_transfer_sz = 32 * 1024; // 4096;
  bus_config.flags = SPICOMMON_BUSFLAG_MASTER;

  ret = spi_bus_initialize(SPI2_HOST, &bus_config, SDSPI_DEFAULT_DMA);
  if (ret != ESP_OK) {
    printf("sdc: Unable to initialize SPI bus\n");
    free(g_mount_point);
    g_mount_point = NULL;
    return SDCARD_ERROR;
  }

#if 0
  // SPI device configuration
  // Note: esp_vfs_fat_sdspi_mount() already handles the SD-SPI device
  // configuration (it internally adds the device and manages the handle)
  spi_device_handle_t m_SPIDevHandle;
  spi_device_interface_config_t dev_config = { }; // Zero init
  dev_config.mode           = 0;
  dev_config.clock_speed_hz = 10000000; //23000000;
  dev_config.spics_io_num   = SDCARD_PIN_CS;
  dev_config.flags          = 0;
  dev_config.queue_size     = 1;

  ret = spi_bus_add_device(SPI2_HOST, &dev_config, &m_SPIDevHandle);
  if (ret != ESP_OK) {
    printf("sdc: WARNING! Unable to add device (%s)\n", esp_err_to_name(ret));
  }
#endif

  // Slot config
  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = (gpio_num_t) SDCARD_PIN_CS;
  slot_config.host_id = SPI2_HOST;
  //slot_config.max_freq_khz = 20000; // Set to 20MHz or 40MHz

  printf("sdc: Mounting at path %s\n", mount_point);
  ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
  if (ret != ESP_OK) {
    printf("sdc: ERROR! Unable to mount (%s)\n", esp_err_to_name(ret));
    spi_bus_free(SPI2_HOST);
    free(g_mount_point);
    g_mount_point = NULL;
    return SDCARD_ERROR;
  }

#endif // USE_SDMMC

  printf("sdc: --- SD Info ---\n");
#if USE_SDMMC
  sdmmc_card_print_info(stdout, card);
#else
  printf("sdc: Name = %s\n", card->cid.name);
  printf("sdc: Type = %s\n", (card->ocr & (1 << 30)) ? "SDHC/SDXC" : "SDSC");
  printf("sdc: Speed = %s\n", (card->csd.tr_speed > 25000000) ? "High Speed" : "Default Speed");
  printf("sdc: Size = %llu MB\n", ((uint64_t) card->csd.capacity) * card->csd.sector_size / (1024 * 1024));
  //printf("sdc: Version = %d\n", card->csd.csd_ver);
  //printf("sdc: Sector size = %d bytes\n", card->csd.sector_size);
  //printf("sdc: Capacity = %d sectors\n", card->csd.capacity);
#endif

  g_card = card;

  return SDCARD_OK;
}

int sdcard_umount()
{
  if (!g_card) {
    printf("sdc: Not mounted!\n");
    return SDCARD_ERROR;
  }

  // Unmount SD card
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  esp_vfs_fat_sdcard_unmount(g_mount_point, g_card);
#else
  esp_vfs_fat_sdmmc_unmount();
#endif

#if !USE_SDMMC
  spi_bus_free(SPI2_HOST);
#endif

  free(g_mount_point);
  g_mount_point = NULL;
  g_card = NULL;

  printf("sdc: unmounted\n");
  return SDCARD_OK;
}
