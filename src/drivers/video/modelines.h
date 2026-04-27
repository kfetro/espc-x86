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

// --- Preset Resolution Modelines ---

// Modeline for 256x192@50Hz resolution - requires upscaler
#define VGA_256x192_50Hz "\"256x192@50\" 8.13 256 288 296 328 192 196 198 202 -HSync -VSync DoubleScan"

// Modeline for 256x384@60Hz resolution
#define VGA_256x384_60Hz "\"256x384@60\" 17.09 256 272 304 352 384 387 391 404 -HSync -VSync DoubleScan"

#define VGA_320x200_60HzD "\"320x200@60HzD\" 25.175 320 328 376 400 200 226 227 262 -HSync -VSync DoubleScan"

// Modeline for 320x200@70Hz resolution - the same of VGA_640x200_70Hz with horizontal halved
#define VGA_320x200_70Hz "\"320x200@70Hz\" 12.5875 320 328 376 400 200 206 207 224 -HSync -VSync DoubleScan"

// Modeline for 320x200@75Hz resolution
#define VGA_320x200_75Hz "\"320x200@75Hz\" 12.93 320 352 376 408 200 208 211 229 -HSync -VSync DoubleScan"

// Modeline for 320x200@75Hz retro resolution
#define VGA_320x200_75HzRetro "\"320x200@75Hz\" 12.93 320 352 376 408 200 208 211 229 -HSync -VSync DoubleScan MultiScanBlank"

// Modeline for 320x240@60Hz resolution
#define QVGA_320x240_60Hz "\"320x240@60Hz\" 12.6 320 328 376 400 240 245 246 262 -HSync -VSync DoubleScan"

// Modeline for 400x300@60Hz resolution
#define VGA_400x300_60Hz "\"400x300@60Hz\" 20 400 420 484 528 300 300 302 314 -HSync -VSync DoubleScan"

// Modeline for 480x300@75Hz resolution
#define VGA_480x300_75Hz "\"480x300@75Hz\" 31.22 480 504 584 624 300 319 322 333 -HSync -VSync DoubleScan"

// Modeline for 512x192@60Hz resolution
#define VGA_512x192_60Hz "\"512x192@60Hz\" 32.5 512 524 592 672 192 193 194 202 -HSync -VSync QuadScan"

// Modeline for 512x384@60Hz resolution
#define VGA_512x384_60Hz "\"512x384@60Hz\" 32.5 512 524 592 672 384 385 388 403 -HSync -VSync DoubleScan"

// Modeline for 512x448@60Hz resolution
#define VGA_512x448_60Hz "\"512x448@60Hz\" 21.21 512 542 598 672 448 469 472 527 -HSync -VSync"

// Modeline for 512x512@58Hz resolution
#define VGA_512x512_58Hz "\"512x512@58Hz\" 21.21 512 538 594 668 512 513 516 545 -HSync -VSync"

// Modeline for 640x200@60Hz doublescan resolution
#define VGA_640x200_60HzD "\"640x200@60HzD\" 25.175 640 656 752 800 200 226 227 262 -HSync -VSync doublescan"

// Modeline for 640x200@70Hz resolution - the same of VGA_640x400_70Hz with vertical halved
#define VGA_640x200_70Hz "\"640x200@70Hz\" 25.175 640 656 752 800 200 206 207 224 -HSync -VSync DoubleScan"

// Modeline for 640x200@70Hz retro resolution
#define VGA_640x200_70HzRetro "\"640x200@70Hz\" 25.175 640 663 759 808 200 208 211 226 -HSync -VSync DoubleScan MultiScanBlank"

// Modeline for 640x240@60Hz (DoubleScan) resolution
#define VGA_640x240_60Hz "\"640x240@60Hz\" 25.175 640 656 752 800 240 245 247 262 -HSync -VSync DoubleScan"

