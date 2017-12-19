#pragma once

#include <cstdint>
#include <memory>

#include "nes_component.h"

class nes_cpu;
class nes_memory;
class nes_apu;
class nes_ppu;

//
// The NES system hardware that manages all the invidual components - CPU, PPU, APU, RAM, etc
// It synchronizes between different components
//
class nes_system
{
public :
    nes_system();
    ~nes_system();

public :
    void power_on();
    void reset();

    // Main emulation loop
    void main_loop();

    // Stop the emulation engine and exit the main loop
    void stop() { _stop_requested = true; }

    void run_program(vector<uint8_t> &&program, uint16_t addr);

    void run_rom(const char *rom_path);
   
    nes_cpu     *cpu() { return _cpu.get(); }
    nes_memory  *ram() { return _ram.get(); }
    nes_ppu     *ppu() { return _ppu.get(); }

private :

    //
    // step <count> amount of cycles
    // We have a few options:
    // 1. Let nes_system drive the cycle - and each component own their own stepping towards that cycle
    // 2. Let CPU drive cycle - and other component "catch up"
    // 3. Let each component own their own thread - and synchronizes at cycle granuarity
    //
    // I think option #1 will produce the most accurate timing without subjecting too much to OS resource
    // management.
    //
    void step(nes_cycle_t count);

private :
    nes_cycle_t _master_cycle;

    unique_ptr<nes_cpu> _cpu;
    unique_ptr<nes_memory> _ram;
    unique_ptr<nes_ppu> _ppu;

    vector<nes_component *> _components;
    bool _stop_requested;
};
