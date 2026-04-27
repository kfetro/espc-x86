#!/bin/sh

nasm ega_rom.asm
xxd -c 256 -i -n ega_rom ega_rom >ega_rom.h
