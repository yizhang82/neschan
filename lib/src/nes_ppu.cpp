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
    _pattern_tbl_addr = 0;
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

    _protect_register = false;
    _frame_count = 0;
    _stop_after_frame = 0;
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

// fetching tile for current line
void nes_ppu::fetch_tile()
{
    // No need to fetch anything if rendering is off
    if (is_render_off()) return;

    auto scanline_render_cycle = nes_ppu_cycle_t(0);
    int cur_scanline = _cur_scanline;
    if (_scanline_cycle > nes_ppu_cycle_t(320))
    {
        // this is prefetch cycle 321~336 for next scanline
        scanline_render_cycle = _scanline_cycle - nes_ppu_cycle_t(321);
        cur_scanline = (cur_scanline + 1) % PPU_SCREEN_Y ;
    }
    else
    {
        // account for the prefetch happened in earlier scanline
        scanline_render_cycle = (_scanline_cycle - nes_ppu_cycle_t(1) + nes_ppu_cycle_t(16));
    }

    auto data_access_cycle = scanline_render_cycle % 8;

    // which 8x8 tile in the screen it is 
    uint16_t screen_tile_column = scanline_render_cycle.count() / 8;
    uint16_t screen_tile_row = cur_scanline / 8;

    // which of 8 rows witin a tile
    uint16_t screen_tile_row_index = cur_scanline % 8;
    if (data_access_cycle == nes_ppu_cycle_t(0))
    {
        // fetch nametable byte for current 8-pixel-tile
        // http://wiki.nesdev.com/w/index.php/PPU_nametables
        _tile_index = read_byte(_name_tbl_addr + (screen_tile_row << 5) + screen_tile_column);

        // add one more cycle for memory access to skip directly to next access
        step_ppu(nes_ppu_cycle_t(1));
    }
    else if (data_access_cycle == nes_ppu_cycle_t(2))
    {
        // fetch attribute table byte
        // each attribute pixel is 4 quadrant of 2x2 tile (so total of 8x8) tile
        // the result color byte is 2-bit (bit 3/2) for each quadrant
        // http://wiki.nesdev.com/w/index.php/PPU_attribute_tables
        uint16_t attr_tbl_addr = _name_tbl_addr + 0x3c0;
        uint8_t color_byte = read_byte((attr_tbl_addr | ((screen_tile_row >> 2) << 3) | (screen_tile_column >> 2)));

        uint8_t _quadrant_id = (screen_tile_row & 0x2) + ((screen_tile_column & 0x2) >> 1);
        uint8_t color_bit32 = (color_byte & (0x3 << (_quadrant_id * 2))) >> (_quadrant_id * 2); 
        _tile_palette_bit32 = color_bit32 << 2;

        // add one more cycle for memory access to skip directly to next access
        step_ppu(nes_ppu_cycle_t(1));
    }
    else if (data_access_cycle == nes_ppu_cycle_t(4))
    {
        // Pattern table is area of memory define all the tiles make up background and sprites.
        // Think it as "lego blocks" that you can build up your background and sprites which 
        // simply consists of indexes. It is quite convoluted by today's standards but it is 
        // just a space saving technique.
        // http://wiki.nesdev.com/w/index.php/PPU_pattern_tables

        _tile_addr = _pattern_tbl_addr | ((_tile_index & 0xf0) << 4) | ((_tile_index & 0xf) << 4);

        _bitplane0 = read_byte(_tile_addr | (/* bitplane = */0 << 3) | screen_tile_row_index);        // lower-bit

        // add one more cycle for memory access to skip directly to next access
        step_ppu(nes_ppu_cycle_t(1));
    }
    else if (data_access_cycle == nes_ppu_cycle_t(6))
    {
        // fetch tilebitmap high
        // add one more cycle for memory access to skip directly to next access
        uint8_t bitplane1 = read_byte(_tile_addr | (/* bitplane = */1 << 3) | screen_tile_row_index);        // higher-bit

        for (int i = 0; i < 8; ++i)
        {
            uint8_t tile_palette_bit01 = ((_bitplane0 >> i) & 0x1) | (((bitplane1 >> i) & 0x1) << 1);
            uint8_t color_4_bit = _tile_palette_bit32 | tile_palette_bit01;

            // @TODO - actually render this
            _pixel_cycle[i] = get_palette_color(/* is_background = */ true, color_4_bit);

            _frame_buffer[cur_scanline * 256 + screen_tile_column * 8 + i] = _pixel_cycle[i];
        }

        step_ppu(nes_ppu_cycle_t(1));
    }
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
                // do nothing and proceed to next scanline (optimization)
                step_ppu(PPU_SCANLINE_CYCLE - nes_ppu_cycle_t(1));
            }
            else if (_scanline_cycle < nes_ppu_cycle_t(257))
            {
                if (!is_ready())
                {
                    step_ppu(PPU_SCANLINE_CYCLE - nes_ppu_cycle_t(1));
                    continue;
                }

                fetch_tile();
            }
            else if (_scanline_cycle < nes_ppu_cycle_t(321))
            {
                if (!is_ready())
                {
                    step_ppu(PPU_SCANLINE_CYCLE - nes_ppu_cycle_t(1));
                    continue;
                }

                // fetch tile data for sprites on the next scanline
            }
            else if (_scanline_cycle < nes_ppu_cycle_t(337))
            {
                if (!is_ready())
                {
                    step_ppu(PPU_SCANLINE_CYCLE - nes_ppu_cycle_t(1));
                    continue;
                }

                // first two tiles for next scanline
                fetch_tile();
            }
            else // 337->340
            {
                // fetch two bytes - no need to emulate for now
                step_ppu(PPU_SCANLINE_CYCLE - nes_ppu_cycle_t(1));
            }
        }
        else if (_cur_scanline == 240)
        {
            // idles - jump straight to the next scanline
            step_ppu(PPU_SCANLINE_CYCLE - nes_ppu_cycle_t(1));
        }
        else if (_cur_scanline < 261)
        {
            if (_cur_scanline == 241 && _scanline_cycle == nes_ppu_cycle_t(1))
            {
                TRACE("[PPU] SCANLINE = 241, VBlank BEGIN");
                _vblank_started = true;
                if (_vblank_nmi)
                {
                    // Request NMI so that games can do their rendering
                    _system->cpu()->request_nmi();
                }
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
            TRACE("[NES_PPU] FRAME " << std::dec << _frame_count << " ------ ");

            if (_frame_count > _stop_after_frame)
            {
                LOG("[NES_PPU] FRAME exceeding " << std::dec << _stop_after_frame << " -> stopping...");
                _system->stop();
            }
        }
        TRACE("[NES_PPU] SCANLINE " << std::dec << _cur_scanline << " ------ ");
    }
}