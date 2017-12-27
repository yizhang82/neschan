#include "stdafx.h"

//
// Called when mapper is loaded into memory
// Useful when all you need is a one-time memcpy
//
void nes_mapper_mmc3::on_load_ram(nes_memory &mem)
{
    // $E000~$FFFF is always the last bank
    mem.set_bytes(0xe000, _prg_rom->data() + _prg_rom->size() - 0x2000, 0x2000);

    _mem = &mem;
}

//
// Called when mapper is loaded into PPU
// Useful when all you need is a one-time memcpy
//
void nes_mapper_mmc3::on_load_ppu(nes_ppu &ppu)
{
    _ppu = &ppu;
}

//
// Returns various mapper related flags
//
void nes_mapper_mmc3::get_info(nes_mapper_info &info)
{
    memset(&info, 0, sizeof(info));

    info.code_addr = 0x8000;

    info.reg_start = 0x8000;
    info.reg_end = 0xffff;

    info.flags = nes_mapper_flags_has_registers;
    if (_vertical_mirroring)
        info.flags = nes_mapper_flags(info.flags | nes_mapper_flags_vertical_mirroring);
}

void nes_mapper_mmc3::write_reg(uint16_t addr, uint8_t val)
{
    if (addr <= 0x9fff)
    {
        if (addr & 1)
            write_bank_data(val);
        else
            write_bank_select(val);
    }
    else if (addr <= 0xbfff)
    {
        if (addr & 1)
            write_prg_ram_protect(val);
        else
            write_mirroring(val);
    }
    else if (addr <= 0xdfff)
    {
        if (addr & 1)
            write_irq_reload(val);
        else
            write_irq_latch(val);
    }
    else
    {
        if (addr & 1)
            write_irq_enable(val);
        else
            write_irq_disable(val);
    }
}

/*
7  bit  0
---- ----
CPMx xRRR
|||   |||
|||   +++- Specify which bank register to update on next write to Bank Data register
|||        0: Select 2 KB CHR bank at PPU $0000-$07FF (or $1000-$17FF);
|||        1: Select 2 KB CHR bank at PPU $0800-$0FFF (or $1800-$1FFF);
|||        2: Select 1 KB CHR bank at PPU $1000-$13FF (or $0000-$03FF);
|||        3: Select 1 KB CHR bank at PPU $1400-$17FF (or $0400-$07FF);
|||        4: Select 1 KB CHR bank at PPU $1800-$1BFF (or $0800-$0BFF);
|||        5: Select 1 KB CHR bank at PPU $1C00-$1FFF (or $0C00-$0FFF);
|||        6: Select 8 KB PRG ROM bank at $8000-$9FFF (or $C000-$DFFF);
|||        7: Select 8 KB PRG ROM bank at $A000-$BFFF
||+------- Nothing on the MMC3, see MMC6
|+-------- PRG ROM bank mode (0: $8000-$9FFF swappable,
|                                $C000-$DFFF fixed to second-last bank;
|                             1: $C000-$DFFF swappable,
|                                $8000-$9FFF fixed to second-last bank)
+--------- CHR A12 inversion (0: two 2 KB banks at $0000-$0FFF,
four 1 KB banks at $1000-$1FFF;
1: two 2 KB banks at $1000-$1FFF,
four 1 KB banks at $0000-$0FFF)
*/
void nes_mapper_mmc3::write_bank_select(uint8_t val)
{
    _bank_select = val;
}

void nes_mapper_mmc3::write_mirroring(uint8_t val)
{
    _vertical_mirroring = !(val & 1);
    _ppu->set_mirroring(nes_mapper_flags(_vertical_mirroring? nes_mapper_flags_vertical_mirroring : nes_mapper_flags_horizontal_mirroring));
}

/*
*/
void nes_mapper_mmc3::write_bank_data(uint8_t val)
{
    // CHIP A12 inversion
    bool inversion = _bank_select & 0x80;

    int select = _bank_select & 0x7;
    bool prg_mode_changed = (_prev_prg_mode != (_bank_select & 0x40));
    _prev_prg_mode = _bank_select & 0x40;

    if (prg_mode_changed)
    {
        // the second last 8KB bank
        if (_bank_select & 0x40)
        {
            _mem->set_bytes(0x8000, _prg_rom->data() + _prg_rom->size() - 0x4000, 0x2000);
        }
        else
        {
            _mem->set_bytes(0xc000, _prg_rom->data() + _prg_rom->size() - 0x4000, 0x2000);
        }
    }

    if (select >= 6)
    {
        // ignore bit 6 and 7 - the register is incompleted decoded
        val &= ~0xb0;

        // 8KB bank
        uint32_t offset = val << 13;
        uint32_t addr;
        uint32_t size = 1 << 13;

        if (select == 6)
        {
            if (_bank_select & 0x40)
                addr = 0xc000;
            else
                addr = 0x8000;
        }
        else
        {
            assert(select == 7);
            addr = 0xa000;
        }

        if (_prg_rom->size() < offset + size)
            return;

        _mem->set_bytes(addr, _prg_rom->data() + offset, size);
    }
    else
    {
        uint16_t ppu_addr;
        uint16_t ppu_size;
        uint32_t offset;
        uint8_t bits;
        switch (select)
        {
        case 0:
            // 2KB $0000-$07FF
            ppu_addr = 0;
            bits = 10;

            // ignore bit 0 - as if this is 2KB mode
            val &= ~0x1;
            break;
        case 1:
            // 2KB $0800~$0FFF
            ppu_addr = 0x800;
            bits = 10;

            // ignore bit 0 - as if this is like 2KB mode
            val &= ~0x1;
            break;

        case 2:
            // 1KB $1000~$13FF
            ppu_addr = 0x1000;
            bits = 10;
            break;
        case 3:
            // 1KB $1400~$17FF
            ppu_addr = 0x1400;
            bits = 10;
            break;
        case 4:
            // 1KB $1800~$1BFF
            ppu_addr = 0x1800;
            bits = 10;
            break;
        case 5:
            // 1KB $1C00~$1FFF
            ppu_addr = 0x1c00;
            bits = 10;
            break;
        default:
            assert(false);
        }

        ppu_size = 1 << bits;
        offset = uint32_t(val) << bits;
        if (inversion)
            ppu_addr ^= 0x1000;

        if (_chr_rom->size() < offset + ppu_size)
            return;

        _ppu->write_bytes(ppu_addr, _chr_rom->data() + offset, ppu_size);
    }
}

