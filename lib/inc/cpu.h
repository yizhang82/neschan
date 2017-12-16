//=================================================================================================
// NESChan 
// Author: Yi Zhang (yizhang82@outlook.com)
//=================================================================================================

#pragma once

#include "memory.h"
#include <vector>

using namespace std;

//
// All processor status codes for the status register
// http://wiki.nesdev.com/w/index.php/CPU_status_flag_behavior
//

// CARRY
// 1 if last addition or shift resulted in a carry, or if last subtraction resulted in no borrow
#define PROCESSOR_STATUS_CARRY_MASK           0x1

// ZERO
// 1 if last operation resulted in a 0 value
#define PROCESSOR_STATUS_ZERO_MASK            0x2

// INTERRUPT
// 0 - IRQ and NMI get through
// 1 - only NMI get through
#define PROCESSOR_STATUS_INTERRUPT_MASK       0x4

// DECIMAL
// 1 to make ADC and SBC use BCD math (ignored in NES)
#define PROCESSOR_STATUS_DECIMAL_MASK         0x8

// S1/S2
// Magical B flag set by interrupts and PHP/BRK when they push flags to stack.
// bit 5 is always set to 1, and bit 4 is set to 1 if from PHP/BRK or 0 if from interrupt IRQ/NMI.
// It only exists in copy of the stack but never in the CPU register itself
#define PROCESSOR_STATUS_S1_MASK              0x10
#define PROCESSOR_STATUS_S2_MASK              0x20
#define PROCESSOR_STATUS_OVERFLOW_MASK        0x40
#define PROCESSOR_STATUS_NEGATIVE_MASK        0x80


// Addressing modes of 6502
// http://obelisk.me.uk/6502/addressing.html
// http://wiki.nesdev.com/w/index.php/CPU_addressing_modes
enum nes_addr_mode
{
    nes_addr_mode_imp,        // implicit
    nes_addr_mode_acc,        //          val = A
    nes_addr_mode_imm,        //          val = arg_8
    nes_addr_mode_rel,        //          val = arg_8, as offset
    nes_addr_mode_abs,        //          val = arg_16, LSB then MSB                   
    nes_addr_mode_zp,         //          val = PEEK(arg_8)
    nes_addr_mode_zp_ind_x,   // d, x     val = PEEK((arg_8 + X) % $FF ), 4 cycles
    nes_addr_mode_zp_ind_y,   // d, y     val = PEEK((arg_8 + Y) % $FF), 4 cycles
    nes_addr_mode_abs_x,      // a, x     val = PEEK(arg_16 + Y), 4+ cycles
    nes_addr_mode_abs_y,      // a, y     val = PEEK(arg_16 + Y), 4+ cycles
    nes_addr_mode_ind_x,      // (d, x)   val = PEEK(PEEK((arg + X) % $FF) + PEEK((arg + X + 1) % $FF) * $FF), 6 cycles
    nes_addr_mode_ind_y,      // (d), y   val = PEEK(PEEK(arg) + PEEK((arg + 1) % $FF)* $FF + Y), 5+ cycles
};

// All 6502 CPU registers - all 6 of them :)
// http://wiki.nesdev.com/w/index.php/CPU_registers
struct nes_cpu_context
{
    unsigned char A;        // Accumulator - supports using status register for carrying, overflow detection, etc
    unsigned char X;        // Index register. Used for addressing modes, loop counters
    unsigned char Y;        // Same as X
    unsigned short PC;      // Program counter. Supports 65535 direct (unbanked) memory locations. 
    unsigned char S;        // Stack pointer. Can be accessed using interrupts, pulls, pushes, and transfers.
    unsigned char P;        // Status register - used by ALU unit
};

// well-known offsets for each address mode
// for example, ADC_imm (0x69) = ADC (0x60) + imm (0x9)
// This only applies for ALU instructions
enum nes_addr_mode_offset
{
    nes_addr_mode_offset_imm = 0x9,
    nes_addr_mode_offset_zp = 0x5,
    nes_addr_mode_offset_zp_ind_x = 0x15,
    nes_addr_mode_offset_abs = 0xd,
    nes_addr_mode_offset_abs_x = 0x1d,
    nes_addr_mode_offset_abs_y = 0x19,
    nes_addr_mode_offset_ind_x = 0x1,
    nes_addr_mode_offset_ind_y = 0x11
};

