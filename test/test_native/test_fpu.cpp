// test_fpu.cpp — Native unit tests for the 8087 FPU dispatch.
//
// These tests drive ESC opcodes through i8086::step() and verify that the
// 8086 decoder advances IP correctly and the i8087 emulator produces the
// expected memory / register side effects.
//
// Run with: pio test -e native --filter test_native

#include <unity.h>
#include <cstring>

#include "core/i8086.h"

using fabgl::i8086;

// 1 MB RAM
static uint8_t test_ram[1048576];

static void     test_writePort(void *, int, uint8_t) {}
static uint8_t  test_readPort(void *, int)   { return 0xFF; }
static void     test_writeVMem8(void *, int, uint8_t) {}
static void     test_writeVMem16(void *, int, uint16_t) {}
static uint8_t  test_readVMem8(void *, int)  { return 0xFF; }
static uint16_t test_readVMem16(void *, int) { return 0xFFFF; }
static bool     test_interrupt(void *, int)  { return false; }

static void fpu_init() {
    memset(test_ram, 0, sizeof(test_ram));
    i8086::setCallbacks(nullptr,
        test_readPort, test_writePort,
        test_writeVMem8, test_writeVMem16,
        test_readVMem8, test_readVMem16,
        test_interrupt);
    i8086::setMemory(test_ram);
    i8086::reset();
    // Place code at a known location instead of FFFF:0000
    i8086::setCS(0x0000);
    i8086::setIP(0x0100);
    i8086::setDS(0x0000);
    i8086::setSS(0x0000);
    i8086::setSP(0x1000);
}

static void put_code(const uint8_t *code, size_t len) {
    uint32_t addr = i8086::CS() * 16 + i8086::IP();
    memcpy(test_ram + addr, code, len);
}

// Helper: read 16-bit little-endian from RAM
static uint16_t mem16(uint32_t addr) {
    return test_ram[addr] | ((uint16_t)test_ram[addr + 1] << 8);
}

// ──────────────────────────────────────────────────────────────────────────
// IP advancement tests — these must pass regardless of whether FPU
// emulation is enabled. They verify the 8086 decoder agrees with Intel
// instruction-length rules for ESC opcodes.
// ──────────────────────────────────────────────────────────────────────────

void test_fpu_ip_fninit() {
    fpu_init();
    // FNINIT = DB E3 (mod=3, reg=4, rm=3) — 2 bytes
    uint8_t code[] = { 0xDB, 0xE3 };
    put_code(code, sizeof(code));
    uint16_t ip = i8086::IP();
    i8086::step();
    TEST_ASSERT_EQUAL_HEX16(ip + 2, i8086::IP());
}

void test_fpu_ip_fstsw_ax() {
    fpu_init();
    // FNSTSW AX = DF E0 (mod=3, reg=4, rm=0) — 2 bytes
    uint8_t code[] = { 0xDF, 0xE0 };
    put_code(code, sizeof(code));
    uint16_t ip = i8086::IP();
    i8086::step();
    TEST_ASSERT_EQUAL_HEX16(ip + 2, i8086::IP());
}

void test_fpu_ip_fnstcw_bx() {
    fpu_init();
    // FNSTCW WORD PTR [BX] = D9 3F (mod=0, reg=7, rm=7) — 2 bytes
    uint8_t code[] = { 0xD9, 0x3F };
    put_code(code, sizeof(code));
    uint16_t ip = i8086::IP();
    i8086::step();
    TEST_ASSERT_EQUAL_HEX16(ip + 2, i8086::IP());
}

void test_fpu_ip_fnstsw_bx() {
    fpu_init();
    // FNSTSW WORD PTR [BX] = DD 3F (mod=0, reg=7, rm=7) — 2 bytes
    uint8_t code[] = { 0xDD, 0x3F };
    put_code(code, sizeof(code));
    uint16_t ip = i8086::IP();
    i8086::step();
    TEST_ASSERT_EQUAL_HEX16(ip + 2, i8086::IP());
}

void test_fpu_ip_fldcw_disp16() {
    fpu_init();
    // FLDCW WORD PTR [0x1234] = D9 2E 34 12 (mod=0, reg=5, rm=6) — 4 bytes
    uint8_t code[] = { 0xD9, 0x2E, 0x34, 0x12 };
    put_code(code, sizeof(code));
    uint16_t ip = i8086::IP();
    i8086::step();
    TEST_ASSERT_EQUAL_HEX16(ip + 4, i8086::IP());
}

void test_fpu_ip_fadd_disp8() {
    fpu_init();
    // FADD DWORD PTR [BX+10h] = D8 47 10 (mod=1, reg=0, rm=7) — 3 bytes
    uint8_t code[] = { 0xD8, 0x47, 0x10 };
    put_code(code, sizeof(code));
    uint16_t ip = i8086::IP();
    i8086::step();
    TEST_ASSERT_EQUAL_HEX16(ip + 3, i8086::IP());
}

