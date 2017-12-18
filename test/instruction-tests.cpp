#include "stdafx.h"

#include "doctest.h"
#include "trace.h"
#include "nes_mapper.h"

using namespace std;

TEST_CASE("instruction tests") {
    nes_memory ram;
    ram.initialize();
    nes_cpu cpu(ram);

    SUBCASE("simple LDA/STA/ADD") {
        INIT_TRACE("neschan.instrtest.simple.log");

        // @TODO - We need an assembler to make testing easier
        cpu.run_program(
            {
                0xa9, 0x10,     // LDA 0x10
                0x85, 0x20,     // STA (0x20)
                0xa9, 0x01,     // LDA 0x01
                0x65, 0x20,     // ADD (0x20)
                0x85, 0x21,     // STA (0x21)
                0xe6, 0x21,     // INC (0x21)
                0xa4, 0x21,     // LDY (0x21)
                0xc8,           // INY
                0x00
            },
            0x1000);
        CHECK(cpu.peek(0x20) == 0x10);
        CHECK(cpu.peek(0x21) == 0x12);
        CHECK(cpu.A() == 0x11);
        CHECK(cpu.Y() == 0x13);
    }
    SUBCASE("full instruction test") {
        INIT_TRACE("neschan.instrtest.full.log");

        cpu.run_rom("./roms/nestest/nestest.nes");
    }

}