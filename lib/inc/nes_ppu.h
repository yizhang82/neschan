#pragma once

#include <cstdint>
#include <memory>

#include "nes_component.h"
#include "nes_cycle.h"
#include <nes_trace.h>

// PPU has its own separate 16KB memory address space
// http://wiki.nesdev.com/w/index.php/PPU_memory_map
#define PPU_VRAM_SIZE 0x4000

// OAM (Object Attribute Memory) - internal memory inside PPU for 64 sprites of 4 bytes each
// wiki.nesdev.com/w/index.php/PPU_OAM
#define PPU_OAM_SIZE 0x100

//
// All registe masks
// http://wiki.nesdev.com/w/index.php/PPU_registers
//

// Base name table address
// 0 = $2000; 1 = $2400; 2 = $2800; 3 = $2c00
#define PPUCTRL_BASE_NAME_TABLE_ADDR_MASK 0x3

// VRAM address increment per CPU read/write of PPUDATA
// 0: add 1, going across; 1: add 32, going down
#define PPUCTRL_VRAM_ADDR_MASK 0x4

// Sprite pattern table address for 8x8 sprites
// 0: $0000; 1: $1000; Ignored for 8x16 mode
#define PPUCTRL_SPRITE_PATTERN_TABLE_ADDR_MASK 0x8

// Background table pattern address
// 0: $0000; 1: $1000
#define PPUCTRL_BACKGROUND_PATTERN_TABLE_ADDRESS_MASK    0x10

// Sprite size
// 0: 8x8; 1: 8x16
#define PPUCTRL_SPRITE_SIZE_MASK     0x20

// PPU master/slave select
// 0: read backdrop from EXT pins; 1: output color on EXT pins
#define PPUCTRL_PPU_MASTER_SLAVE_SELECT 0x40

// Generate a NMI at start of vertical blanking interval
// 0: off; 1: on
#define PPUCTRL_NMI_AT_VBLANK_MASK 0x80

// 0: normal color; 1: grayscale
#define PPUMASK_GRAYSCALE 0x1

#define PPUMASK_BACKGROUND_IN_LEFTMOST_8 0x2
#define PPUMASK_SPRITE_IN_LEFTMOST_8 0x4

#define PPUMASK_SHOW_BACKGROUND 0x8
#define PPUMASK_SHOW_SPRITES 0x10

#define PPUMASK_EMPHASIZE_RED 0x20
#define PPUMASK_EMPHASIZE_GREEN 0x40
#define PPUMASK_EMPHASIZE_BLUE 0x80

// Previously written to a PPU register (due to not being updated for this address)
#define PPUSTATUS_LATCH_MASK 0x1f

// 0: no more than 8 sprites appear on a scanline
// 1: otherwise. however, there are bugs in hardware implementation that it can be 
// set incorrectly for false positive and false negatives
// set during sprite evaluation and clear at dot 1 of pre-render line
#define PPUSTATUS_SPRITE_OVERFLOW 0x20

// Set when a nonzero pixel of sprite 0 overlaps a nonzero background pixel
// Cleared at dot 1 (second dot) of the pre-render line
#define PPUSTATUS_SPRITE_0_HIT 0x40

// 0: not in vblank; 1: in vblank
// Set at dot 1 of line 241 (the line *after* the post-render line)
// Clear after reading $2002 and at dot 1 of the pre-render line
#define PPUSTATUS_VBLANK_START 0x80

#define PPU_SCREEN_X 256
#define PPU_SCREEN_Y 240

#define PPU_SCANLINE_COUNT 262

// Only max of 8 sprites can be drawn in one scanlinekkkkk
#define PPU_ACTIVE_SPRITE_MAX 0x8
#define PPU_SPRITE_MAX 64 

#define PPU_SPRITE_ATTR_BIT32_MASK 0x3
#define PPU_SPRITE_ATTR_BEHIND_BG 0x20
#define PPU_SPRITE_ATTR_HORIZONTAL_FLIP 0x40
#define PPU_SPRITE_ATTR_VERTICAL_FLIP 0x80

class nes_system;
class nes_mapper;

using namespace std;

enum nes_ppu_state
{
    nes_ppu_state_power_on,     // initial
    nes_ppu_state_vbl_1,        // 1st VBL flag set
    nes_ppu_state_vbl_2,        // 2nd VBL flag set
    nes_ppu_state_ready,        // ready to render
    nes_ppu_state_render,       // rendering
};

// Protect internal register state to avoid destructive reads in log code
// For example, we don't want to clear v-blank in our logging code when reads from PPUSTATUS
class nes_ppu_protect
{
public :
    nes_ppu_protect(nes_ppu *ppu);
    ~nes_ppu_protect();

private :
    nes_ppu *_ppu;
};

