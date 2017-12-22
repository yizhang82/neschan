#include "stdafx.h"

#include "doctest.h"
#include "nes_trace.h"
#include "nes_mapper.h"
#include "nes_system.h"

using namespace std;

TEST_CASE("ppu_tests") {
    nes_system system;

    SUBCASE("color test") {
        INIT_TRACE("neschan.ppu.colortest.log");

        system.power_on();

        // The test infinite loops and does rendering in NMI
        // Wait for 10 frames so that it can finish rendering - and we can use the VRAM value to validate
        system.ppu()->stop_after_frame(10);

        system.run_rom("./roms/color_test/color_test.nes", nes_rom_exec_mode_reset);

        auto cpu = system.cpu();
        auto ppu = system.ppu();

        CHECK(cpu->PC() == 0x8153);
        CHECK(ppu->read_byte(0x3f00) == 0x0);
        CHECK(ppu->read_byte(0x3f01) == 0x16);
        CHECK(ppu->read_byte(0x3f02) == 0x2d);
        CHECK(ppu->read_byte(0x3f03) == 0x30);
        CHECK(ppu->read_byte(0x3f13) == 0x30);
    }
    SUBCASE("vbl_clear_time") {
        INIT_TRACE("neschan.ppu.vbl_clear_time.log");

        system.power_on();

        system.ppu()->stop_after_frame(10);

        system.run_rom("./roms/blargg_ppu_tests/vbl_clear_time.nes", nes_rom_exec_mode_reset);

        auto cpu = system.cpu();

        CHECK(cpu->peek(0xf0) == 0x1);
    }
    SUBCASE("sprite_ram") {
        INIT_TRACE("neschan.ppu.sprite_ram.log");

        system.power_on();

        system.ppu()->stop_after_frame(10);

        system.run_rom("./roms/blargg_ppu_tests/sprite_ram.nes", nes_rom_exec_mode_reset);

        auto cpu = system.cpu();

        CHECK(cpu->peek(0xf0) == 0x1);
    }
    SUBCASE("vram_access") {
        INIT_TRACE("neschan.ppu.vram_access.log");

        system.power_on();

        system.ppu()->stop_after_frame(10);

        system.run_rom("./roms/blargg_ppu_tests/vram_access.nes", nes_rom_exec_mode_reset);

        auto cpu = system.cpu();

        CHECK(cpu->peek(0xf0) == 0x1);
    }
    SUBCASE("palette_ram") {
        INIT_TRACE("neschan.ppu.palette_ram.log");

        system.power_on();

        system.ppu()->stop_after_frame(10);

        system.run_rom("./roms/blargg_ppu_tests/palette_ram.nes", nes_rom_exec_mode_reset);

        auto cpu = system.cpu();

        CHECK(cpu->peek(0xf0) == 0x1);
    }
}