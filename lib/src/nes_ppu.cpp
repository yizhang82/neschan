#pragma once

#include "stdafx.h"

#include "nes_ppu.h"
#include "nes_system.h"
#include "nes_memory.h"

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

    _cycle = nes_cycle_t(0);
}

void nes_ppu::reset()
{
    init();
}

void nes_ppu::power_on(nes_system *system)
{
    init();

    _system = system;
}

void nes_ppu::step_to(nes_cycle_t count)
{

}