class nes_ppu : public nes_component
{
public :
    nes_ppu() 
    {
        _vram = make_unique<uint8_t[]>(PPU_VRAM_SIZE);
        _oam = make_unique<uint8_t[]>(PPU_OAM_SIZE);
    }
    
    ~nes_ppu();

public :
    //
    // nes_component overrides
    //
    virtual void power_on(nes_system *system);

    virtual void reset();

    virtual void step_to(nes_cycle_t count);

public :
    void init();

    void step_ppu(nes_ppu_cycle_t cycle);
    void fetch_tile();
    void fetch_tile_pipeline();
    void fetch_sprite_pipeline();
    void fetch_sprite(uint8_t sprite_id);

    bool is_ready() { return _master_cycle > nes_ppu_cycle_t(29658); }

    void stop_after_frame(uint32_t frame) 
    {
        _auto_stop = true;
        _stop_after_frame = frame; 
    }

    bool is_render_off() { return !_show_bg && !_show_sprites; }

    void load_mapper(shared_ptr<nes_mapper> &mapper);

    uint8_t *frame_buffer()
    {
        // Return the completed buffer
        if (_frame_buffer == _frame_buffer_1)
            return _frame_buffer_2;
        else
            return _frame_buffer_1;
    }

    void swap_buffer()
    {
        if (_frame_buffer == _frame_buffer_1)
            _frame_buffer = _frame_buffer_2;
        else
            _frame_buffer = _frame_buffer_1;
    }

public :

    //
    // PPU internal ram
    //
    uint8_t read_byte(uint16_t addr)
    {
        redirect_addr(addr);

        if (addr >= PPU_VRAM_SIZE)
            return 0xff;

        return _vram[addr];
    }

    void write_byte(uint16_t addr, uint8_t val)
    {
        redirect_addr(addr);
        
        if (addr >= PPU_VRAM_SIZE)
            return;

        _vram[addr] = val;
    }

    void write_bytes(uint16_t addr, uint8_t *src, size_t src_size)
    {
        redirect_addr(addr);
        memcpy_s(_vram.get() + addr, PPU_VRAM_SIZE - addr, src, src_size);
    }

    void redirect_addr(uint16_t &addr)
    {
        if ((addr & 0xff00) == 0x3f00)
        {
            // mirror of palette table every 0x20 bytes
            addr &= 0xff1f;

            // mirror special case 0x3f10 = 0x3f00, 0x3f14 = 0x3f04, ...
            if ((addr & 0xfff3) == 0x3f10)
                addr &= 0x3f0f;
        }
        else if ((addr & 0xf000) == 0x2000)
        {
            // name table mirroring
            if (_vertical_mirroring)
                addr &= 0xf7ff;     // $2000=$2800, $2400=$2c00
            else
                addr &= 0xfbff;     // $2000=$2400, $2800=$2c00
        } 
        else if (addr >= 0x3000)
        {
            // 0x3000~0x3f00 mirrors to 0x2000~0x2f00
            addr -= 0x1000;
        }
    }

    // Avoid destructive reads for PPU registers
    // Useful in logging code
    // See nes_ppu_protect
    void set_protect(bool set)
    {
        _protect_register = set;
    }

    //
    // All registers
    //
    void write_latch(uint8_t val)
    {
        // Don't update latch in "protected reads"  
        if (_protect_register) return;

        _latch = val;
    }

    uint8_t read_latch()
    {
        // This latch is also subject to decay but it is random so no need to emulate that
        return _latch;
    }

    void write_PPUCTRL(uint8_t val)
    {   
        // Ignore write before PPU is ready
        if (!is_ready()) return;

        write_latch(val);

        uint8_t name_table_addr_bit = val & PPUCTRL_BASE_NAME_TABLE_ADDR_MASK;
        _temp_ppu_addr = (_temp_ppu_addr & 0xf3ff) | ((val & PPUCTRL_BASE_NAME_TABLE_ADDR_MASK) << 10);
        _name_tbl_addr = 0x2000 + uint16_t(name_table_addr_bit) * 0x400;

        _bg_pattern_tbl_addr = (val & PPUCTRL_BACKGROUND_PATTERN_TABLE_ADDRESS_MASK) << 0x8;
        _sprite_pattern_tbl_addr = (val & PPUCTRL_SPRITE_PATTERN_TABLE_ADDR_MASK) << 0x8;

        _use_8x16_sprite = val & PPUCTRL_SPRITE_SIZE_MASK;

        _ppu_addr_inc = (val & PPUCTRL_VRAM_ADDR_MASK) ? 0x20 : 1;

        _vblank_nmi = (val & PPUCTRL_NMI_AT_VBLANK_MASK);
    }