void test_fpu_ip_fnop() {
    fpu_init();
    // FNOP = D9 D0 — 2 bytes
    uint8_t code[] = { 0xD9, 0xD0 };
    put_code(code, sizeof(code));
    uint16_t ip = i8086::IP();
    i8086::step();
    TEST_ASSERT_EQUAL_HEX16(ip + 2, i8086::IP());
}

void test_fpu_ip_fwait() {
    fpu_init();
    // FWAIT = 9B — 1 byte
    uint8_t code[] = { 0x9B };
    put_code(code, sizeof(code));
    uint16_t ip = i8086::IP();
    i8086::step();
    TEST_ASSERT_EQUAL_HEX16(ip + 1, i8086::IP());
}

// ──────────────────────────────────────────────────────────────────────────
// Functional tests — verify the FPU produces the right side effects.
// These require case 69 in i8086.cpp to dispatch to the i8087.
// ──────────────────────────────────────────────────────────────────────────

void test_fpu_fninit_then_fnstcw_writes_037F() {
    // FNINIT must leave control word = 0x037F; FNSTCW writes that 16-bit
    // value to DS:[BX]. Bytes above must remain untouched (a 16-bit store,
    // not a 64-bit one).
    fpu_init();

    uint8_t code[] = {
        0xBB, 0x00, 0x20,          // MOV BX, 0x2000
        0xDB, 0xE3,                // FNINIT
        0xD9, 0x3F,                // FNSTCW [BX]
    };
    put_code(code, sizeof(code));

    i8086::step();   // MOV BX
    i8086::step();   // FNINIT
    i8086::step();   // FNSTCW [BX]

    TEST_ASSERT_EQUAL_HEX16(0x037F, mem16(0x2000));
    TEST_ASSERT_EQUAL_HEX16(0x0000, mem16(0x2002));
    TEST_ASSERT_EQUAL_HEX16(0x0000, mem16(0x2004));
    TEST_ASSERT_EQUAL_HEX16(0x0000, mem16(0x2006));
}

void test_fpu_fninit_then_fnstsw_writes_0000() {
    // FNSTSW [BX] (DD /7) must write a single 16-bit word, not 8 bytes.
    // We detect an over-wide write by sentinel-filling the surrounding
    // memory.
    fpu_init();

    uint8_t code[] = {
        0xBB, 0x00, 0x20,          // MOV BX, 0x2000
        0xDB, 0xE3,                // FNINIT
        0xDD, 0x3F,                // FNSTSW [BX]
    };
    put_code(code, sizeof(code));

    for (int i = 0; i < 16; i++)
        test_ram[0x2000 + i] = 0xCC;

    i8086::step();   // MOV BX
    i8086::step();   // FNINIT
    i8086::step();   // FNSTSW [BX]

    TEST_ASSERT_EQUAL_HEX16(0x0000, mem16(0x2000));

    // Bytes 2..7 above the target must remain at the sentinel value.
    TEST_ASSERT_EQUAL_HEX8(0xCC, test_ram[0x2002]);
    TEST_ASSERT_EQUAL_HEX8(0xCC, test_ram[0x2003]);
    TEST_ASSERT_EQUAL_HEX8(0xCC, test_ram[0x2004]);
    TEST_ASSERT_EQUAL_HEX8(0xCC, test_ram[0x2005]);
    TEST_ASSERT_EQUAL_HEX8(0xCC, test_ram[0x2006]);
    TEST_ASSERT_EQUAL_HEX8(0xCC, test_ram[0x2007]);
}

void test_fpu_fninit_then_fstsw_ax_returns_0() {
    // FNSTSW AX (DF E0) must copy the status word into AX.
    fpu_init();

    uint8_t code[] = {
        0xB8, 0xFF, 0xFF,          // MOV AX, 0xFFFF       ; sentinel
        0xDB, 0xE3,                // FNINIT
        0xDF, 0xE0,                // FNSTSW AX
    };
    put_code(code, sizeof(code));

    i8086::step();   // MOV AX
    i8086::step();   // FNINIT
    i8086::step();   // FNSTSW AX

    TEST_ASSERT_EQUAL_HEX16(0x0000, i8086::AX());
}

void test_fpu_fldcw_fnstcw_roundtrip() {
    // Set a non-default control word, then read it back.
    fpu_init();

    // Place 0x027F into RAM at DS:0x3000
    test_ram[0x3000] = 0x7F;
    test_ram[0x3001] = 0x02;

    uint8_t code[] = {
        0xBB, 0x00, 0x30,          // MOV BX, 0x3000
        0xD9, 0x2F,                // FLDCW  [BX]
        0xBB, 0x00, 0x40,          // MOV BX, 0x4000
        0xD9, 0x3F,                // FNSTCW [BX]
    };
    put_code(code, sizeof(code));

    for (int i = 0; i < 4; i++)
        i8086::step();

    TEST_ASSERT_EQUAL_HEX16(0x027F, mem16(0x4000));
}

