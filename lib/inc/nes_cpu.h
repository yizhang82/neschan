//=================================================================================================
// NESChan 
// Author: Yi Zhang (yizhang82@outlook.com)
//=================================================================================================

#pragma once

#include "nes_memory.h"
#include "nes_mapper.h"
#include "nes_component.h"
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

// Magical B flag set by interrupts and PHP/BRK when they push flags to stack.
// bit 5 is always set to 1, and bit 4 is set to 1 if from PHP/BRK or 0 if from interrupt IRQ/NMI.
// It only exists in copy of the stack but never in the CPU register itself
#define PROCESSOR_STATUS_B_MASK              0x10
#define PROCESSOR_STATUS_I_MASK              0x20
#define PROCESSOR_STATUS_OVERFLOW_MASK        0x40
#define PROCESSOR_STATUS_NEGATIVE_MASK        0x80

// Vertical blanking interrupt handler
#define NMI_HANDLER     0xfffa

// Reset handler - the "main" for NES roms
#define RESET_HANDLER   0xfffc

#define IRQ_HANDLER     0xfffe

// Addressing modes of 6502
// http://obelisk.me.uk/6502/addressing.html
// http://wiki.nesdev.com/w/index.php/CPU_addressing_modes
enum nes_addr_mode
{
    nes_addr_mode_imp,        // implicit
    nes_addr_mode_acc,        //          val = A
    nes_addr_mode_imm,        //          val = arg_8
    nes_addr_mode_ind_jmp,    //          val = peek16(arg_16), with JMP bug
    nes_addr_mode_rel,        //          val = arg_8, as offset
    nes_addr_mode_abs,        //          val = PEEK(arg_16), LSB then MSB                   
    nes_addr_mode_abs_jmp,    //          val = arg_16, LSB then MSB, direct jump address                  
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
};

class nes_cpu : public nes_component
{
public :
    nes_cpu() 
    {
        _system = nullptr;
        _mem = nullptr;
    }

public :
    //
    // nes_component overrides
    //
    virtual void power_on(nes_system *system);

    virtual void reset();
    virtual void step_to(nes_cycle_t count);

public :

    void stop_at_infinite_loop() { _stop_at_infinite_loop = true; }
    void stop_at_addr(uint16_t addr) { _is_stop_at_addr = true;  _stop_at_addr = addr; }

    void set_carry_flag(bool set) { set_flag(PROCESSOR_STATUS_CARRY_MASK, set); }
    uint8_t get_carry() { return (_context.P & PROCESSOR_STATUS_CARRY_MASK); }

    void set_zero_flag(bool set) { set_flag(PROCESSOR_STATUS_ZERO_MASK, set); }
    bool is_zero() { return _context.P & PROCESSOR_STATUS_ZERO_MASK; }

    void set_interrupt_flag(bool set) { set_flag(PROCESSOR_STATUS_INTERRUPT_MASK, set); }
    bool is_interrupt() { return _context.P & PROCESSOR_STATUS_INTERRUPT_MASK; }

    void set_decimal_flag(bool set) { set_flag(PROCESSOR_STATUS_DECIMAL_MASK, set); }

    void set_I_flag(bool set) { set_flag(PROCESSOR_STATUS_I_MASK, set); }
    void set_B_flag(bool set) { set_flag(PROCESSOR_STATUS_B_MASK, set); }

    void set_overflow_flag(bool set) { set_flag(PROCESSOR_STATUS_OVERFLOW_MASK, set); }
    bool is_overflow() { return _context.P & PROCESSOR_STATUS_OVERFLOW_MASK; }

    void set_negative_flag(bool set) { set_flag(PROCESSOR_STATUS_NEGATIVE_MASK, set); }
    bool is_negative() { return _context.P & PROCESSOR_STATUS_NEGATIVE_MASK; }

    uint8_t peek(uint16_t addr) { return _mem->get_byte(addr); }
    uint16_t peek_word(uint16_t addr) { return _mem->get_word(addr); }
    void poke(uint16_t addr, uint8_t value);
    
    uint8_t &A() { return _context.A; }
    uint8_t &X() { return _context.X; }
    uint8_t &Y() { return _context.Y; }
    uint16_t &PC() { return _context.PC; }
    uint8_t &P() { return _context.P; }
    uint8_t &S() { return _context.S; }

    void request_nmi() { _nmi_pending = true; };
    void request_dma(uint16_t addr) { _dma_pending = true; _dma_addr = addr; }

public :
    //
    // Stack operations
    //
    #define STACK_OFFSET 0x100

