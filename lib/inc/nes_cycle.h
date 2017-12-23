#pragma once

#include <cstdint>
#include <chrono>

using namespace std::chrono;

//
// NES 6502 CPU 21.477272 / 12 MHz
// NES PPU 21.477272 / 4 MHz
//
#define NES_CLOCK_HZ (21477272ll / 4)

//
// Given that 1 CPU cycle = 3 PPU cycle, we'll count in terms of PPU cycle
// 1 nes_cycle_t = 1 PPU cycle = 1/3 CPU cycle
// 3 nes_cycle_t = 3 PPU cycle = 1 CPU cycle
//
typedef duration<int64_t, std::ratio<1, 1>> nes_cycle_t;
typedef duration<int64_t, std::ratio<1, 1>> nes_ppu_cycle_t;
typedef duration<int64_t, std::ratio<3, 1>> nes_cpu_cycle_t;

// use double for better precision
static nes_cycle_t ms_to_nes_cycle(double ms)
{
    return nes_cycle_t(int64_t(NES_CLOCK_HZ / 1000 * ms));
}

#define PPU_SCANLINE_CYCLE nes_ppu_cycle_t(341)  