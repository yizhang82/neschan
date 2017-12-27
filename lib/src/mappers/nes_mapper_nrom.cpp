#include "stdafx.h"

//
// Called when mapper is loaded into memory
// Useful when all you need is a one-time memcpy
//
void nes_mapper_nrom::on_load_ram(nes_memory &mem)
{
    // memcpy
    mem.set_bytes(0x8000, _prg_rom->data(), _prg_rom->size());

    if (_prg_rom->size() == 0x4000)
    {
        // "map" 0xC000 to 0x8000
        mem.set_bytes(0xc000, _prg_rom->data(), _prg_rom->size());
    }
}

//
// Called when mapper is loaded into PPU
// Useful when all you need is a one-time memcpy
//
void nes_mapper_nrom::on_load_ppu(nes_ppu &ppu)
{
    // memcpy
    ppu.write_bytes(0x0000, _chr_rom->data(), _chr_rom->size());
}

//
// Returns various mapper related flags
//
void nes_mapper_nrom::get_info(nes_mapper_info &info)
{
    memset(&info, 0, sizeof(info));

    if (_prg_rom->size() == 0x4000)
        info.code_addr = 0xc000;
    else
        info.code_addr = 0x8000;

    info.flags = nes_mapper_flags_none;
    if (_vertical_mirroring)
        info.flags = nes_mapper_flags(info.flags | nes_mapper_flags_vertical_mirroring);
    else
        info.flags = nes_mapper_flags(info.flags | nes_mapper_flags_horizontal_mirroring);
}
