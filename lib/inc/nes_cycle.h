#pragma once

#include <cstdint>
#include <chrono>

using namespace std::chrono;

//
// NES 6502 CPU 21.477272 / 12 MHz
// NES PPU 21.477272 / 4 MHz
// @TODO - /4 is too fast so use /12. need to double check the math/facts
//
#define NES_CLOCK_HZ (21477272ul / 12)

//
// Given that 1 CPU cycle = 3 PPU cycle, we'll count in terms of PPU cycle
// 1 nes_cycle_t = 1 PPU cycle = 1/3 CPU cycle
// 3 nes_cycle_t = 3 PPU cycle = 1 CPU cycle
//
typedef duration<int64_t, std::ratio<1, 1>> nes_cycle_t;
typedef duration<int64_t, std::ratio<1, 1>> nes_ppu_cycle_t;
typedef duration<int64_t, std::ratio<3, 1>> nes_cpu_cycle_t;

static nes_cycle_t ms_to_nes_cycle(uint32_t ms)
{
    return nes_cycle_t(NES_CLOCK_HZ * ms / 1000);
}

#define PPU_SCANLINE_CYCLE nes_ppu_cycle_t(341)  