// Thanks to Paul Rickards (http://biosrhythm.com) - requires upscaler
#define NTSC_640x240_60hz "\"NTSC 640x240 (60Hz)\" 12.312 640 662 719 784 240 244 247 262 -hsync -vsync"

// Modeline for 640x350@70Hz resolution
#define VGA_640x350_70Hz "\"640x350@70Hz\" 25.175 640 656 752 800 350 387 389 449 -HSync -VSync"

// Modeline for 640x350@70HzAlt1 resolution
#define VGA_640x350_70HzAlt1 "\"640x350@70HzAlt1\" 25.175 640 658 754 808 350 387 389 449 -HSync -VSync"

// Modeline for 640x350@85Hz resolution
#define VESA_640x350_85Hz "\"640x350@85Hz\" 31.5 640 672 736 832 350 382 385 445 -HSync -VSync"

// Modeline for 640x382@60Hz resolution
#define VGA_640x382_60Hz "\"640x382@60Hz\" 40 640 672 740 840 382 385 387 397 -HSync +VSync DoubleScan"

// Modeline for 640x384@60Hz resolution
#define VGA_640x384_60Hz "\"640x384@60Hz\" 40 640 672 740 840 384 384 386 397 -HSync +VSync DoubleScan"

// Modeline for 640x400@70Hz resolution
#define VGA_640x400_70Hz "\"640x400@70Hz\" 25.175 640 656 752 800 400 412 414 449 -HSync -VSync"

// Modeline for 640x400@60Hz (actually 640x480 but with less lines)
#define VGA_640x400_60Hz "\"640x400@60Hz-mod\" 25.175 640 656 752 800 400 452 454 525 -HSync -VSync"

// Modeline for 640x480@60Hz resolution
#define VGA_640x480_60Hz "\"640x480@60Hz\" 25.175 640 656 752 800 480 490 492 525 -HSync -VSync"

// Modeline for 640x480@60HzAlt1 resolution
#define VGA_640x480_60HzAlt1 "\"640x480@60HzAlt1\" 27.5 640 672 768 864 480 482 488 530 -HSync -VSync"

// Modeline for 640x480@60Hz doublescan resolution
#define VGA_640x480_60HzD "\"640x480@60HzD\" 54.00 640 688 744 900 480 480 482 500 +HSync +VSync DoubleScan"

// Modeline for 640x480@73Hz resolution
#define VGA_640x480_73Hz "\"640x480@73Hz\" 31.5 640 664 704 832 480 489 491 520 -HSync -VSync"

// Modeline for 640x480@75Hz resolution
#define VESA_640x480_75Hz "\"640x480@75Hz\" 31.5 640 656 720 840 480 481 484 500 -HSync -VSync"

// Thanks to Paul Rickards (http://biosrhythm.com) - requires upscaler
#define NTSC_720x240_60hz "\"NTSC 720x240 (60Hz)\" 13.820 720 744 809 880 240 244 247 262 -hsync -vsync"

// Thanks to Paul Rickards (http://biosrhythm.com) - requires upscaler
#define PAL_720x288_50hz "\"PAL 720x288 (50Hz)\" 13.853 720 741 806 888 288 290 293 312 -hsync -vsync"

// Modeline for 720x348@50Hz doublescan resolution
#define VGA_720x348_50HzD "\"720x348@50HzD\" 30.84 720 752 808 840 348 355 358 366 -hsync -vsync doublescan"

// Modeline for 720x348@59Hz doublescan resolution
#define VGA_720x348_59HzD "\"720x348@59d\" 38.87 720 788 852 896 348 350 359 366 -HSync -VSync DoubleScan"

// Modeline for 720x348@73Hz resolution
#define VGA_720x348_73Hz "\"720x348@73Hz\" 27 720 736 799 872 348 379 381 433 -HSync -VSync"

// Modeline for 720x350@70Hz resolution - thanks Stan Pechal
#define VGA_720x350_70Hz "\"720x350@70Hz\" 28.32 720 738 846 900 350 387 389 449 -HSync -VSync"

