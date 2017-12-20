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

        system.power_on();

        // @TODO - need to support propery PPU initialization sequence / state machine
        // system.run_rom("./roms/color_test/color_test.nes", nes_rom_exec_mode_reset);

        auto cpu = system.cpu();
    }
}