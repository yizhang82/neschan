#include "stdafx.h"

#include "doctest.h"
#include "trace.h"

using namespace std;

TEST_CASE("instruction tests") {
    nes_memory ram;
    ram.initialize();
    nes_cpu cpu(ram);

    SUBCASE("simple LDA/STA/ADD") {
        INIT_TRACE("neschan.simple.log");

        // @TODO - We need an assembler to make testing easier
        cpu.load_program(
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
        cpu.run(0x1000);
        CHECK(cpu.peek(0x20) == 0x10);
        CHECK(cpu.peek(0x21) == 0x12);
        CHECK(cpu.A() == 0x11);
        CHECK(cpu.Y() == 0x13);
    }
}