    void write_PPUMASK(uint8_t val)
    {
        // Ignore write before PPU is ready
        if (!is_ready()) return;

        write_latch(val);

        _show_bg = val & PPUMASK_SHOW_BACKGROUND;
        _show_sprites = val & PPUMASK_SHOW_SPRITES;
        _gray_scale_mode = val & PPUMASK_GRAYSCALE;
    }

    uint8_t read_PPUSTATUS()
    {
        uint8_t status = _latch & PPUSTATUS_LATCH_MASK;
        if (_sprite_0_hit)
            status |= PPUSTATUS_SPRITE_0_HIT;
        if (_sprite_overflow)
            status |= PPUSTATUS_SPRITE_OVERFLOW;
        if (_vblank_started)
            status |= PPUSTATUS_VBLANK_START;

        // Don't clear flags
        if (!_protect_register)
        {
            // clear various flags after reading
            _vblank_started = false;
            _addr_toggle = false;
        }

        write_latch(status);
        return status;
    }

    void write_OAMADDR(uint8_t val)
    {
        write_latch(val);

        _oam_addr = val;
    }

    void write_OAMDATA(uint8_t val)
    {
        write_latch(val);
        if (_oam_addr >= PPU_OAM_SIZE)
            return;

        _oam[_oam_addr] = val;
        _oam_addr++;
    }

    uint8_t read_OAMDATA()
    {
        // @TODO - This exposes internal OAM access during rendering
        if (_mask_oam_read)
            return 0xff;

        if (_oam_addr >= PPU_OAM_SIZE)
            return 0xff;

        uint8_t val = _oam[_oam_addr];
        write_latch(val);
        return val;
    }

    void write_PPUSCROLL(uint8_t val)
    {
        NES_TRACE4("[NES_PPU] write_PPUSCROLL(Val=" << std::hex << (uint32_t)val << ")");

        // Ignore write before PPU is ready
        if (!is_ready()) return;

        write_latch(val);

        _addr_toggle = !_addr_toggle;
        if (_addr_toggle)
        {
            // first write
            _temp_ppu_addr = (_temp_ppu_addr & 0xffe0) | (val >> 3);
            _fine_x_scroll = val & 0x7;
            _scroll_x = val;
        }
        else
        {
            // second write
            _temp_ppu_addr = (_temp_ppu_addr & 0xc1f) | (uint16_t(val & 0xf8) << 2) | (uint16_t(val & 0x7) << 12);
            _scroll_y = val;
        }
    }

    void write_PPUADDR(uint8_t val)
    {
        NES_TRACE4("[NES_PPU] write_PPUADDR(Val=" << std::hex << (uint32_t)val << ")");
        write_latch(val);

        _addr_toggle = !_addr_toggle;
        if (_addr_toggle)
        {
            // first write
            // note that both PPUADDR(2006) and PPUSCROLL (2005) share the same _temp_ppu_addr
            _temp_ppu_addr = (_temp_ppu_addr & 0x00ff) | (uint16_t(val & 0x3f) << 8);
        }
        else
        {
            // second write
            // note that both PPUADDR(2006) and PPUSCROLL (2005) share the same _temp_ppu_addr
            _temp_ppu_addr = (_temp_ppu_addr & 0xff00) | val;
            _ppu_addr = _temp_ppu_addr;
        }
    }

    void write_PPUDATA(uint8_t val)
    {
        NES_TRACE4("[NES_PPU] write_PPUDATA(Addr=" << std::hex << _ppu_addr << ", Val=" << (uint32_t)val << ")");
        write_latch(val);
        write_byte(_ppu_addr, val);
        _ppu_addr += _ppu_addr_inc;
    }

    uint8_t read_PPUDATA()
    {
        // use _vram_read_buf to implement VRAM delay reading buffer behavior
        // First time read will read from buffer and then update the buffer
        // This means all the reads are delayed by 1 read
        uint8_t val = _vram_read_buf;
        uint8_t new_val = read_byte(_ppu_addr);

        bool is_palette = ((_ppu_addr & 0xff00) == 0x3f00);
        if (!_protect_register)
        {
            // for palette - the read buf is updated with the mirrored nametable address
            if (is_palette)
                _vram_read_buf = read_byte(_ppu_addr - 0x1000);
            else
                _vram_read_buf = new_val;
            _ppu_addr += _ppu_addr_inc;
        }

        write_latch(val);

        // palette reads returns the correct data immediately
        if (is_palette)
            return new_val;

        return val;
    }

    void write_OAMDMA(uint8_t val);

