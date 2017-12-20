#include "stdafx.h"

#include "doctest.h"
#include "trace.h"
#include "nes_mapper.h"
#include "nes_system.h"

using namespace std;

TEST_CASE("ppu_tests") {
    nes_system system;

    SUBCASE("color test") {
        INIT_TRACE("neschan.ppu.colortest.log");

        system.run_rom("./roms/color_test/color_test.nes", nes_rom_exec_mode_reset);

        auto cpu = system.cpu();
        auto ppu = system.ppu();

        // We've successfully reached the end of the ROM
        CHECK(cpu->PC() == 0x8153);

        CHECK(ppu->read_byte(0x3f00) == 0x0);
        CHECK(ppu->read_byte(0x3f01) == 0x16);
        CHECK(ppu->read_byte(0x3f01) == 0x2d);
        CHECK(ppu->read_byte(0x3f01) == 0x30);
    }
}