enum nes_op_code
{
    ORA_base = 0x00,
    AND_base = 0x20,
    EOR_base = 0x40,
    ADC_base = 0x60,
    STA_base = 0x80,
    LDA_base = 0xA0,
    CMP_base = 0xC0,
    SBC_base = 0xE0,
    BRK = 0x00,
};


class nes_cpu
{
public :
    nes_cpu(nes_memory &mem)
        :_mem(mem)
    {
        memset(&_context, 0, sizeof(_context));
    }

    // Set status register P accordingly
    void compute_processor_status() { /* @NYI */ }

    void decode_adress_mode(nes_addr_mode mode)
    {

    }

    void load_program(vector<uint8_t> &&program, uint16_t start_addr)
    {
        // load at addr 1000 for convenience
        _mem.set_bytes(start_addr, program.data(), program.size());
    }

    void reset_context()
    {
        memset(&_context, 0, sizeof(_context));
    }

    void run(uint16_t addr)
    {
        reset_context();
        _context.PC = addr;
        execute();
    }

public :
    void set_carry_flag(bool set)
    {
        set_flag(PROCESSOR_STATUS_CARRY_MASK, set);
    }
    void set_zero_flag(bool set)
    {
        set_flag(PROCESSOR_STATUS_ZERO_MASK, set);
    }
    void set_interrupt_flag(bool set)
    {
        set_flag(PROCESSOR_STATUS_INTERRUPT_MASK, set);
    }
    void set_decimal_flag(bool set)
    {
        set_flag(PROCESSOR_STATUS_DECIMAL_MASK, set);
    }
    void set_s1_flag(bool set)
    {
        set_flag(PROCESSOR_STATUS_S1_MASK, set);
    }
    void set_s2_flag(bool set) 
    {
        set_flag(PROCESSOR_STATUS_S2_MASK, set);
    }
    void set_overflow_flag(bool set)
    {
        set_flag(PROCESSOR_STATUS_OVERFLOW_MASK, set);
    }
    void set_negative_flag(bool set)
    {
        set_flag(PROCESSOR_STATUS_NEGATIVE_MASK, set);
    }
    uint8_t get_carry()
    {
        return (_context.S & PROCESSOR_STATUS_CARRY_MASK);
    }

    uint8_t peek(uint16_t addr) { return _mem.get_byte(addr); }
    void poke(uint16_t addr, uint8_t value) { _mem.set_byte(addr, value); }
    
    uint8_t &A() { return _context.A; }
    uint8_t &X() { return _context.X; }
    uint8_t &Y() { return _context.Y; }
    uint16_t &PC() { return _context.PC; }
    uint8_t &P() { return _context.P; }
    uint8_t &S() { return _context.S; }

private :
    void execute();

    uint8_t decode_byte()
    {
        return _mem.get_byte(_context.PC++);
    }

    uint16_t decode_word()
    {
        auto word = _mem.get_word(_context.PC);
        _context.PC += 2;
        return word;
    }

    void set_flag(uint8_t mask, bool set)
    {
        if (set)
            _context.S |= mask;
        else
            _context.S &= ~mask;
    }

    void cycle(uint8_t count)
    {
        // @TODO - we may need to step until the next event
        _cycle += count;
    }

private :
    //
    // Implements all address mode
    //
    uint8_t get_operand(nes_addr_mode addr_mode)
    {
        if (addr_mode == nes_addr_mode::nes_addr_mode_acc)
        {
            return _context.X;
        }
        else if (addr_mode == nes_addr_mode::nes_addr_mode_imm)
        {
            // immediate - next byte is a constant
            return decode_byte();
        }
        else
        {
            return peek(get_operand_addr(addr_mode));
        }
    }

