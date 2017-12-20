#pragma once

#include "stdafx.h"

#include "nes_ppu.h"
#include "nes_system.h"
#include "nes_memory.h"

nes_ppu_protect::nes_ppu_protect(nes_ppu *ppu)
{
    _ppu = ppu;
    _ppu->set_protect(true);
}

nes_ppu_protect::~nes_ppu_protect()
{
    _ppu->set_protect(false);
}

void nes_ppu::oam_dma(uint16_t addr)
{
    _system->ram()->get_bytes(_oam.get(), PPU_OAM_SIZE, addr, PPU_OAM_SIZE);
}

void nes_ppu::init()
{
    // PPUCTRL data
    _name_tbl_addr = 0;
    _bg_tbl_addr = 0;
    _ppu_addr_inc = 0;
    _vblank_nmi = false;
    _use_8x16_sprite = false;

    // PPUMASK
    _show_bg = false;
    _show_sprites = false;
    _gray_scale_mode = false;

    // PPUSTATUS
    _latch = 0;
    _sprite_overflow = false;
    _vblank_started = false;
    _sprite_0_hit = false;

    // OAMADDR, OAMDATA
    _oam_addr = 0;

    // PPUSCROLL
    _addr_latch = 0;
    _scroll_x = 0;
    _scroll_y = 0;

    // PPUADDR
    _ppu_addr = 0;

    _master_cycle = nes_cycle_t(0);
    _scanline_cycle = nes_cycle_t(0);
    _cur_scanline = 0;

    _frame_count = 0;
}

void nes_ppu::reset()
{
    init();
}

void nes_ppu::power_on(nes_system *system)
{
    LOG("[NES_PPU] POWER ON");

    init();

    _system = system;

    TRACE("[NES_PPU] SCANLINE " << std::dec << _cur_scanline << " ------ ");
}

void nes_ppu::step_to(nes_cycle_t count)
{
    while (_master_cycle < count && !_system->stop_requested())
    {     
        step_ppu(nes_ppu_cycle_t(1));

        if (_cur_scanline <= 239)
        {
            if (_scanline_cycle == nes_ppu_cycle_t(0))
            {
                // do nothing
            }
            else if (_scanline_cycle < nes_ppu_cycle_t(257))
            {
                // fetching tile for current line
                auto data_access_cycle = (_scanline_cycle - nes_ppu_cycle_t(1)) % 8;
                if (data_access_cycle == nes_ppu_cycle_t(0))
                {
                    // fetch nametable byte

                    // add one more cycle for memory access to skip directly to next access
                    step_ppu(nes_ppu_cycle_t(1));
                }
                else if (data_access_cycle == nes_ppu_cycle_t(3))
                {
                    // fetch attribute table byte

                    // add one more cycle for memory access to skip directly to next access
                    step_ppu(nes_ppu_cycle_t(1));
                }
                else if (data_access_cycle == nes_ppu_cycle_t(5))
                {
                    // fetch tile bitmap low
                    // add one more cycle for memory access to skip directly to next access
                    step_ppu(nes_ppu_cycle_t(1));
                }
                else if (data_access_cycle == nes_ppu_cycle_t(7))
                {
                    // fetch tilebitmap high
                    // add one more cycle for memory access to skip directly to next access
                    step_ppu(nes_ppu_cycle_t(1));
                }
            }
            else if (_scanline_cycle < nes_ppu_cycle_t(320))
            {
                // fetch tile data for sprites on the next scanline
            }
            else if (_scanline_cycle < nes_ppu_cycle_t(337))
            {
                // first two tiles for next scanline
            }
            else // 337->340
            {
                // fetch two bytes
            }
        }
        else if (_cur_scanline == 240)
        {
            // idles

        }
        else if (_cur_scanline < 261)
        {
            if (_cur_scanline == 241 && _scanline_cycle == nes_ppu_cycle_t(1))
            {
                TRACE("[PPU] SCANLINE = 241, VBlank BEGIN");
                _vblank_started = true;

                // @TODO - request CPU to do NMI
            }
        }
        else
        {
            if (_cur_scanline == 261 && _scanline_cycle == nes_ppu_cycle_t(1))
            {
                TRACE("[NES_PPU] SCANLINE = 261, VBlank END");
                _vblank_started = false;
            }

            // pre-render scanline
            if (_scanline_cycle == nes_ppu_cycle_t(340) && _frame_count % 2 == 1)
            {
                // odd frame skip the last cycle
                step_ppu(nes_ppu_cycle_t(1));
            }
        }
    }
}

void nes_ppu::step_ppu(nes_ppu_cycle_t count)
{
    assert(count < PPU_SCANLINE_CYCLE);

    _master_cycle += nes_ppu_cycle_t(count);
    _scanline_cycle += nes_ppu_cycle_t(count);
    if (_scanline_cycle >= PPU_SCANLINE_CYCLE)
    {
        _scanline_cycle %= PPU_SCANLINE_CYCLE;
        _cur_scanline++;
        if (_cur_scanline >= PPU_SCANLINE_COUNT)
        {
            _cur_scanline %= PPU_SCANLINE_COUNT;
            _frame_count++;
        }
        TRACE("[NES_PPU] SCANLINE " << std::dec << _cur_scanline << " ------ ");
    }
}