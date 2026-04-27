#!/bin/sh

jwasm -Fl -Zm -DCPU_TYPE="CPU_8088" -DARCH_TYPE="ARCH_TURBO" -bin GLABIOS.ASM

xxd -c 256 -i -n bios_rom GLABIOS.BIN >bios_rom.h
