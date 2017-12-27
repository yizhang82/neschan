#include "stdafx.h"

//
// Called when mapper is loaded into memory
// Useful when all you need is a one-time memcpy
//
void nes_mapper_mmc1::on_load_ram(nes_memory &mem)
{
    mem.set_bytes(0x8000, _prg_rom->data() + _prg_rom->size() - 0x8000, 0x8000);

    _mem = &mem;
}

//
// Called when mapper is loaded into PPU
// Useful when all you need is a one-time memcpy
//
void nes_mapper_mmc1::on_load_ppu(nes_ppu &ppu)
{
    _ppu = &ppu;
}

//
// Returns various mapper related flags
//
void nes_mapper_mmc1::get_info(nes_mapper_info &info)
{
    memset(&info, 0, sizeof(info));

    if (_prg_rom->size() == 0x4000)
        info.code_addr = 0xc000;
    else
        info.code_addr = 0x8000;

    info.reg_start = 0x8000;
    info.reg_end = 0xffff;

    info.flags = nes_mapper_flags_has_registers;
    if (_vertical_mirroring)
        info.flags = nes_mapper_flags(info.flags | nes_mapper_flags_vertical_mirroring);
}

void nes_mapper_mmc1::write_reg(uint16_t addr, uint8_t val) 
{
    if (val & 0x80)
    {
        // clears register to initial state
        _bit_latch = 0;
        _reg = 0x10;
        return;
    }

    // this is a serial port "register" - need to write bit-by-bit
    // lowest-bit first
    _bit_latch++;
    _reg >>= 1;
    _reg |= ((val & 1) << 4);
    if (_bit_latch == 5)
    {
        // fifth write - we only need the 5 bits
        _bit_latch = 0;

        if (addr <= 0x9fff)
            write_control(_reg);
        else if (addr <= 0xbfff)
            write_chr_bank_0(_reg);
        else if (addr <= 0xdfff)
            write_chr_bank_1(_reg);
        else
            write_prg_bank(_reg);
    }
}

/*
Control (internal, $8000-$9FFF)
4bit0
-----
CPPMM
|||||
|||++- Mirroring (0: one-screen, lower bank; 1: one-screen, upper bank;
|||               2: vertical; 3: horizontal)
|++--- PRG ROM bank mode (0, 1: switch 32 KB at $8000, ignoring low bit of bank number;
|                         2: fix first bank at $8000 and switch 16 KB bank at $C000;
|                         3: fix last bank at $C000 and switch 16 KB bank at $8000)
+----- CHR ROM bank mode (0: switch 8 KB at a time; 1: switch two separate 4 KB banks)
*/
void nes_mapper_mmc1::write_control(uint8_t val)
{
    _control = val;

    _ppu->set_mirroring(nes_mapper_flags(_control & nes_mapper_flags_mirroring_mask));
}

/*
CHR bank 0 (internal, $A000-$BFFF)
4bit0
-----
CCCCC
|||||
+++++- Select 4 KB or 8 KB CHR bank at PPU $0000 (low bit ignored in 8 KB mode)
MMC1 can do CHR banking in 4KB chunks. Known carts with CHR RAM have 8 KiB, so that makes 2 banks. RAM vs ROM doesn't make any difference for address lines. For carts with 8 KiB of CHR (be it ROM or RAM), MMC1 follows the common behavior of using only the low-order bits: the bank number is in effect ANDed with 1.
*/
void nes_mapper_mmc1::write_chr_bank_0(uint8_t val)
{
    uint32_t addr;
    uint16_t size;
    if (_control & 0x10)
    {
        // 4KB mode
        addr = (val & 0x1f) << 12;
        size = 0x1000;
    }
    else
    {
        // 8KB mode
        addr = (val & 0x1e) << 12;
        size = 0x2000;
    }

    if (_chr_rom->size() < addr + size)
        return;

    _ppu->write_bytes(0x0000, _chr_rom->data() + addr, size);
}

/*
CHR bank 1 (internal, $C000-$DFFF)
4bit0
-----
CCCCC
|||||
+++++- Select 4 KB CHR bank at PPU $1000 (ignored in 8 KB mode)
*/
void nes_mapper_mmc1::write_chr_bank_1(uint8_t val)
{
    if (_control & 0x10)
    {
        // 4KB mode only
        // 8KB mode is ignored completely
        uint32_t addr = (val & 0x1f) << 12;
        uint16_t size = 0x1000;
        if (_chr_rom->size() < addr + size)
            return;

        _ppu->write_bytes(0x1000, _chr_rom->data() + addr, size);
    }
}

/*
PRG bank (internal, $E000-$FFFF)
4bit0
-----
RPPPP
|||||
|++++- Select 16 KB PRG ROM bank (low bit ignored in 32 KB mode)
+----- PRG RAM chip enable (0: enabled; 1: disabled; ignored on MMC1A)
*/
void nes_mapper_mmc1::write_prg_bank(uint8_t val)
{
    if (_control & 0x8)
    {
        // 16KB mode
        if (_control & 0x4)
        {
            // fix last bank at $C000 and switch 16KB bank at $8000
            _mem->set_bytes(0x8000, _prg_rom->data() + (val & 0xf) * 0x4000, 0x4000);
            _mem->set_bytes(0xc000, _prg_rom->data() + _prg_rom->size() - 0x4000, 0x4000);
        }
        else
        {
            // fix first bank at $8000 and switch 16KB bank at $C000
            _mem->set_bytes(0x8000, _prg_rom->data(), 0x4000);
            _mem->set_bytes(0xc000, _prg_rom->data() + (val & 0xf) * 0x4000, 0x4000);
        }
    }
    else
    {
        // 32KB mode at $8000
        _mem->set_bytes(0x8000, _prg_rom->data() + (val & 0xe) * 0x4000, 0x8000);
    }
}