void test_fpu_glabios_has_fpu_stack_safety() {
    // Reproduces GLaBIOS' HAS_FPU probe (GLABIOS.ASM line ~12909):
    //
    //   PUSH AX                  ; push sentinel
    //   MOV BX, SP               ; BX -> top of stack
    //   FNSTSW SS:[BX]           ; 16-bit store of status word
    //   POP AX                   ; AX = status word
    //
    // Stack grows down; PUSH AX lands at SS:0x0FFE, so the caller's
    // saved frame above sits at SS:0x1000+. A 64-bit FNSTSW would
    // clobber 6 bytes of that frame.
    fpu_init();

    test_ram[0x1000] = 0xAA;
    test_ram[0x1001] = 0xBB;
    test_ram[0x1002] = 0xCC;
    test_ram[0x1003] = 0xDD;
    test_ram[0x1004] = 0xEE;
    test_ram[0x1005] = 0xFF;

    uint8_t code[] = {
        0xB8, 0x34, 0x12,          // MOV AX, 0x1234
        0xDB, 0xE3,                // FNINIT
        0x50,                      // PUSH AX
        0x8B, 0xDC,                // MOV BX, SP
        0x36, 0xDD, 0x3F,          // FNSTSW SS:[BX]   (SS prefix + DD /7)
        0x58,                      // POP AX
    };
    put_code(code, sizeof(code));

    i8086::step();   // MOV AX, 0x1234
    i8086::step();   // FNINIT
    i8086::step();   // PUSH AX
    i8086::step();   // MOV BX, SP
    i8086::step();   // SS prefix (consumed as its own step in this core)
    i8086::step();   // FNSTSW SS:[BX]
    i8086::step();   // POP AX

    // After FNINIT, status word = 0x0000 → POP AX sees 0.
    TEST_ASSERT_EQUAL_HEX16(0x0000, i8086::AX());

    // Sentinel above the pushed word MUST survive intact. If FNSTSW
    // wrote 8 bytes (FSTP-m64 bug), these would all be 0x00.
    TEST_ASSERT_EQUAL_HEX8(0xAA, test_ram[0x1000]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, test_ram[0x1001]);
    TEST_ASSERT_EQUAL_HEX8(0xCC, test_ram[0x1002]);
    TEST_ASSERT_EQUAL_HEX8(0xDD, test_ram[0x1003]);
    TEST_ASSERT_EQUAL_HEX8(0xEE, test_ram[0x1004]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, test_ram[0x1005]);
}

void test_fpu_fnop_does_not_swallow_fld_sti() {
    // FNOP is specifically D9 D0 (reg=2, rm=0). FLD ST(i) is D9 C0+i
    // (reg=0, rm=i). After FLDZ + FLD ST(0), no exception bits should
    // be set in the status word: bit 0 = IE, bit 6 = SF.
    fpu_init();

    uint8_t code[] = {
        0xDB, 0xE3,                // FNINIT
        0xD9, 0xEE,                // FLDZ            ; ST(0) = +0.0
        0xD9, 0xC0,                // FLD ST(0)       ; duplicate ST(0)
        0xDF, 0xE0,                // FNSTSW AX
    };
    put_code(code, sizeof(code));

    for (int i = 0; i < 4; i++)
        i8086::step();

    uint16_t ax = i8086::AX();
    TEST_ASSERT_EQUAL_HEX16(0, ax & ((1u << 0) | (1u << 6)));
}

// ──────────────────────────────────────────────────────────────────────────
// Test runner
// ──────────────────────────────────────────────────────────────────────────

void run_fpu_tests() {
    RUN_TEST(test_fpu_ip_fninit);
    RUN_TEST(test_fpu_ip_fstsw_ax);
    RUN_TEST(test_fpu_ip_fnstcw_bx);
    RUN_TEST(test_fpu_ip_fnstsw_bx);
    RUN_TEST(test_fpu_ip_fldcw_disp16);
    RUN_TEST(test_fpu_ip_fadd_disp8);
    RUN_TEST(test_fpu_ip_fnop);
    RUN_TEST(test_fpu_ip_fwait);

    RUN_TEST(test_fpu_fninit_then_fnstcw_writes_037F);
    RUN_TEST(test_fpu_fninit_then_fnstsw_writes_0000);
    RUN_TEST(test_fpu_fninit_then_fstsw_ax_returns_0);
    RUN_TEST(test_fpu_fldcw_fnstcw_roundtrip);
    RUN_TEST(test_fpu_glabios_has_fpu_stack_safety);
    RUN_TEST(test_fpu_fnop_does_not_swallow_fld_sti);
}
