#include "stdafx.h"

#include "nes_cpu.h"
#include "nes_system.h"
#include "nes_ppu.h"

using namespace std;

nes_system::nes_system()
{
    _ram = make_unique<nes_memory>();
    _cpu = make_unique<nes_cpu>();
    _ppu = make_unique<nes_ppu>();

    _components.push_back(_ram.get());
    _components.push_back(_cpu.get());
    _components.push_back(_ppu.get());

    _stop_requested = false;
}
                         
nes_system::~nes_system() {}

void nes_system::power_on()
{
    _stop_requested = false;

    for (auto comp : _components)
        comp->power_on(this);
}

void nes_system::reset()
{
    for (auto comp : _components)
        comp->reset();
}

void nes_system::run_program(vector<uint8_t> &&program, uint16_t addr)
{
    _ram->set_bytes(addr, program.data(), program.size());
    _cpu->PC() = addr;

    main_loop();
}

void nes_system::run_rom(const char *rom_path, nes_rom_exec_mode mode)
{
    auto mapper = nes_rom_loader::load_from(rom_path);
    _ram->load_mapper(mapper);
    if (mode == nes_rom_exec_mode_direct)
    {
        _cpu->PC() = mapper->get_code_addr();
    }
    else 
    {
        assert(mode == nes_rom_exec_mode_reset);
        _cpu->PC() = ram()->get_word(RESET_HANDLER);
    }

    main_loop();
}

void nes_system::main_loop()
{
    auto tick = nes_cycle_t(1);
    while (!_stop_requested)
    {
        step(tick);

        // @TODO - add proper time synchronization/delay
    }
}

void nes_system::step(nes_cycle_t count)
{
    _master_cycle += count;

    for (auto comp : _components)
        comp->step_to(_master_cycle);
}