    uint16_t get_operand_addr(nes_addr_mode addr_mode)
    {
        if (addr_mode == nes_addr_mode::nes_addr_mode_zp)
        {
            // zero page - next byte is 8-bit address
            return decode_byte();
        }
        else if (addr_mode == nes_addr_mode::nes_addr_mode_zp_ind_x)
        {
            // zero page indexed X
            return decode_byte() + _context.X;
        }
        else if (addr_mode == nes_addr_mode::nes_addr_mode_zp_ind_y)
        {
            // zero page indexed Y 
            return decode_byte() + _context.Y;
        }
        else if (addr_mode == nes_addr_mode::nes_addr_mode_abs)
        {
            // Absolute
            return decode_word();
        }
        else if (addr_mode == nes_addr_mode::nes_addr_mode_abs_x)
        {
            // Absolute X
            return decode_word() + _context.X;
        }
        else if (addr_mode == nes_addr_mode::nes_addr_mode_abs_y)
        {
            // Absolute Y
            return decode_word() + _context.Y;
        }
        else if (addr_mode == nes_addr_mode::nes_addr_mode_ind_x)
        {
            // Indexed Indirect, rarely used
            uint8_t addr = peek(decode_byte());
            return peek(addr + _context.X) + (((uint16_t) peek(addr + _context.X + 1)) << 8);
        }
        else if (addr_mode == nes_addr_mode::nes_addr_mode_ind_y)
        {
            // Indirect Indexed
            // implies a table of table address in zero page
            uint8_t addr = decode_byte();
            return peek(addr) + (((uint16_t) peek(addr + 1)) << 8) + _context.Y;
        }
        else
        {
            assert(false);
            return -1;
        }
    }

private :

    //
    // Implements all CPU instructions here
    // http://obelisk.me.uk/6502/reference.html
    //

    void determine_alu_flag(uint8_t value)
    {
        set_zero_flag(value == 0);
        set_negative_flag(value & 0x80);
    }

    void determine_overflow_flag(uint8_t old_value, uint8_t new_value)
    {
        // sign bit change due to overflow or underflow
        set_overflow_flag((old_value & 0x80) != (new_value & 0x80));
    }

    // Add with carry
    void ADC(nes_addr_mode addr_mode)
    {
        uint8_t val = get_operand(addr_mode);
        A() = val + A() + get_carry();

        // flags
        determine_overflow_flag(val, A());
        determine_alu_flag(A());
    }

    // Logical AND
    void AND(nes_addr_mode addr_mode)
    {
        uint8_t val = get_operand(addr_mode);
        A() &= val;

        // flags
        determine_alu_flag(val);
    }

    // Compare 
    void CMP(nes_addr_mode addr_mode)
    {
        uint8_t val = get_operand(addr_mode);

        // flags
        if (A() < val)
        {
            set_negative_flag(true);
        }
        else if (A() > val)
        {
            set_carry_flag(true);
        }
        else
        {
            set_carry_flag(true);
            set_zero_flag(true);
        }
    }

    // Exclusive OR 
    void EOR(nes_addr_mode addr_mode)
    {
        uint8_t val = get_operand(addr_mode);
        A() ^= val;

        // flags
        determine_alu_flag(A());
    }

    // Logical Inclusive OR
    void ORA(nes_addr_mode addr_mode)
    {
        uint8_t val = get_operand(addr_mode);
        A() |= val;

        determine_alu_flag(A());
    }

    // Subtract with carry
    void SBC(nes_addr_mode addr_mode)
    {
        uint8_t val = get_operand(addr_mode);
        A() = A() - val - (1 - get_carry());

        // flags
        set_carry_flag(A() > val);
        determine_overflow_flag(val, A());
        determine_alu_flag(A());
    }

    // Load Accumulator
    void LDA(nes_addr_mode addr_mode)
    {
        uint8_t val = get_operand(addr_mode);
        A() = val;

        // flags
        determine_alu_flag(A());
    }

    // Store Accumulator  
    void STA(nes_addr_mode addr_mode)
    {
        poke(get_operand_addr(addr_mode), A());
        
        // Doesn't impact any flags
    }

private :
    nes_memory      &_mem;
    nes_cpu_context _context;
    uint32_t        _cycle;
};

