#pragma once

#include "stdafx.h"

#include "nes_ppu.h"
#include "nes_system.h"
#include "nes_memory.h"

void nes_ppu::oam_dma(uint16_t addr)
{
    _system->ram()->get_bytes(_oam.get(), PPU_OAM_SIZE, addr, PPU_OAM_SIZE);
}