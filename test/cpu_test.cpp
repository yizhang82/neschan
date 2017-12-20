#include "stdafx.h"

#include "doctest.h"
#include "trace.h"
#include "nes_mapper.h"
#include "nes_system.h"

using namespace std;

TEST_CASE("CPU tests") {
    nes_system system;

    SUBCASE("simple LDA/STA/ADD") {
        INIT_TRACE("neschan.instrtest.simple.log");

        // @TODO - We need an assembler to make testing easier
        system.run_program(
            {
                0xa9, 0x10,     // LDA 0x10
                0x85, 0x20,     // STA (0x20)
                0xa9, 0x01,     // LDA 0x01
                0x65, 0x20,     // ADD (0x20)
                0x85, 0x21,     // STA (0x21)
                0xe6, 0x21,     // INC (0x21)
                0xa4, 0x21,     // LDY (0x21)
                0xc8,           // INY
                0x60,           // RTS
            },
            0x1000);

        auto cpu = system.cpu();

        CHECK(cpu->peek(0x20) == 0x10);
        CHECK(cpu->peek(0x21) == 0x12);
        CHECK(cpu->A() == 0x11);
        CHECK(cpu->Y() == 0x13);
    }
    SUBCASE("full instruction test") {
        INIT_TRACE("neschan.instrtest.full.log");

        system.run_rom("./roms/nestest/nestest.nes", nes_rom_exec_mode_direct);
        
        auto cpu = system.cpu();

        // Check we've proceeded to the end of the ROM
        CHECK(cpu->PC() == 0xc66f);
        CHECK(cpu->S() == 0xfd);

        // Check the test is successful
        CHECK(cpu->peek(0x2) == 0);
        CHECK(cpu->peek(0x3) == 0);
    }
}