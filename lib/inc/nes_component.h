#pragma once

#include <cstdint>

//
// Given that 1 CPU cycle = 3 PPU cycle, we'll count in terms of PPU cycle
// 1 nes_cycle_t = 1 PPU cycle = 1/3 CPU cycle
// 3 nes_cycle_t = 3 PPU cycle = 1 CPU cycle
//
typedef int64_t nes_cycle_t;

#define CPU_CYCLE_RATIO      0x3
#define PPU_CYCLE_RATIO      0x1

class nes_system;

class nes_component
{
public :
    virtual void power_on(nes_system *system) = 0;

    virtual void reset() = 0;

    virtual void step_to(nes_cycle_t count) = 0;
};
