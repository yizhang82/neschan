#include "stdafx.h"

// This file is (C) Copyright 2023 Robert Kolski
// Open source license - no warranty of any kind.
// MIT License

#define NROM_383_START_BYTE 0x4020
#define NROM_368_START_BYTE 0x4800
#define NROM_320_START_BYTE 0x6000

#define NROM_383_PADDING_BYTES 0x20
#define NROM_368_PADDING_BYTES 0x800
#define NROM_320_PADDING_BYTES 0x2000

//
// Called when mapper is loaded into memory
// Useful when all you need is a one-time memcpy
//
void nes_mapper_474::on_load_ram(nes_memory& mem)
{
    // memcpy
    switch (_submapper_number)
    {
        case 0:
            mem.set_bytes(NROM_383_START_BYTE, _prg_rom->data() + NROM_383_PADDING_BYTES, _prg_rom->size() - NROM_383_PADDING_BYTES);
            break;
        case 1:
            mem.set_bytes(NROM_368_START_BYTE, _prg_rom->data() + NROM_368_PADDING_BYTES, _prg_rom->size() - NROM_368_PADDING_BYTES);
            break;
        case 2:
            mem.set_bytes(NROM_320_START_BYTE, _prg_rom->data() + NROM_320_PADDING_BYTES, _prg_rom->size() - NROM_320_PADDING_BYTES);
            break;
        case 3:
            mem.set_bytes(NROM_320_START_BYTE, _prg_rom->data() + NROM_320_PADDING_BYTES, _prg_rom->size() - NROM_320_PADDING_BYTES);
            break;
        default:
            assert("Unsupported submapper number!");
            break;
    }
}

//
// Called when mapper is loaded into PPU
// Useful when all you need is a one-time memcpy
//
void nes_mapper_474::on_load_ppu(nes_ppu& ppu)
{
    // memcpy
    ppu.write_bytes(0x0000, _chr_rom->data(), _chr_rom->size());
}

//
// Returns various mapper related flags
//
void nes_mapper_474::get_info(nes_mapper_info& info)
{
    memset(&info, 0, sizeof(info));

    switch (_submapper_number)
    {
        case 0:
            info.code_addr = NROM_383_START_BYTE;
            break;
        case 1:
            info.code_addr = NROM_368_START_BYTE;
            break;
        case 2:
            info.code_addr = NROM_320_START_BYTE;
            break;
        case 3:
            info.code_addr = NROM_320_START_BYTE;
            break;
        default:
            assert("Unsupported submapper number!");
            break;
    }

    info.flags = nes_mapper_flags_none;
    if (_vertical_mirroring)
        info.flags = nes_mapper_flags(info.flags | nes_mapper_flags_vertical_mirroring);
    else
        info.flags = nes_mapper_flags(info.flags | nes_mapper_flags_horizontal_mirroring);
}
