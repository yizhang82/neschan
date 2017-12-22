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

nes_ppu::~nes_ppu()
{
    _oam = nullptr;
    _vram = nullptr;
}

void nes_ppu::write_OAMDMA(uint8_t val)
{
    // @TODO - CPU is suspended and take 513/514 cycle
    _system->cpu()->request_dma((uint16_t(val) << 8));
}

void nes_ppu::oam_dma(uint16_t addr)
{
    if (_oam_addr == 0)
    {
        // simple case - copy the 0x100 bytes directly
        _system->ram()->get_bytes(_oam.get(), PPU_OAM_SIZE, addr, PPU_OAM_SIZE);
    }
    else
    {
        // the copy starts at _oam_addr and wraps around
        int copy_before_wrap = 0x100 - _oam_addr;
        _system->ram()->get_bytes(_oam.get() + _oam_addr, copy_before_wrap, addr, copy_before_wrap);
        _system->ram()->get_bytes(_oam.get(), PPU_OAM_SIZE - copy_before_wrap, addr + copy_before_wrap, PPU_OAM_SIZE - copy_before_wrap);
    }
}

void nes_ppu::load_mapper(shared_ptr<nes_mapper> &mapper)
{
    // unset previous mapper
    _mapper = nullptr;

    // Give mapper a chance to copy all the bytes needed
    mapper->on_load_ppu(*this);

    _mapper = mapper;
}

void nes_ppu::init()
{
    // PPUCTRL data
    _name_tbl_addr = 0;
    _bg_pattern_tbl_addr = 0;
    _ppu_addr_inc = 1;
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
    _stop_after_frame = -1;
    _auto_stop = false;

    _mask_oam_read = false;
    _frame_buffer = _frame_buffer_1;
}

void nes_ppu::reset()
{
    init();
}

void nes_ppu::power_on(nes_system *system)
{
    NES_TRACE1("[NES_PPU] POWER ON");

    init();

    _system = system;

    NES_TRACE3("[NES_PPU] SCANLINE " << std::dec << _cur_scanline << " ------ ");
}

// fetching tile for current line
void nes_ppu::fetch_tile()
{
    auto scanline_render_cycle = nes_ppu_cycle_t(0);
    uint8_t cur_scanline = _cur_scanline;
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
    uint8_t screen_tile_column = (uint8_t) scanline_render_cycle.count() / 8;
    uint8_t screen_tile_row = cur_scanline / 8;

    // which of 8 rows witin a tile
    uint8_t screen_tile_row_index = cur_scanline % 8;
    if (data_access_cycle == nes_ppu_cycle_t(0))
    {
        // fetch nametable byte for current 8-pixel-tile
        // http://wiki.nesdev.com/w/index.php/PPU_nametables
        _tile_index = read_byte(_name_tbl_addr + (screen_tile_row << 5) + screen_tile_column);
    }
    else if (data_access_cycle == nes_ppu_cycle_t(2))
    {
        // fetch attribute table byte
        // each attribute pixel is 4 quadrant of 2x2 tile (so total of 8x8) tile
        // the result color byte is 2-bit (bit 3/2) for each quadrant
        // http://wiki.nesdev.com/w/index.php/PPU_attribute_tables
        uint16_t attr_tbl_addr = _name_tbl_addr + 0x3c0;
        uint8_t color_byte = read_byte((attr_tbl_addr + ((screen_tile_row >> 2) << 3) + (screen_tile_column >> 2)));

        uint8_t _quadrant_id = ((screen_tile_row & 0x1) << 1) + (screen_tile_column & 0x1);
        uint8_t color_bit32 = (color_byte & (0x3 << (_quadrant_id * 2))) >> (_quadrant_id * 2); 
        _tile_palette_bit32 = color_bit32 << 2;
    }
    else if (data_access_cycle == nes_ppu_cycle_t(4))
    {
        // Pattern table is area of memory define all the tiles make up background and sprites.
        // Think it as "lego blocks" that you can build up your background and sprites which 
        // simply consists of indexes. It is quite convoluted by today's standards but it is 
        // just a space saving technique.
        // http://wiki.nesdev.com/w/index.php/PPU_pattern_tables
        _bitplane0 = read_pattern_table_column(/* sprite = */false, _tile_index, /* bitplane = */ 0, screen_tile_row_index);
    }
    else if (data_access_cycle == nes_ppu_cycle_t(6))
    {
        // fetch tilebitmap high
        // add one more cycle for memory access to skip directly to next access
        uint8_t bitplane1 = read_pattern_table_column(/* sprite = */false, _tile_index, /* bitplane = */ 1, screen_tile_row_index);

        // for each column - bitplane0/bitplane1 has entire 8 column
        // high bit -> low bit
        for (int i = 7; i >= 0; --i)
        {
            uint8_t column_mask = 1 << i;
            uint8_t tile_palette_bit01 = ((_bitplane0 & column_mask) >> i) | ((bitplane1 & column_mask) >> i << 1);
            uint8_t color_4_bit = _tile_palette_bit32 | tile_palette_bit01;

            _pixel_cycle[i] = get_palette_color(/* is_background = */ true, color_4_bit);

            uint16_t frame_addr = uint16_t(cur_scanline) * PPU_SCREEN_X + screen_tile_column * 8 + (7 - i);
            if (frame_addr >= sizeof(_frame_buffer_1))
                continue;
            _frame_buffer[frame_addr] = _pixel_cycle[i];
        }
    }
}

