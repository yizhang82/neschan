#pragma once

#include <cstdint>
#include "nes_memory.h"
#include "nes_ppu.h"
#include "nes_trace.h"
#include <memory>
#include <fstream>

using namespace std;

enum nes_mapper_flags
{
    nes_mapper_flags_none,
    // This mapper can dynamically switch memory
    nes_mapper_flags_dynamic_switching = 0x1,

    // Uses vertical mirroring
    nes_mapper_flags_vertical_mirroring = 0x2
};

class nes_mapper
{
public :
    //
    // PRG ROM base address. execution of code starts here
    //
    virtual uint16_t get_code_addr() = 0;

    //
    // Called when mapper is loaded into memory
    // Useful when all you need is a one-time memcpy
    //
    virtual void on_load_ram(nes_memory &mem) = 0;

    //
    // Called when mapper is loaded into PPU
    // Useful when all you need is a one-time memcpy
    //
    virtual void on_load_ppu(nes_ppu &ppu) = 0;

    //
    // Returns various mapper related flags
    //
    virtual nes_mapper_flags get_flags() = 0;

    //
    // Return the mapped address given the target address
    // nullptr if not mapped
    // This is only needed if nes_mapper_flags_dynamic_switching is set 
    //
    virtual uint8_t *redirect_addr(uint16_t addr) = 0;
};

class nes_mapper_1 : public nes_mapper
{
public :
    nes_mapper_1(shared_ptr<vector<uint8_t>> &prg_rom, shared_ptr<vector<uint8_t>> &chr_rom, bool vertical_mirroring)
        :_prg_rom(prg_rom), _chr_rom(chr_rom), _vertical_mirroring(vertical_mirroring)
    {

    }

    //
    // PRG ROM base address. execution of code starts here
    //
    virtual uint16_t get_code_addr()
    {        
        if (_prg_rom->size() == 0x4000)
            return 0xc000;
        else
            return 0x8000;
    }

    //
    // Called when mapper is loaded into memory
    // Useful when all you need is a one-time memcpy
    //
    virtual void on_load_ram(nes_memory &mem)
    {
        // memcpy
        mem.set_bytes(0x8000, _prg_rom->data(), _prg_rom->size());

        if (_prg_rom->size() == 0x4000)
        {
            // "map" 0xC000 to 0x8000
            mem.set_bytes(0xc000, _prg_rom->data(), _prg_rom->size());
        }
    }


    //
    // Called when mapper is loaded into PPU
    // Useful when all you need is a one-time memcpy
    //
    virtual void on_load_ppu(nes_ppu &ppu)
    {
        // memcpy
        ppu.write_bytes(0x0000, _chr_rom->data(), _chr_rom->size());
    }

    //
    // Returns various mapper related flags
    //
    virtual nes_mapper_flags get_flags()
    {
        if (_vertical_mirroring)
            return nes_mapper_flags_vertical_mirroring;
        return nes_mapper_flags_none;
    }

    //
    // Return the mapped address given the target address
    // nullptr if not mapped
    // This is only needed if nes_mapper_flags_dynamic_switching is set 
    //
    virtual uint8_t *redirect_addr(uint16_t addr) { return nullptr; }

private :
    shared_ptr<vector<uint8_t>> _prg_rom;
    shared_ptr<vector<uint8_t>> _chr_rom;
    bool _vertical_mirroring;
};

#define FLAG_6_USE_VERTICAL_MIRRORING_MASK 0x1
#define FLAG_6_HAS_BATTERY_BACKED_PRG_RAM_MASK 0x2
#define FLAG_6_HAS_TRAINER_MASK  0x4
#define FLAG_6_USE_FOUR_SCREEN_VRAM_MASK 0x8
#define FLAG_6_LO_MAPPER_NUMBER_MASK 0xf0
#define FLAG_7_HI_MAPPER_NUMBER_MASK 0xf0

class nes_rom_loader
{
public :
    struct ines_header
    {
        uint8_t magic[4];       // 0x4E, 0x45, 0x53, 0x1A
        uint8_t prg_size;       // PRG ROM in 16K
        uint8_t chr_size;       // CHR ROM in 8K, 0 -> using CHR RAM
        uint8_t flag6;
        uint8_t flag7;
        uint8_t prg_ram_size;   // PRG RAM in 8K
        uint8_t flag9;
        uint8_t flag10;         // unofficial
        uint8_t reserved[5];    // reserved
    };

    // Loads a NES ROM file
    // Automatically detects format according to extension and header
    // Returns a nes_mapper instance which has all necessary memory mapped
    static shared_ptr<nes_mapper> load_from(const char *path)

    {
        NES_TRACE1("[NES_ROM] Opening NES ROM file '" << path << "' ...");

        assert(sizeof(ines_header) == 0x10);

        ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        file.open(path, std::ifstream::in | std::ifstream::binary);
        
        // Parse header
        ines_header header;
        file.read((char *)&header, sizeof(header));

        if (header.flag6 & FLAG_6_HAS_TRAINER_MASK)
        {
            NES_TRACE1("[NES_ROM] HEADER: Trainer bytes 0x200 present.");
            NES_TRACE1("[NES_ROM] Skipping trainer bytes...");

            // skip the 512-byte trainer
            file.seekg(ios_base::cur, 0x200);
        }

        NES_TRACE1("[NES_ROM] HEADER: Flags6 = 0x" << std::hex << (uint32_t) header.flag6);
        bool vertical_mirroring = header.flag6 & FLAG_6_USE_VERTICAL_MIRRORING_MASK;
        if (vertical_mirroring)
        {
            NES_TRACE1("    Mirroring: Vertical");
        }
        else
        {
            NES_TRACE1("    Mirroring: Horizontal");
        }

        NES_TRACE1("[NES_ROM] HEADER: Flags7 = 0x" << std::hex << (uint32_t) header.flag7);
        int mapper_id = ((header.flag6 & FLAG_6_LO_MAPPER_NUMBER_MASK) >> 4) + ((header.flag7 & FLAG_7_HI_MAPPER_NUMBER_MASK));
        NES_TRACE1("[NES_ROM] HEADER: Mapper_ID = " << std::dec << mapper_id);
        
        int prg_rom_size = header.prg_size * 0x4000;    // 16KB 
        int chr_rom_size = header.chr_size * 0x2000;    // 8KB

        NES_TRACE1("[NES_ROM] HEADER: PRG ROM Size = 0x" << std::hex << (uint32_t) prg_rom_size);
        NES_TRACE1("[NES_ROM] HEADER: CHR_ROM Size = 0x" << std::hex << (uint32_t) chr_rom_size);

        auto prg_rom = make_shared<vector<uint8_t>>(prg_rom_size);
        auto chr_rom = make_shared<vector<uint8_t>>(chr_rom_size);

        file.read((char *)prg_rom->data(), prg_rom->size());
        file.read((char *)chr_rom->data(), chr_rom->size());

        assert(mapper_id == 0);
        shared_ptr<nes_mapper> mapper = make_shared<nes_mapper_1>(prg_rom, chr_rom, vertical_mirroring);

        file.close();

        return mapper;
    }
};

