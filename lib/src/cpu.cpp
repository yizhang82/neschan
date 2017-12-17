#include "stdafx.h"
#include "cpu.h"

#define IS_ALU_OP_CODE_(op, offset, mode) case nes_op_code::op##_base + offset : op(nes_addr_mode::nes_addr_mode_##mode); break; 
#define IS_ALU_OP_CODE(op) \
    IS_ALU_OP_CODE_(op, 0x9, imm) \
    IS_ALU_OP_CODE_(op, 0x5, zp) \
    IS_ALU_OP_CODE_(op, 0x15, zp_ind_x) \
    IS_ALU_OP_CODE_(op, 0xd, abs) \
    IS_ALU_OP_CODE_(op, 0x1d, abs_x) \
    IS_ALU_OP_CODE_(op, 0x19, abs_y) \
    IS_ALU_OP_CODE_(op, 0x1, ind_x) \
    IS_ALU_OP_CODE_(op, 0x11, ind_y)

#define IS_RMW_OP_CODE_(op, opcode, offset, mode) case opcode + offset : op(nes_addr_mode::nes_addr_mode_##mode); break; 
#define IS_RMW_OP_CODE(op, opcode) \
    IS_RMW_OP_CODE_(op, opcode, 0x6, zp) \
    IS_RMW_OP_CODE_(op, opcode, 0xa, acc) \
    IS_RMW_OP_CODE_(op, opcode, 0x16, zp_ind_x) \
    IS_RMW_OP_CODE_(op, opcode, 0xe, abs) \
    IS_RMW_OP_CODE_(op, opcode, 0x1e, abs_x)

#define IS_OP_CODE(op, opcode) case opcode : op(nes_addr_mode_imp); break;
#define IS_OP_CODE_MODE(op, opcode, mode) case opcode : op(nes_addr_mode_##mode); break;

void nes_cpu::execute()
{
    while (true)
    {
        // next op
        auto op_code = (nes_op_code)decode_byte();

        // Let's start with a switch / case
        // Compiler should do good enough job to create a jump table
        // The problem with starting with my own table is that it get massive with lots of empty entries before I code
        // up any instructions.
        switch (op_code)
        {
        IS_ALU_OP_CODE(ADC)
        IS_ALU_OP_CODE(AND)
        IS_ALU_OP_CODE(CMP)
        IS_ALU_OP_CODE(EOR)
        IS_ALU_OP_CODE(ORA)
        IS_ALU_OP_CODE(SBC)
        IS_ALU_OP_CODE(STA)
        IS_ALU_OP_CODE(LDA)

        IS_RMW_OP_CODE(ASL, 0x0)
        IS_RMW_OP_CODE(ROL, 0x20)
        IS_RMW_OP_CODE(LSR, 0x40)
        IS_RMW_OP_CODE(ROR, 0x60)

        IS_OP_CODE_MODE(LDX, 0xa2, imm)
        IS_OP_CODE_MODE(LDX, 0xa6, zp)
        IS_OP_CODE_MODE(LDX, 0xb6, zp_ind_y)
        IS_OP_CODE_MODE(LDX, 0xae, abs)
        IS_OP_CODE_MODE(LDX, 0xbe, abs_y)

        IS_OP_CODE_MODE(LDY, 0xa0, imm)
        IS_OP_CODE_MODE(LDY, 0xa4, zp)
        IS_OP_CODE_MODE(LDY, 0xb4, zp_ind_x)
        IS_OP_CODE_MODE(LDY, 0xac, abs)
        IS_OP_CODE_MODE(LDY, 0xbc, abs_x)

        IS_OP_CODE_MODE(STX, 0x86, zp)
        IS_OP_CODE_MODE(STX, 0x96, zp_ind_y)
        IS_OP_CODE_MODE(STX, 0x8e, abs)

        IS_OP_CODE_MODE(STY, 0x84, zp)
        IS_OP_CODE_MODE(STY, 0x94, zp_ind_x)
        IS_OP_CODE_MODE(STY, 0x8c, abs)

        IS_OP_CODE(TAX, 0xaa)
        IS_OP_CODE(TAY, 0xa8)
        IS_OP_CODE(TSX, 0xba)
        IS_OP_CODE(TXA, 0x8a)
        IS_OP_CODE(TXS, 0x9a)
        IS_OP_CODE(TYA, 0x98)

        IS_OP_CODE_MODE(INC, 0xe6, zp)
        IS_OP_CODE_MODE(INC, 0xf6, zp_ind_x)
        IS_OP_CODE_MODE(INC, 0xee, abs)
        IS_OP_CODE_MODE(INC, 0xfe, abs_x)
        IS_OP_CODE(INX, 0xe8)
        IS_OP_CODE(INY, 0xc8)
        IS_OP_CODE_MODE(DEC, 0xc6, zp)
        IS_OP_CODE_MODE(DEC, 0xd6, zp_ind_x)
        IS_OP_CODE_MODE(DEC, 0xce, abs)
        IS_OP_CODE_MODE(DEC, 0xde, abs_x)
        IS_OP_CODE(DEX, 0xca)
        IS_OP_CODE(DEY, 0x88)

        IS_OP_CODE(SEC, 0x38)
        IS_OP_CODE(SED, 0xf8)
        IS_OP_CODE(SEI, 0x78)
        IS_OP_CODE(CLC, 0x18)
        IS_OP_CODE(CLD, 0xd8)
        IS_OP_CODE(CLI, 0x58)
        IS_OP_CODE(CLV, 0xB8)

        IS_OP_CODE_MODE(JMP, 0x4c, abs)
        IS_OP_CODE_MODE(JMP, 0x6c, ind)
        
        IS_OP_CODE_MODE(BCC, 0x90, rel)
        IS_OP_CODE_MODE(BCS, 0xb0, rel)
        IS_OP_CODE_MODE(BEQ, 0xf0, rel)
        IS_OP_CODE_MODE(BMI, 0x30, rel)
        IS_OP_CODE_MODE(BNE, 0xd0, rel)
        IS_OP_CODE_MODE(BPL, 0x10, rel)
        IS_OP_CODE_MODE(BVS, 0x70, rel)

        IS_OP_CODE_MODE(BIT, 0x24, zp)
        IS_OP_CODE_MODE(BIT, 0x2c, abs)

        case nes_op_code::BRK:
            return;

        default:
            break;
        }
    }
}