    void push_byte(uint8_t val)
    {
        // stack grow top->down
        // no underflow/overflow detection
        _mem->set_byte(_context.S + STACK_OFFSET, val);
        _context.S--;
    }

    void push_word(uint16_t val)
    {
        // high-order bytes push first since the stack grow top->down and the machine is little-endian
        push_byte(val >> 8);
        push_byte(val & 0xff);
    }

    int8_t pop_byte()
    {
        // stack grow top->down
        // no underflow/overflow detection        
        _context.S++;
        return _mem->get_byte(_context.S + STACK_OFFSET);
    }

    int16_t pop_word()    
    {
        // low-order bytes pop first since the stack grow top->down and the machine is little-endian
        uint8_t lo = pop_byte();
        uint8_t hi = pop_byte();
        return uint16_t(hi << 8) + lo;
    }

private :
    // execute on instruction, update processor status as needed, and move CPU internal cycle count
    void exec_one_instruction();
    void NMI();
    void OAMDMA();

    uint8_t decode_byte()
    {
        return _mem->get_byte(_context.PC++);
    }

    uint16_t decode_word()
    {
        auto word = _mem->get_word(_context.PC);
        _context.PC += 2;
        return word;
    }

    void set_flag(uint8_t mask, bool set)
    {
        if (set)
            _context.P |= mask;
        else
            _context.P &= ~mask;
    }

private :
    enum operand_kind : uint8_t
    {
        operand_kind_acc,
        operand_kind_imm,
        operand_kind_addr
    };

    struct operand_t
    {
        uint16_t addr_or_value;
        operand_kind kind;
        bool is_page_crossing : 1;
    };

    void step_cpu(nes_cpu_cycle_t cycle);
    void step_cpu(int64_t cycle);

    nes_cpu_cycle_t get_cpu_cycle(operand_t operand, nes_addr_mode mode);
    nes_cpu_cycle_t get_branch_cycle(bool cond, uint16_t new_addr, int8_t rel);
    nes_cpu_cycle_t get_shift_cycle(nes_addr_mode addr_mode);

    //
    // Implements all address mode
    //
    operand_t decode_operand(nes_addr_mode addr_mode)
    {    
        if (addr_mode == nes_addr_mode::nes_addr_mode_acc)
        {
            return { 0, operand_kind_acc, false };
        }
        else if (addr_mode == nes_addr_mode::nes_addr_mode_imm)
        {
            // immediate - next byte is a constant
            return { decode_byte(), operand_kind_imm, false };
        }
        else
        {
            bool page_crossing;
            uint16_t addr = decode_operand_addr(addr_mode, &page_crossing);
            return { addr, operand_kind_addr, page_crossing };
        }
    }

    uint8_t read_operand(operand_t op)
    {
        switch (op.kind)
        {
        case operand_kind_acc:
            return A();
        case operand_kind_imm:
            // constants are always 8-bit
            return (uint8_t)op.addr_or_value;
        case operand_kind_addr:
            return peek(op.addr_or_value);
        default:
            assert(false);
            return -1;
        }
    }

    void write_operand(operand_t op, int8_t value)
    {
        switch (op.kind)
        {
        case operand_kind_acc:
            A() = value;
            break;
        case operand_kind_addr:
            poke(op.addr_or_value, value);
            break;
        default:
            assert(false);
        }
    }