// Modeline for 720x400@70Hz resolution
#define VGA_720x400_70Hz "\"720x400@70Hz\" 28.32  720 738 846 900  400 412 414 449 -hsync +vsync"

// Modeline for 720x400@85Hz resolution
#define VESA_720x400_85Hz "\"720x400@85Hz\" 35.5 720 756 828 936 400 401 404 446 -HSync -VSync"

// Modeline for 720x576@50Hz resolution
#define PAL_720x576_50Hz "\"720x576@50Hz\" 27 720 732 795 864 576 581 586 625 -HSync -VSync"

// Thanks to Paul Rickards (http://biosrhythm.com) - requires upscaler
#define PAL_768x288_50hz "\"PAL 768x288 (50Hz)\" 14.726 768 790 859 944 288 290 293 312 -hsync -vsync"

// Modeline for 768x576@60Hz resolution
#define VESA_768x576_60Hz "\"768x576@60Hz\" 34.96 768 792 872 976 576 577 580 597 -HSync -VSync"

// Modeline for 800x300@60Hz resolution
#define SVGA_800x300_60Hz "\"800x300@60Hz\" 40 800 840 968 1056 300 301 303 314 -HSync -VSync DoubleScan"

// Modeline for 800x600@56Hz resolution
#define SVGA_800x600_56Hz "\"800x600@56Hz\" 36 800 824 896 1024 600 601 603 625 -HSync -VSync"

// Modeline for 800x600@60Hz resolution
#define SVGA_800x600_60Hz "\"800x600@60Hz\" 40 800 840 968 1056 600 601 605 628 -HSync -VSync"

// Modeline for 960x540@60Hz resolution
#define SVGA_960x540_60Hz "\"960x540@60Hz\" 37.26 960 976 1008 1104 540 542 548 563 +hsync +vsync"

// Modeline for 1024x768@60Hz resolution
#define SVGA_1024x768_60Hz "\"1024x768@60Hz\" 65 1024 1048 1184 1344 768 771 777 806 -HSync -VSync"

// Modeline for 1024x768@70Hz resolution
#define SVGA_1024x768_70Hz "\"1024x768@70Hz\" 75 1024 1048 1184 1328 768 771 777 806 -HSync -VSync"

// Modeline for 1024x768@75Hz resolution
#define SVGA_1024x768_75Hz "\"1024x768@75Hz\" 78.80 1024 1040 1136 1312 768 769 772 800 +HSync +VSync"

// Modeline for 1280x600@60Hz resolution
#define SVGA_1280x600_60Hz "\"1280x600@60Hz\" 61.5 1280 1336 1464 1648 600 601 604 622 -HSync -VSync"

// Modeline for 1280x720@60Hz resolution
#define SVGA_1280x720_60Hz "\"1280x720@60Hz\" 74.48 1280 1468 1604 1664 720 721 724 746 +hsync +vsync"

// Modeline for 1280x720@60Hz resolution
#define SVGA_1280x720_60HzAlt1 "\"1280x720@60HzAlt1\" 73.78 1280 1312 1592 1624 720 735 742 757"

// Modeline for 1280x768@50Hz resolution
#define SVGA_1280x768_50Hz "\"1280x768@50Hz\" 64.050004 1280 1312 1552 1584 768 784 791 807 -HSync -VSync"


// Modeline for OLED 128x64
#define OLED_128x64 "\"OLED_128x64\" 128 64"

// Modeline for OLED 128x32
#define OLED_128x32 "\"OLED_128x32\" 128 32"

// Modeline for TFT 240x135
#define TFT_135x240 "\"TFT_135x240\" 135 240"

// Modeline for TFT 240x240
#define TFT_240x240 "\"TFT_240x240\" 240 240"

// Modeline for TFT 240x320
#define TFT_240x320 "\"TFT_240x320\" 240 320"
