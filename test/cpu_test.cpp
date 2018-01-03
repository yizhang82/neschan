#include "stdafx.h"

#include "doctest.h"
#include "nes_trace.h"
#include "nes_mapper.h"
#include "nes_system.h"

using namespace std;

TEST_CASE("CPU tests") {
    nes_system system;

    SUBCASE("simple") {
        INIT_TRACE("neschan.instrtest.simple.log");

        cout << "Running [CPU][simple]..." << endl;

        system.power_on();

        // @TODO - We need an assembler to make testing easier
        system.run_program(
            {
                0xa9, 0x10,     // LDA #$10     -> A = #$10
                0x85, 0x20,     // STA $20      -> $20 = #$10
                0xa9, 0x01,     // LDA #$1      -> A = #$1
                0x65, 0x20,     // ADC $20      -> A = #$11
                0x85, 0x21,     // STA $21      -> $21=#$11
                0xe6, 0x21,     // INC $21      -> $21=#$12
                0xa4, 0x21,     // LDY $21      -> Y=#$12
                0xc8,           // INY          -> Y=#$13
                0x00,           // BRK 
            },
            0x1000);

        auto cpu = system.cpu();

        CHECK(cpu->peek(0x20) == 0x10);
        CHECK(cpu->peek(0x21) == 0x12);
        CHECK(cpu->A() == 0x11);
        CHECK(cpu->Y() == 0x13);
    }
    SUBCASE("simple_flags") {
        INIT_TRACE("neschan.instrtest.simple_flags.log");

        cout << "Running [CPU][simple_flags]..." << endl;

        system.power_on();

        system.run_program(
            {
                0xa9, 0xff,     // LDA #$ff
                0x85, 0x30,     // STA $30      -> $30 = #$ff
                0xa9, 0x01,     // LDA #$1
                0x65, 0x30,     // ADC $30      -> carry, A = 0
                0xa9, 0x01,     // LDA #$1      
                0x65, 0x30,     // ADC $30      -> carry, A = 1
                0x00,           // BRK 
            },
            0x1000);

        auto cpu = system.cpu();

        CHECK(cpu->A() == 1);
        CHECK((cpu->P() & PROCESSOR_STATUS_CARRY_MASK));
    }
    SUBCASE("nestest") {
        INIT_TRACE("neschan.instrtest.full.log");
        cout << "Running [CPU][nestest]..." << endl;

        system.power_on();

        system.run_rom("./roms/nestest/nestest.nes", nes_rom_exec_mode_direct);
        
        auto cpu = system.cpu();

        // Check we've proceeded to the end of the ROM
        CHECK(cpu->PC() == 0x0005);
        CHECK(cpu->S() == 0xff);

        // Check the test is successful
        CHECK(cpu->peek(0x2) == 0);
        CHECK(cpu->peek(0x3) == 0);
    }
#define INSTR_V5_TEST_CASE(test) \
    SUBCASE("instr_test-v5 " test) { \
        INIT_TRACE("neschan.instrtest.instr_test-v5." test ".log"); \
        cout << "Running [CPU][instr_test-v5-" << test << "]" << endl; \
        system.power_on(); \
        auto cpu = system.cpu(); \
        cpu->stop_at_infinite_loop(); \
        system.run_rom("./roms/instr_test-v5/rom_singles/" test ".nes", nes_rom_exec_mode_reset); \
        CHECK(cpu->peek(0x6000) == 0); \
    } 
    INSTR_V5_TEST_CASE("01-basics")
    INSTR_V5_TEST_CASE("02-implied")
    // INSTR_V5_TEST_CASE("03-immediate")
    INSTR_V5_TEST_CASE("04-zero_page")
    INSTR_V5_TEST_CASE("05-zp_xy")
    INSTR_V5_TEST_CASE("06-absolute")
    // INSTR_V5_TEST_CASE("07-abs_xy")
    INSTR_V5_TEST_CASE("08-ind_x")
    INSTR_V5_TEST_CASE("09-ind_y")
    INSTR_V5_TEST_CASE("10-branches")
    INSTR_V5_TEST_CASE("11-stack")
    INSTR_V5_TEST_CASE("12-jmp_jsr")
    INSTR_V5_TEST_CASE("13-rts")
    INSTR_V5_TEST_CASE("14-rti")
    // INSTR_V5_TEST_CASE("15-brk")
    // INSTR_V5_TEST_CASE("16-special")
}