// Add with carry
void nes_cpu::ADC(nes_addr_mode addr_mode)
{
    uint8_t val = read_operand(decode_operand(addr_mode));
    A() = val + A() + get_carry();

    // flags
    determine_overflow_flag(val, A());
    calc_alu_flag(A());
}

// Logical AND
void nes_cpu::AND(nes_addr_mode addr_mode)
{
    uint8_t val = read_operand(decode_operand(addr_mode));
    A() &= val;

    // flags
    calc_alu_flag(val);
}

// Compare 
void nes_cpu::CMP(nes_addr_mode addr_mode)
{
    uint8_t val = read_operand(decode_operand(addr_mode));

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
void nes_cpu::EOR(nes_addr_mode addr_mode)
{
    uint8_t val = read_operand(decode_operand(addr_mode));
    A() ^= val;

    // flags
    calc_alu_flag(A());
}

// Logical Inclusive OR
void nes_cpu::ORA(nes_addr_mode addr_mode)
{
    uint8_t val = read_operand(decode_operand(addr_mode));
    A() |= val;

    calc_alu_flag(A());
}

// Subtract with carry
void nes_cpu::SBC(nes_addr_mode addr_mode)
{
    uint8_t val = read_operand(decode_operand(addr_mode));
    A() = A() - val - (1 - get_carry());

    // flags
    set_carry_flag(A() > val);
    determine_overflow_flag(val, A());
    calc_alu_flag(A());
}

// Load Accumulator
void nes_cpu::LDA(nes_addr_mode addr_mode)
{
    uint8_t val = read_operand(decode_operand(addr_mode));
    A() = val;

    // flags
    calc_alu_flag(A());
}

// Store Accumulator  
void nes_cpu::STA(nes_addr_mode addr_mode)
{
    // STA imm (0x89) = NOP
    if (addr_mode == nes_addr_mode::nes_addr_mode_imm)
        return;

    poke(decode_operand_addr(addr_mode), A());

    // Doesn't impact any flags
}

// ASL - Arithmetic shift left
void nes_cpu::ASL(nes_addr_mode addr_mode) 
{
    operand op = decode_operand(addr_mode);
    uint8_t val = read_operand(op);
    uint8_t new_val = val << 1;
    write_operand(op, new_val);

    // flags
    set_carry_flag(val & 0x80);
    set_zero_flag(A() == 0);
    set_negative_flag(new_val & 0x80);
}

// BCC - Branch if Carry Clear
void nes_cpu::BCC(nes_addr_mode addr_mode) 
{
    if (is_overflow())
        PC() = PC() + (int8_t) decode_byte();
}


// BCS - Branch if Carry Set 
void nes_cpu::BCS(nes_addr_mode addr_mode) 
{
    if (get_carry())
        PC() = PC() + (int8_t) decode_byte();
}

// BEQ - Branch if Equal
void nes_cpu::BEQ(nes_addr_mode addr_mode) 
{
    if (is_zero())
        PC() = PC() + (int8_t) decode_byte();
}

// BIT - Bit test
void nes_cpu::BIT(nes_addr_mode addr_mode) 
{
    uint8_t val = read_operand(decode_operand(addr_mode));
    uint8_t new_val = val & A();

    set_zero_flag(new_val == 0);
    set_overflow_flag(val & 0x40);
    set_negative_flag(val & 0x80);
}

// BMI - Branch if minus
void nes_cpu::BMI(nes_addr_mode addr_mode) 
{
    if (is_negative())
        PC() = PC() + (int8_t) decode_byte(); 
}

// BNE - Branch if not equal
void nes_cpu::BNE(nes_addr_mode addr_mode) 
{
    if (!is_zero())
        PC() = PC() + (int8_t) decode_byte(); 
}

// BPL - Branch if positive 
void nes_cpu::BPL(nes_addr_mode addr_mode) 
{
    if (!is_negative())
        PC() = PC() + (int8_t) decode_byte(); 
}

// BRK - Force interrupt
void nes_cpu::BRK(nes_addr_mode addr_mode) {}

// BVC - Branch if overflow clear
void nes_cpu::BVC(nes_addr_mode addr_mode) 
{
    if (!is_overflow())
        PC() = PC() + (int8_t) decode_byte(); 
}

// BVS - Branch if overflow set
void nes_cpu::BVS(nes_addr_mode addr_mode) 
{
    if (!is_overflow())
        PC() = PC() + (int8_t) decode_byte(); 
}

// CLC - Clear carry flag
void nes_cpu::CLC(nes_addr_mode addr_mode) { set_carry_flag(false); }

// CLD - Clear decimal mode
void nes_cpu::CLD(nes_addr_mode addr_mode) { set_decimal_flag(false); }

// CLI - Clear interrupt disable
void nes_cpu::CLI(nes_addr_mode addr_mode) { set_interrupt_flag(false); }

// CLV - Clear overflow flag
void nes_cpu::CLV(nes_addr_mode addr_mode) { set_overflow_flag(false); }

// CPX - Compare X register
void nes_cpu::CPX(nes_addr_mode addr_mode) {}

// CPY - Compare Y register
void nes_cpu::CPY(nes_addr_mode addr_mode) {}

// DEC - Decrement memory
void nes_cpu::DEC(nes_addr_mode addr_mode) 
{
    uint16_t addr = decode_operand_addr(addr_mode);
    uint8_t new_val = peek(addr) - 1;
    poke(addr, new_val);

    calc_alu_flag(new_val);
}

// DEX - Decrement X register
void nes_cpu::DEX(nes_addr_mode addr_mode) { X()--; }

// DEY - Decrement Y register
void nes_cpu::DEY(nes_addr_mode addr_mode) { Y()--; }

// INC - Increment memory
void nes_cpu::INC(nes_addr_mode addr_mode) 
{
    uint16_t addr = decode_operand_addr(addr_mode);
    uint8_t new_val = peek(addr) + 1;
    poke(addr, new_val);

    // flags
    calc_alu_flag(new_val);
}

// INX - Increment X
void nes_cpu::INX(nes_addr_mode addr_mode) 
{
    X() = X() + 1;

    calc_alu_flag(X());
}

// INY - Increment Y
void nes_cpu::INY(nes_addr_mode addr_mode) 
{
    Y() = Y() + 1;

    calc_alu_flag(Y());
}

// JMP - Jump 
void nes_cpu::JMP(nes_addr_mode addr_mode) 
{
    PC() = decode_operand_addr(addr_mode);

    // No impact to flags
}

// JSR - Jump to subroutine
void nes_cpu::JSR(nes_addr_mode addr_mode) {}

// LDX - Load X register
void nes_cpu::LDX(nes_addr_mode addr_mode) 
{
    X() = read_operand(decode_operand(addr_mode));

    calc_alu_flag(X());
}

// LDY - Load Y register
void nes_cpu::LDY(nes_addr_mode addr_mode) 
{
    Y() = read_operand(decode_operand(addr_mode));

    calc_alu_flag(Y());
}

// LSR - Logical shift right
void nes_cpu::LSR(nes_addr_mode addr_mode)
{
    operand op = decode_operand(addr_mode);
    uint8_t val = read_operand(op);
    uint8_t new_val = (val >> 1);
    write_operand(op, new_val);

    // flags
    set_carry_flag(val & 0x1);
    set_zero_flag(A() == 0);
    set_negative_flag(new_val & 0x80);
}

// NOP - NOP
void nes_cpu::NOP(nes_addr_mode addr_mode) {}

// PHA - Push accumulator
void nes_cpu::PHA(nes_addr_mode addr_mode) {}

// PHP - Push processor status
void nes_cpu::PHP(nes_addr_mode addr_mode) {}

// PLA - Pull accumulator
void nes_cpu::PLA(nes_addr_mode addr_mode) {}

// PLP - Pull processor status
void nes_cpu::PLP(nes_addr_mode addr_mode) {}

// ROL - Rotate left
void nes_cpu::ROL(nes_addr_mode addr_mode)
{
    operand op = decode_operand(addr_mode);
    uint8_t val = read_operand(op);
    uint8_t new_val = (val << 1) & get_carry();
    write_operand(op, new_val);

    // flags
    set_carry_flag(val & 0x80);
    set_zero_flag(A() == 0);
    set_negative_flag(new_val & 0x80);
}

// ROR - Rotate right
void nes_cpu::ROR(nes_addr_mode addr_mode)
{
    operand op = decode_operand(addr_mode);
    uint8_t val = read_operand(op);
    uint8_t new_val = (val >> 1) & (get_carry() << 7);
    write_operand(op, new_val);

    // flags
    set_carry_flag(val & 0x1);
    set_zero_flag(A() == 0);
    set_negative_flag(new_val & 0x80);
}

// RTI - Return from interrupt
void nes_cpu::RTI(nes_addr_mode addr_mode) {}

// RTS - Return from subroutine
void nes_cpu::RTS(nes_addr_mode addr_mode) {}

// SEC - Set carry flag
void nes_cpu::SEC(nes_addr_mode addr_mode) { set_carry_flag(true); }

// SED - Set decimal flag
void nes_cpu::SED(nes_addr_mode addr_mode) { set_decimal_flag(true); }

// SEI - Set interrupt disable
void nes_cpu::SEI(nes_addr_mode addr_mode) { set_interrupt_flag(true); }

// STX - Store X
void nes_cpu::STX(nes_addr_mode addr_mode) 
{
    poke(decode_operand_addr(addr_mode), X());

    // Doesn't impact any flags
}

// STY- Store Y
void nes_cpu::STY(nes_addr_mode addr_mode)
{
    poke(decode_operand_addr(addr_mode), Y());

    // Doesn't impact any flags
}

// TAX - Transfer accumulator to X 
void nes_cpu::TAX(nes_addr_mode addr_mode) 
{
    X() = A();

    calc_alu_flag(X());
}

// TAY - Transfer accumulator to Y
void nes_cpu::TAY(nes_addr_mode addr_mode)
{
    Y() = A();

    calc_alu_flag(Y());
}

// TSX - Transfer stack pointer to X 
void nes_cpu::TSX(nes_addr_mode addr_mode) 
{
    X() = S();

    calc_alu_flag(X());
}

// TXA - Transfer X to acc
void nes_cpu::TXA(nes_addr_mode addr_mode)
{
    A() = X();

    calc_alu_flag(A());
}

// TXS - Transfer X to stack pointer
void nes_cpu::TXS(nes_addr_mode addr_mode)
{
    S() = X();

    // Doesn't impact flags
}

// TYA - Transfer Y to accumulator
void nes_cpu::TYA(nes_addr_mode addr_mode) 
{
    A() = Y();

    calc_alu_flag(A());
}