    void oam_dma(uint16_t addr);

private :
    struct sprite_info
    {
        uint8_t pos_y;            // off by 1
        uint8_t tile_index;
        uint8_t attr;
        uint8_t pos_x;
    };

    sprite_info *get_sprite(uint8_t sprite_id)
    {
        assert(sprite_id < PPU_SPRITE_MAX);

        // sprite info resides in OAM memory and there are 64 sprites x 4 bytes each = 256 bytes
        return &((sprite_info *)_oam.get())[sprite_id];
    }

    uint8_t get_palette_color(bool is_background, uint8_t palette_index_4_bit)
    {
        // There is only one universal backdrop color doesn't matter which background it is
        if ((palette_index_4_bit & 0x3) == 0)
            return read_byte(0x3f00);

        uint16_t palette_addr = (is_background ? 0x3f00 : 0x3f10) | palette_index_4_bit;
        return read_byte(palette_addr);
    }

    uint8_t read_pattern_table_column(bool sprite, uint8_t tile_index, uint8_t bitplane, uint8_t tile_row_index)
    {
        uint16_t tile_addr = sprite ? _sprite_pattern_tbl_addr : _bg_pattern_tbl_addr;
        tile_addr |= (tile_index << 4);

        return read_byte(tile_addr | (bitplane << 3) | tile_row_index);
    }
   
private :
    nes_system *_system;

    unique_ptr<uint8_t[]> _vram;
    unique_ptr<uint8_t[]> _oam;

    // PPUCTRL data
    uint16_t _name_tbl_addr;
    uint16_t _bg_pattern_tbl_addr;
    uint16_t _sprite_pattern_tbl_addr;
    uint16_t _ppu_addr_inc;
    bool _vblank_nmi;
    bool _use_8x16_sprite;

    // PPUMASK
    bool _show_bg;
    bool _show_sprites;
    bool _gray_scale_mode;

    // PPUSTATUS
    uint8_t _latch;
    bool _sprite_overflow;
    bool _vblank_started;
    bool _sprite_0_hit;

    // OAMADDR, OAMDATA
    uint8_t _oam_addr;

    // PPUSCROLL
    bool _addr_toggle;                  // the "w" register - see http://wiki.nesdev.com/w/index.php/PPU_scrolling
    uint8_t _scroll_x;
    uint8_t _scroll_y;

    // PPUADDR
    // yyy NN YYYYY XXXXX
    //    ||| || ||||| +++++-- coarse X scroll
    //    ||| || +++++-------- coarse Y scroll
    //    ||| ++-------------- nametable select
    //    +++----------------- fine Y scroll
    uint16_t _ppu_addr;                 // the "v" register - see http://wiki.nesdev.com/w/index.php/PPU_scrolling
    uint16_t _temp_ppu_addr;            // the "t" register - see http://wiki.nesdev.com/w/index.php/PPU_scrolling
    uint8_t  _fine_x_scroll;            // the "x" register - see http://wiki.nesdev.com/w/index.php/PPU_scrolling 

    // PPUDATA
    uint8_t _vram_read_buf;             // delayed VRAM reads

    nes_cycle_t _master_cycle;
    nes_ppu_cycle_t _scanline_cycle;
    int _cur_scanline;
    uint32_t _frame_count;

    bool _protect_register;             // protect PPU register from destructive reads temporarily
    uint32_t _stop_after_frame;              // stop after X frames - useful for testing
    int _auto_stop;                     // stop after X frames - useful for testing

    // rendering states
    uint8_t _tile_index;                // tile index from name table - it consists of 
    uint8_t _tile_palette_bit32;        // palette index bit 3/2 from attribute table
    uint8_t _bitplane0;                 // bitplane0 of current tile from pattern table
    uint8_t *_frame_buffer;             // entire frame buffer - only 4 bit is used
    uint8_t _frame_buffer_1[PPU_SCREEN_Y * PPU_SCREEN_X];   // entire frame buffer - only 4 bit is used
    uint8_t _frame_buffer_2[PPU_SCREEN_Y * PPU_SCREEN_X];   // entire frame buffer - only 4 bit is used
    uint8_t _pixel_cycle[8];            // pixels in each cycle
    uint8_t _shift_reg;                 // which bit do we care about

    // sprite rendering
    sprite_info _sprite_buf[PPU_ACTIVE_SPRITE_MAX];    // max 8 sprites
    uint8_t _last_sprite_id;            // current max sprite ID
    bool _has_sprite_0;                 // first active sprite is sprite 0 - needed in sprite 0 hit detection
    bool _mask_oam_read;                // OAM read is masked at certain sprite evaluation stage to always return FF
    uint8_t _sprite_pos_y;              // last sprite Y read

    shared_ptr<nes_mapper> _mapper;

    bool _vertical_mirroring;
};