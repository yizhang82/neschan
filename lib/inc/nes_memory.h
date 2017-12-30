#pragma once

#include <vector>
#include <cassert>
#include <memory>

#include <nes_component.h>
#include <nes_mapper.h>

using namespace std;

#define RAM_SIZE 0x10000

class nes_mapper;
class nes_ppu;

class nes_memory : public nes_component
{
public :
    nes_memory()
    {
        _ram = make_unique<uint8_t []>(RAM_SIZE);
    }

    bool is_io_reg(uint16_t addr)
    {
        // $2000~$2007
        if ((addr & 0xfff8) == 0x2000)
            return true;
        // $4000~401f
        if ((addr & 0xffe0) == 0x4000)
            return true;
        return false;
    }

    uint8_t read_io_reg(uint16_t addr);
    void write_io_reg(uint16_t addr, uint8_t val);

    uint8_t get_byte(uint16_t addr)
    {
        redirect_addr(addr);
        if (is_io_reg(addr))
            return read_io_reg(addr);

        return _ram[addr];
    }

    uint16_t get_word(uint16_t addr)
    {
        // NES 6502 CPU is little endian
        return get_byte(addr) + (uint16_t(get_byte(addr + 1)) << 8);
    }

    void set_byte(uint16_t addr, uint8_t val);

    void set_bytes(uint16_t addr, uint8_t *data, size_t size)
    {
        assert(size + addr <= RAM_SIZE);
        redirect_addr(addr);
        memcpy_s(&_ram[0] + addr, RAM_SIZE - addr, data, size);
    }

    void get_bytes(uint8_t *dest, uint16_t dest_size, uint16_t src_addr, size_t src_size)
    {
        assert(src_addr + src_size <= RAM_SIZE);
        redirect_addr(src_addr);
        memcpy_s(dest, dest_size, &_ram[0] + src_addr, src_size);
    }

    void set_word(uint16_t addr, uint16_t value)
    {
        // NES 6502 CPU is little endian
        set_byte(addr, value & 0xff);
        set_byte(addr + 1, (value >> 8));
    }

    void redirect_addr(uint16_t &addr)
    {
        if ((addr & 0xE000) == 0)
        {
            // map 0x0000~0x07ff 4 times until 0x1fff
            addr &= 0x7ff;
        }
        else if ((addr & 0xE000) == 0x2000)
        {
            // map 0x2000~0x2008 every 8 bytes until 0x3fff
            addr &= 0x2007;
        }
    }

    void load_mapper(shared_ptr<nes_mapper> &mapper);

    nes_mapper& get_mapper() { return *_mapper; }

public :
    //
    // nes_component overrides
    //
    virtual void power_on(nes_system *system);
    virtual void reset()
    {
        // Do nothing
    }

    virtual void step_to(nes_cycle_t count)
    {
        // Do nothing
    }

private :
    unique_ptr<uint8_t[]> _ram;
    shared_ptr<nes_mapper> _mapper;

    nes_system *_system;
    nes_ppu *_ppu;
    nes_input *_input;

    nes_mapper_info _mapper_info;
};

