# ESPC-x86 — Top-level Makefile
#
# Targets:
#   make              — build both ESP32 and native
#   make esp32        — build the ESP32 firmware
#   make native       — build the native (host) binary
#   make test         — build and run native unit tests
#   make run          — build and run the native binary
#   make flash        — build and flash ESP32 firmware
#   make monitor      — open ESP32 serial monitor
#   make clean        — clean all build artifacts
#   make clean-esp32  — clean ESP32 build only
#   make clean-native — clean native build only

PIO ?= pio

.PHONY: all esp32 native test run flash monitor clean clean-esp32 clean-native

all: esp32 native

esp32:
	$(PIO) run -e esp32dev

native:
	$(PIO) run -e native

test:
	$(PIO) test -e native

run: native
	$(PIO) run -e native -t exec

flash:
	$(PIO) run -e esp32dev -t upload

monitor:
	$(PIO) device monitor

clean: clean-esp32 clean-native

clean-esp32:
	$(PIO) run -e esp32dev -t clean 2>/dev/null || true

clean-native:
	$(PIO) run -e native -t clean 2>/dev/null || true