    uint16_t decode_operand_addr(nes_addr_mode addr_mode, bool *page_crossing = nullptr)
    {
        if (page_crossing)
            *page_crossing = false;
        if (addr_mode == nes_addr_mode::nes_addr_mode_zp)
        {
            // zero page - next byte is 8-bit address
            return decode_byte();
        }
        else if (addr_mode == nes_addr_mode::nes_addr_mode_zp_ind_x)
        {
            // zero page indexed X
            return (decode_byte() + _context.X) & 0xff;
        }
        else if (addr_mode == nes_addr_mode::nes_addr_mode_zp_ind_y)
        {
            // zero page indexed Y 
            return (decode_byte() + _context.Y) & 0xff;
        }
        else if (addr_mode == nes_addr_mode::nes_addr_mode_ind_jmp)
        {
            // Indirect
            uint16_t addr = decode_word();
            if ((addr & 0xff) == 0xff)
            {
                // Account for JMP hardware bug
                // http://wiki.nesdev.com/w/index.php/Errata
                return peek(addr) + (uint16_t(peek(addr & 0xff00)) << 8);
            }
            else
            {
                return peek_word(addr);
            }
        }
        else if (addr_mode == nes_addr_mode::nes_addr_mode_abs || addr_mode == nes_addr_mode::nes_addr_mode_abs_jmp)
        {
            // Absolute
            return decode_word();
        }
        else if (addr_mode == nes_addr_mode::nes_addr_mode_abs_x)
        {
            // Absolute X
            uint16_t addr = decode_word();
            uint16_t new_addr = addr + _context.X;
            if (page_crossing)
                *page_crossing = ((addr & 0xff00) != (new_addr & 0xff00));
            return new_addr;
        }
        else if (addr_mode == nes_addr_mode::nes_addr_mode_abs_y)
        {
            // Absolute Y
            uint16_t addr = decode_word();
            uint16_t new_addr = addr + _context.Y;
            if (page_crossing)
                *page_crossing = ((addr & 0xff00) != (new_addr & 0xff00));
            return new_addr;
        }
        else if (addr_mode == nes_addr_mode::nes_addr_mode_ind_x)
        {
            // Indexed Indirect, rarely used
            uint8_t addr = decode_byte();
            return peek((addr + _context.X) & 0xff) + (uint16_t(peek((addr + _context.X + 1) & 0xff)) << 8);
        }
        else if (addr_mode == nes_addr_mode::nes_addr_mode_ind_y)
        {
            // Indirect Indexed
            // implies a table of table address in zero page
            uint8_t arg_addr = decode_byte();
            uint16_t addr = peek(arg_addr) + (uint16_t(peek((arg_addr + 1) & 0xff)) << 8);
            uint16_t new_addr = addr + _context.Y;
            if (page_crossing)
                *page_crossing = ((addr & 0xff00) != (new_addr & 0xff00));
            return new_addr;
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

    void calc_alu_flag(uint8_t value)
    {
        set_zero_flag(value == 0);
        set_negative_flag(value & 0x80);
    }

    bool is_sign_overflow(uint8_t val1, int8_t val2, uint8_t new_value)
    {
        return ((val1 & 0x80) == (val2 & 0x80) &&
            ((val1 & 0x80) != (new_value & 0x80)));
    }

    string get_op_str(const char *op, nes_addr_mode addr_mode, bool is_official = true);
    void append_operand_str(string &str, nes_addr_mode addr_mode);

    void branch(bool cond, nes_addr_mode addr_mode);

    // ADC - Add with carry
    void ADC(nes_addr_mode addr_mode);
    void _ADC(uint8_t val);

    // AND - Logical AND
    void AND(nes_addr_mode addr_mode);

    // ASL - Arithmetic Shift Left
    void ASL(nes_addr_mode addr_mode);
    
    // BCC - Branch if Carry Clear
    void BCC(nes_addr_mode addr_mode);

    // BCS - Branch if Carry Set 
    void BCS(nes_addr_mode addr_mode);

    // BEQ - Branch if Equal
    void BEQ(nes_addr_mode addr_mode);

    // BIT - Bit test
    void BIT(nes_addr_mode addr_mode);

    // BMI - Branch if minus
    void BMI(nes_addr_mode addr_mode);

    // BNE - Branch if not equal
    void BNE(nes_addr_mode addr_mode);

    // BPL - Branch if positive 
    void BPL(nes_addr_mode addr_mode);

    // BRK - Force interrupt
    void BRK(nes_addr_mode addr_mode);

    // BVC - Branch if overflow clear
    void BVC(nes_addr_mode addr_mode);

    // BVS - Branch if overflow set
    void BVS(nes_addr_mode addr_mode);

    // CLC - Clear carry flag
    void CLC(nes_addr_mode addr_mode);

    // CLD - Clear decimal mode
    void CLD(nes_addr_mode addr_mode);

    // CLI - Clear interrupt disable
    void CLI(nes_addr_mode addr_mode);

    // CLV - Clear overflow flag
    void CLV(nes_addr_mode addr_mode);

    // CMP - Compare 
    void CMP(nes_addr_mode addr_mode);

    // CPX - Compare X register
    void CPX(nes_addr_mode addr_mode);

    // CPY - Compare Y register
    void CPY(nes_addr_mode addr_mode);

    // DEC - Decrement memory
    void DEC(nes_addr_mode addr_mode);

    // DEX - Decrement X register
    void DEX(nes_addr_mode addr_mode);

    // DEY - Decrement Y register
    void DEY(nes_addr_mode addr_mode);

    // Exclusive OR 
    void EOR(nes_addr_mode addr_mode);

    // INC - Increment memory
    void INC(nes_addr_mode addr_mode);

    // INX - Increment X
    void INX(nes_addr_mode addr_mode);

    // INY - Increment Y
    void INY(nes_addr_mode addr_mode);

    // JMP - Jump 
    void JMP(nes_addr_mode addr_mode);

    // JSR - Jump to subroutine
    void JSR(nes_addr_mode addr_mode);

    // LDA - Load Accumulator
    void LDA(nes_addr_mode addr_mode);

    // LDX - Load X register
    void LDX(nes_addr_mode addr_mode);

    // LDY - Load Y register
    void LDY(nes_addr_mode addr_mode);

    // LSR - Logical shift right
    void LSR(nes_addr_mode addr_mode);

    // NOP - NOP
    void NOP(nes_addr_mode addr_mode);

    // ORA - Logical Inclusive OR
    void ORA(nes_addr_mode addr_mode);

    // PHA - Push accumulator
    void PHA(nes_addr_mode addr_mode);

    // PHP - Push processor status
    void PHP(nes_addr_mode addr_mode);

    // PLA - Pull accumulator
    void PLA(nes_addr_mode addr_mode);

    // PLP - Pull processor status
    void PLP(nes_addr_mode addr_mode);
    void _PLP();

    // ROL - Rotate left
    void ROL(nes_addr_mode addr_mode);

    // ROR - Rotate right
    void ROR(nes_addr_mode addr_mode);

    // RTI - Return from interrupt
    void RTI(nes_addr_mode addr_mode);

    // RTS - Return from subroutine
    void RTS(nes_addr_mode addr_mode);

    // SBC - Subtract with carry
    void SBC(nes_addr_mode addr_mode);
    void _SBC(uint8_t val);

    // SEC - Set carry flag
    void SEC(nes_addr_mode addr_mode);

    // SED - Set decimal flag
    void SED(nes_addr_mode addr_mode);

    // SEI - Set interrupt disable
    void SEI(nes_addr_mode addr_mode);

    // STA - Store Accumulator  
    void STA(nes_addr_mode addr_mode);

    // STX - Store X
    void STX(nes_addr_mode addr_mode);

    // STY- Store Y
    void STY(nes_addr_mode addr_mode);

    // TAX - Transfer accumulator to X 
    void TAX(nes_addr_mode addr_mode);

    // TAY - Transfer accumulator to Y
    void TAY(nes_addr_mode addr_mode);

    // TSX - Transfer stack pointer to X 
    void TSX(nes_addr_mode addr_mode);

    // TXA - Transfer X to acc
    void TXA(nes_addr_mode addr_mode);

    // TXS - Transfer X to stack pointer
    void TXS(nes_addr_mode addr_mode);

    // TYA - Transfer Y to accumulator
    void TYA(nes_addr_mode addr_mode);

    // KIL - Kill?
    void KIL(nes_addr_mode addr_mode);

    //===================================================================================
    // Unofficial OP codes
    //===================================================================================

    void ALR(nes_addr_mode addr_mode);
    void ANC(nes_addr_mode addr_mode);
    void ARR(nes_addr_mode addr_mode);
    void AXS(nes_addr_mode addr_mode);
    void LAX(nes_addr_mode addr_mode);
    void SAX(nes_addr_mode addr_mode);
    void DCP(nes_addr_mode addr_mode);
    void ISC(nes_addr_mode addr_mode);
    void RLA(nes_addr_mode addr_mode);
    void RRA(nes_addr_mode addr_mode);
    void SLO(nes_addr_mode addr_mode);
    void SRE(nes_addr_mode addr_mode);
    
    void XAA(nes_addr_mode addr_mode);
    void AHX(nes_addr_mode addr_mode);
    void TAS(nes_addr_mode addr_mode);
    void LAS(nes_addr_mode addr_mode);

private :
    nes_system      *_system;
    nes_memory      *_mem;
    nes_ppu         *_ppu;
    nes_cpu_context _context;
    nes_cycle_t     _cycle;
    bool            _nmi_pending;           // NMI interrupt pending from PPU vertical blanking
    bool            _dma_pending;           // OAMDMA is requested from writing $4014
    uint16_t        _dma_addr;              // starting address
    bool            _stop_at_infinite_loop; // stop at when the ROM starts infinite loop - useful for testing
    bool            _is_stop_at_addr;       // stop at a certain address - useful for testing
    uint16_t        _stop_at_addr;          // stop at a certain address - useful for testing
};