void nes_ppu::fetch_tile_pipeline()
{
    // No need to fetch anything if rendering is off
    if (!_show_bg) return;

    if (_scanline_cycle == nes_ppu_cycle_t(0))
    {
    }
    else if (_scanline_cycle < nes_ppu_cycle_t(257))
    {
        fetch_tile();
    }
    else if (_scanline_cycle < nes_ppu_cycle_t(321))
    {
        // fetch tile data for sprites on the next scanline
    }
    else if (_scanline_cycle < nes_ppu_cycle_t(337))
    {
        // first two tiles for next scanline
        fetch_tile();
    }
    else // 337->340
    {
        // fetch two bytes - no need to emulate for now
    }
}

void nes_ppu::fetch_sprite_pipeline()
{
    if (!_show_sprites)
        return;

    // sprite never show up in scanline 0
    if (_cur_scanline == 0)
        return;

    // @TODO - Sprite 0 hit testing
    if (_scanline_cycle == nes_ppu_cycle_t(0))
    {
        _last_sprite_id = 0;
        memset(_sprite_buf, 0xff, sizeof(_sprite_buf));
        _sprite_overflow = false;
    }
    else if (_scanline_cycle < nes_ppu_cycle_t(65))
    {
        // initialize secondary OAM (_sprite_buf)
        // we've already done this earlier and the side effect isn't observable anyway

        // NOTE: We can set this conditionaly but it's simply faster always to set it
        _mask_oam_read = true;
    }
    else if (_scanline_cycle < nes_ppu_cycle_t(257))
    {
        // fetch tile data for sprites on the next scanline

        // NOTE: We can set this conditionaly but it's simply faster always to set it
        _mask_oam_read = false;

        uint8_t sprite_cycle = (uint8_t) (_scanline_cycle.count() - 65);
        uint8_t sprite_id = sprite_cycle / 2;

        // Cycle 65-256 has more than enough cycles to read all 64 sprites - but it appears that this
        // pipeline would synchronize with the background rendering pipeline so that sprites rendering
        // happens later and can freely overwrite background if needed.
        // Skip if we are already past 64
        if (sprite_id >= PPU_SPRITE_MAX)
            return;

        if ((_scanline_cycle.count() % 2) == 0)
        {
            // even cycle - write to secondary OAM
            // if in range
            if (_sprite_pos_y <= _cur_scanline && _cur_scanline < _sprite_pos_y + 8)
            {
                if (_last_sprite_id >= PPU_ACTIVE_SPRITE_MAX)
                    _sprite_overflow = true;
                else
                    _sprite_buf[_last_sprite_id++] = *get_sprite(sprite_id);
            }
        }
        else
        {
            // odd cycle - read from primary OAM
            _sprite_pos_y = get_sprite(sprite_id)->pos_y;            
        }
    }
    else if (_scanline_cycle < nes_ppu_cycle_t(321))
    {
        uint8_t sprite_cycle = (uint8_t) (_scanline_cycle.count() - 257);
        uint8_t sprite_id = sprite_cycle / 8;
        if (sprite_cycle % 8 == 4)
        {
            if (sprite_id < _last_sprite_id)
                fetch_sprite(sprite_id);
        }
    }
    else // 337->340
    {
        // fetch two bytes - no need to emulate for now
    }
}

