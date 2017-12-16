#pragma once

#include <vector>
#include <cassert>

using namespace std;

#define RAM_SIZE 0x10000

class nes_memory
{
public :
    nes_memory()
    {
        m_ram = nullptr;
    }

    void initialize()
    {
        m_ram = new uint8_t[RAM_SIZE];
        memset(m_ram, 0, RAM_SIZE);
    }

    ~nes_memory()
    {
        delete[] m_ram;
        m_ram = nullptr;
    }

    uint8_t get_byte(uint16_t addr)
    {
        return m_ram[addr];
    }

    uint16_t get_word(uint16_t addr)
    {
        // NES 6502 CPU is little endian
        return (uint16_t)m_ram[addr] + ((uint16_t)m_ram[addr + 1] << 8);
    }

    void set_byte(uint16_t addr, uint8_t value)
    {
        m_ram[addr] = value;
    }

    void set_bytes(uint16_t addr, uint8_t *data, size_t size)
    {
        assert(size + addr < RAM_SIZE);
        memcpy_s(m_ram + addr, RAM_SIZE, data, size);
    }

    uint8_t set_word(uint16_t addr, uint16_t value)
    {
        // NES 6502 CPU is little endian
        m_ram[addr] = value & 0xff;
        m_ram[addr + 1] = (value >> 8);
    }

private :
    uint8_t *m_ram;
};

