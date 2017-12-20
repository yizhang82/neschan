#pragma once

#include <cstdint>
#include <chrono>

using namespace std::chrono;

//
// Given that 1 CPU cycle = 3 PPU cycle, we'll count in terms of PPU cycle
// 1 nes_cycle_t = 1 PPU cycle = 1/3 CPU cycle
// 3 nes_cycle_t = 3 PPU cycle = 1 CPU cycle
//
typedef duration<int64_t, std::ratio<1, 1>> nes_cycle_t;
typedef duration<int64_t, std::ratio<1, 1>> nes_ppu_cycle_t;
typedef duration<int64_t, std::ratio<3, 1>> nes_cpu_cycle_t;

#define PPU_SCANLINE_CYCLE nes_ppu_cycle_t(341)  