void nes_ppu::fetch_sprite(uint8_t sprite_id)
{
    assert(sprite_id < PPU_ACTIVE_SPRITE_MAX);
    assert(!_use_8x16_sprite);

    sprite_info *sprite = &_sprite_buf[sprite_id];
    uint8_t tile_index = sprite->tile_index;

    uint8_t tile_row_index = (_cur_scanline - sprite->pos_y) % 8;
    if (sprite->attr & PPU_SPRITE_ATTR_VERTICAL_FLIP)
        tile_row_index = 7 - tile_row_index;

    uint8_t bitplane0 = read_pattern_table_column(/* sprite = */ true, tile_index, 0, tile_row_index);
    uint8_t bitplane1 = read_pattern_table_column(/* sprite = */ true, tile_index, 1, tile_row_index);

    // bit3/2 is shared for the entire sprite (just like background attribute table)
    uint8_t palette_bit32 = (sprite->attr & PPU_SPRITE_ATTR_BIT32_MASK) << 2;

    // loop all bits - high -> low
    for (int i = 7; i >= 0; --i)
    {
        uint8_t column_mask = 1 << i;
        uint8_t palette_index = palette_bit32 | ((bitplane1 & column_mask) >> i << 1) | ((bitplane0 & column_mask) >> i);

        uint8_t color = get_palette_color(/* is_background = */false, palette_index);

        // color 0 = transparent for sprites
        uint16_t frame_addr = _cur_scanline * PPU_SCREEN_X + sprite->pos_x;
        if (sprite->attr & PPU_SPRITE_ATTR_HORIZONTAL_FLIP)
            frame_addr += i;     // low -> high in horizontal flip
        else
            frame_addr += 7 - i; // high -> low as usual

        if (frame_addr >= sizeof(_frame_buffer_1))
        {
            // part of the sprite might be over
            continue;
        }

        if (sprite->attr & PPU_SPRITE_ATTR_BEHIND_BG)
        {
            if (_frame_buffer[frame_addr] != 0)
                continue;
        }

        _frame_buffer[frame_addr] = color;
    }
}

void nes_ppu::step_to(nes_cycle_t count)
{
    while (_master_cycle < count && !_system->stop_requested())
    {     
        step_ppu(nes_ppu_cycle_t(1));

        if (_cur_scanline <= 239)
        {
            fetch_tile_pipeline();
            fetch_sprite_pipeline();
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
                NES_TRACE4("[NES_PPU] SCANLINE = 241, VBlank BEGIN");
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
                NES_TRACE4("[NES_PPU] SCANLINE = 261, VBlank END");
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
            swap_buffer();
            _frame_count++;
            NES_TRACE4("[NES_PPU] FRAME " << std::dec << _frame_count << " ------ ");

            if (_auto_stop && _frame_count > _stop_after_frame)
            {
                NES_TRACE1("[NES_PPU] FRAME exceeding " << std::dec << _stop_after_frame << " -> stopping...");
                _system->stop();
            }
        }
        NES_TRACE4("[NES_PPU] SCANLINE " << std::dec << (uint32_t) _cur_scanline << " ------ ");
    }
}