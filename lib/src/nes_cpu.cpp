#include "stdafx.h"
#include "nes_cpu.h"
#include "nes_system.h"
#include "trace.h"

void nes_cpu::power_on(nes_system *system)
{
    _system = system;
    _mem = system->ram();

    // @TODO - Simulate full power-on state
    // http://wiki.nesdev.com/w/index.php/CPU_power_up_state
    _context.P = 0x24;          // @TODO - Should be 0x34 - but temporarily set to 0x24 to match nintendulator baseline 
    _context.A = _context.X = _context.Y = 0;
    _context.S = 0xfd;
    _context.PC = 0;
}

void nes_cpu::reset()
{

}

void nes_cpu::step_to(nes_cycle_t count)
{
    // @TODO - Implement correct cycle count
    execute();

    _system->stop();
}

#define IS_ALU_OP_CODE_(op, offset, mode) case nes_op_code::op##_base + offset : LOG(get_op_str(#op, nes_addr_mode::nes_addr_mode_##mode)); op(nes_addr_mode::nes_addr_mode_##mode); break; 
#define IS_ALU_OP_CODE(op) \
    IS_ALU_OP_CODE_(op, 0x9, imm) \
    IS_ALU_OP_CODE_(op, 0x5, zp) \
    IS_ALU_OP_CODE_(op, 0x15, zp_ind_x) \
    IS_ALU_OP_CODE_(op, 0xd, abs) \
    IS_ALU_OP_CODE_(op, 0x1d, abs_x) \
    IS_ALU_OP_CODE_(op, 0x19, abs_y) \
    IS_ALU_OP_CODE_(op, 0x1, ind_x) \
    IS_ALU_OP_CODE_(op, 0x11, ind_y)

#define IS_ALU_OP_CODE_NO_IMM(op) \
    IS_ALU_OP_CODE_(op, 0x5, zp) \
    IS_ALU_OP_CODE_(op, 0x15, zp_ind_x) \
    IS_ALU_OP_CODE_(op, 0xd, abs) \
    IS_ALU_OP_CODE_(op, 0x1d, abs_x) \
    IS_ALU_OP_CODE_(op, 0x19, abs_y) \
    IS_ALU_OP_CODE_(op, 0x1, ind_x) \
    IS_ALU_OP_CODE_(op, 0x11, ind_y)

#define IS_RMW_OP_CODE_(op, opcode, offset, mode) case opcode + offset : LOG(get_op_str(#op, nes_addr_mode::nes_addr_mode_##mode)); op(nes_addr_mode::nes_addr_mode_##mode); break; 
#define IS_RMW_OP_CODE(op, opcode) \
    IS_RMW_OP_CODE_(op, opcode, 0x6, zp) \
    IS_RMW_OP_CODE_(op, opcode, 0xa, acc) \
    IS_RMW_OP_CODE_(op, opcode, 0x16, zp_ind_x) \
    IS_RMW_OP_CODE_(op, opcode, 0xe, abs) \
    IS_RMW_OP_CODE_(op, opcode, 0x1e, abs_x)

#define IS_OP_CODE(op, opcode) case opcode : LOG(get_op_str(#op, nes_addr_mode_imp)); op(nes_addr_mode_imp); break;
#define IS_OP_CODE_MODE(op, opcode, mode) case opcode : LOG(get_op_str(#op, nes_addr_mode_##mode)); op(nes_addr_mode_##mode); break;

#define IS_UNOFFICIAL_OP_CODE(op, opcode) case opcode : LOG(get_op_str(#op, nes_addr_mode_imp, false)); op(nes_addr_mode_imp); break;
#define IS_UNOFFICIAL_OP_CODE_MODE(op, opcode, mode) case opcode : LOG(get_op_str(#op, nes_addr_mode_##mode, false)); op(nes_addr_mode_##mode); break;

void nes_cpu::execute()
{
    LOG("[NES_CPU] Starts code at " << PC());

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
        IS_ALU_OP_CODE_NO_IMM(STA)
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

        IS_OP_CODE_MODE(CPX, 0xe0, imm)
        IS_OP_CODE_MODE(CPX, 0xe4, zp)
        IS_OP_CODE_MODE(CPX, 0xec, abs)
        IS_OP_CODE_MODE(CPY, 0xc0, imm)
        IS_OP_CODE_MODE(CPY, 0xc4, zp)
        IS_OP_CODE_MODE(CPY, 0xcc, abs)

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

        IS_OP_CODE_MODE(JMP, 0x4c, abs_jmp)
        IS_OP_CODE_MODE(JMP, 0x6c, ind_jmp)
        
        IS_OP_CODE_MODE(BCC, 0x90, rel)
        IS_OP_CODE_MODE(BCS, 0xb0, rel)
        IS_OP_CODE_MODE(BEQ, 0xf0, rel)
        IS_OP_CODE_MODE(BMI, 0x30, rel)
        IS_OP_CODE_MODE(BNE, 0xd0, rel)
        IS_OP_CODE_MODE(BPL, 0x10, rel)
        IS_OP_CODE_MODE(BVC, 0x50, rel)
        IS_OP_CODE_MODE(BVS, 0x70, rel)

        IS_OP_CODE_MODE(BIT, 0x24, zp)
        IS_OP_CODE_MODE(BIT, 0x2c, abs)

        IS_OP_CODE(PHA, 0x48)
        IS_OP_CODE(PHP, 0x08)
        IS_OP_CODE(PLA, 0x68)
        IS_OP_CODE(PLP, 0x28)

        IS_OP_CODE(RTI, 0x40)
        IS_OP_CODE_MODE(JSR, 0x20, abs_jmp)

        case 0x60:
            // If RTS and SP = 0xfd (empty stack), terminate the loop
            if (S() == 0xfd)
            {
                LOG(get_op_str("RTS", nes_addr_mode_imp)); 
                LOG("[NES_CPU] RTS when SP = $FD -> terminate");
                return;
            }
            LOG(get_op_str("RTS", nes_addr_mode_imp)); 
            RTS(nes_addr_mode_imp);
            break;

        case 0x02:
        case 0x12:
        case 0x22:
        case 0x32:
        case 0x42:
        case 0x52:
        case 0x62:
        case 0x72:
        case 0x92:
        case 0xB2:
        case 0xd2:
        case 0xf2:
            LOG(get_op_str("KIL", nes_addr_mode_imp)); 
            KIL(nes_addr_mode_imp);
            return;

        // The real NOP
        IS_OP_CODE_MODE(NOP, 0xea, imp)

        // All the other "NOP"
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0x80, imm)

        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0x04, zp)
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0x44, zp)
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0x64, zp)

        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0x0c, abs)

        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0x14, zp_ind_x)
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0x34, zp_ind_x)
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0x54, zp_ind_x)
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0x74, zp_ind_x)
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0xd4, zp_ind_x)
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0xf4, zp_ind_x)

        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0x1c, abs_x)
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0x3c, abs_x)
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0x5c, abs_x)
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0x7c, abs_x)
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0xdc, abs_x)
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0xfc, abs_x)
       
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0x89, imm)

        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0x82, imm)
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0xc2, imm)
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0xe2, imm)

        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0x1a, imp)
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0x3a, imp)
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0x5a, imp)
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0x7a, imp)
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0xda, imp)
        IS_UNOFFICIAL_OP_CODE_MODE(NOP, 0xfa, imp)

        IS_UNOFFICIAL_OP_CODE_MODE(SLO, 0x03, ind_x)     
        IS_UNOFFICIAL_OP_CODE_MODE(SLO, 0x07, zp)
        IS_UNOFFICIAL_OP_CODE_MODE(ANC, 0x0b, imm)
        IS_UNOFFICIAL_OP_CODE_MODE(SLO, 0x0f, abs)
        IS_UNOFFICIAL_OP_CODE_MODE(SLO, 0x13, ind_y)
        IS_UNOFFICIAL_OP_CODE_MODE(SLO, 0x17, zp_ind_x)
        IS_UNOFFICIAL_OP_CODE_MODE(SLO, 0x1b, abs_y)
        IS_UNOFFICIAL_OP_CODE_MODE(SLO, 0x1f, abs_x)

        IS_UNOFFICIAL_OP_CODE_MODE(RLA, 0x23, ind_x)     
        IS_UNOFFICIAL_OP_CODE_MODE(RLA, 0x27, zp)
        IS_UNOFFICIAL_OP_CODE_MODE(ANC, 0x2b, imm)
        IS_UNOFFICIAL_OP_CODE_MODE(RLA, 0x2f, abs)
        IS_UNOFFICIAL_OP_CODE_MODE(RLA, 0x33, ind_y)
        IS_UNOFFICIAL_OP_CODE_MODE(RLA, 0x37, zp_ind_x)
        IS_UNOFFICIAL_OP_CODE_MODE(RLA, 0x3b, abs_y)
        IS_UNOFFICIAL_OP_CODE_MODE(RLA, 0x3f, abs_x)

        IS_UNOFFICIAL_OP_CODE_MODE(SRE, 0x43, ind_x)     
        IS_UNOFFICIAL_OP_CODE_MODE(SRE, 0x47, zp)
        IS_UNOFFICIAL_OP_CODE_MODE(ALR, 0x4b, imm)
        IS_UNOFFICIAL_OP_CODE_MODE(SRE, 0x4f, abs)
        IS_UNOFFICIAL_OP_CODE_MODE(SRE, 0x53, ind_y)
        IS_UNOFFICIAL_OP_CODE_MODE(SRE, 0x57, zp_ind_x)
        IS_UNOFFICIAL_OP_CODE_MODE(SRE, 0x5b, abs_y)
        IS_UNOFFICIAL_OP_CODE_MODE(SRE, 0x5f, abs_x)

        IS_UNOFFICIAL_OP_CODE_MODE(RRA, 0x63, ind_x)     
        IS_UNOFFICIAL_OP_CODE_MODE(RRA, 0x67, zp)
        IS_UNOFFICIAL_OP_CODE_MODE(ARR, 0x6b, imm)
        IS_UNOFFICIAL_OP_CODE_MODE(RRA, 0x6f, abs)
        IS_UNOFFICIAL_OP_CODE_MODE(RRA, 0x73, ind_y)
        IS_UNOFFICIAL_OP_CODE_MODE(RRA, 0x77, zp_ind_x)
        IS_UNOFFICIAL_OP_CODE_MODE(RRA, 0x7b, abs_y)
        IS_UNOFFICIAL_OP_CODE_MODE(RRA, 0x7f, abs_x)

        IS_UNOFFICIAL_OP_CODE_MODE(SAX, 0x83, ind_x)     
        IS_UNOFFICIAL_OP_CODE_MODE(SAX, 0x87, zp)
        IS_UNOFFICIAL_OP_CODE_MODE(XAA, 0x8b, imm)
        IS_UNOFFICIAL_OP_CODE_MODE(SAX, 0x8f, abs)
        IS_UNOFFICIAL_OP_CODE_MODE(AHX, 0x93, ind_y)
        IS_UNOFFICIAL_OP_CODE_MODE(SAX, 0x97, zp_ind_y)
        IS_UNOFFICIAL_OP_CODE_MODE(TAS, 0x9b, abs_y)
        IS_UNOFFICIAL_OP_CODE_MODE(AHX, 0x9f, abs_y)

        IS_UNOFFICIAL_OP_CODE_MODE(LAX, 0xa3, ind_x)     
        IS_UNOFFICIAL_OP_CODE_MODE(LAX, 0xa7, zp)
        IS_UNOFFICIAL_OP_CODE_MODE(LAX, 0xab, imm)
        IS_UNOFFICIAL_OP_CODE_MODE(LAX, 0xaf, abs)
        IS_UNOFFICIAL_OP_CODE_MODE(LAX, 0xb3, ind_y)
        IS_UNOFFICIAL_OP_CODE_MODE(LAX, 0xb7, zp_ind_y)
        IS_UNOFFICIAL_OP_CODE_MODE(LAS, 0xbb, zp_ind_y)
        IS_UNOFFICIAL_OP_CODE_MODE(LAX, 0xbf, abs_y)

        IS_UNOFFICIAL_OP_CODE_MODE(DCP, 0xc3, ind_x)     
        IS_UNOFFICIAL_OP_CODE_MODE(DCP, 0xc7, zp)
        IS_UNOFFICIAL_OP_CODE_MODE(AXS, 0xcb, imm)
        IS_UNOFFICIAL_OP_CODE_MODE(DCP, 0xcf, abs)
        IS_UNOFFICIAL_OP_CODE_MODE(DCP, 0xd3, ind_y)
        IS_UNOFFICIAL_OP_CODE_MODE(DCP, 0xd7, zp_ind_x)
        IS_UNOFFICIAL_OP_CODE_MODE(DCP, 0xdb, abs_y)
        IS_UNOFFICIAL_OP_CODE_MODE(DCP, 0xdf, abs_x)

        IS_UNOFFICIAL_OP_CODE_MODE(ISC, 0xe3, ind_x)     
        IS_UNOFFICIAL_OP_CODE_MODE(ISC, 0xe7, zp)
        IS_UNOFFICIAL_OP_CODE_MODE(SBC, 0xeb, imm)
        IS_UNOFFICIAL_OP_CODE_MODE(ISC, 0xef, abs)
        IS_UNOFFICIAL_OP_CODE_MODE(ISC, 0xf3, ind_y)
        IS_UNOFFICIAL_OP_CODE_MODE(ISC, 0xf7, zp_ind_x)
        IS_UNOFFICIAL_OP_CODE_MODE(ISC, 0xfb, abs_y)
        IS_UNOFFICIAL_OP_CODE_MODE(ISC, 0xff, abs_x)


        case 00:
            LOG(get_op_str("BRK", nes_addr_mode::nes_addr_mode_imp));
            return;

        default:
            LOG("[CPU] Unrecognized instruction or illegal instruction!");
            assert(false);
            return;
        }
    }
}

static void append_space(string &str)
{
    str.append(1, ' ');
}

static void append_hex(string &str, uint8_t val)
{
    if (val < 10)
        str.append(1, val + '0');
    else
        str.append(1, val - 10 + 'A');
}

static void append_byte(string &str, uint8_t val)
{
    append_hex(str, val >> 4);
    append_hex(str, val & 0xf);
}

static void append_word(string &str, uint16_t val)
{
    append_byte(str, val >> 8);
    append_byte(str, val & 0xff);
}

static void align(string &str, int loc)
{
    if (str.size() < loc)
        str.append(loc - str.size(), ' ');
}


// Follow Nintendulator log format
// 0         1         2         3         4         5         6         7         8
// 012345678901234567890123456789012345678901234567890123456789012345678901234567890
// C000  4C F5 C5  JMP $C5F5                       A:00 X:00 Y:00 P:24 SP:FD CYC:  0
string nes_cpu::get_op_str(const char *op, nes_addr_mode addr_mode, bool is_official)
{
    int operand_size = 0;
    switch (addr_mode)
    {
        case nes_addr_mode::nes_addr_mode_imp: 
        case nes_addr_mode::nes_addr_mode_acc:
            break;

        case nes_addr_mode::nes_addr_mode_rel: 
        case nes_addr_mode::nes_addr_mode_imm:
        case nes_addr_mode::nes_addr_mode_zp:
        case nes_addr_mode::nes_addr_mode_zp_ind_x:
        case nes_addr_mode::nes_addr_mode_zp_ind_y:
        case nes_addr_mode::nes_addr_mode_ind_x:
        case nes_addr_mode::nes_addr_mode_ind_y:
            operand_size = 1;
            break;

        case nes_addr_mode::nes_addr_mode_ind:
        case nes_addr_mode::nes_addr_mode_ind_jmp:
        case nes_addr_mode::nes_addr_mode_abs:
        case nes_addr_mode::nes_addr_mode_abs_jmp:
        case nes_addr_mode::nes_addr_mode_abs_x:
        case nes_addr_mode::nes_addr_mode_abs_y:
            operand_size = 2;
            break;

        default:
            assert(false);
    }

    string msg;

    // 0         1         2         3         4         5         6         7         8
    // 012345678901234567890123456789012345678901234567890123456789012345678901234567890
    // C000  4C F5 C5  JMP $C5F5                       A:00 X:00 Y:00 P:24 SP:FD CYC:  0

    // Opcode
    // We've already decoded the op code so it's PC - 1
    append_word(msg, PC() - 1);
    align(msg, 6);

    // Dump instruction bytes
    for (int i = 0; i < operand_size + 1; ++i)
    {
        append_byte(msg, peek(PC() - 1 + i));
        append_space(msg);
    }

    if (is_official)
    {
        align(msg, 16);
    }
    else
    {
        align(msg, 15);
        msg.append("*");
    }

    msg.append(op);
    append_operand_str(msg, addr_mode);

    align(msg, 48);

    msg.append("A:");
    append_byte(msg, A());
    append_space(msg);

    msg.append("X:");
    append_byte(msg, X());
    append_space(msg);

    msg.append("Y:");
    append_byte(msg, Y());
    append_space(msg);

    msg.append("P:");
    append_byte(msg, P());
    append_space(msg);

    msg.append("SP:");
    append_byte(msg, S());
    append_space(msg);

    msg.append("CYC:");
    msg.append("  0");

    return msg;
}

void nes_cpu::append_operand_str(string &str, nes_addr_mode addr_mode)
{
    append_space(str);
    switch (addr_mode)
    {
    case nes_addr_mode::nes_addr_mode_imp:
        return;

    case nes_addr_mode::nes_addr_mode_acc:
        str.append("A");
        break;

    case nes_addr_mode::nes_addr_mode_imm:
        str.append("#$");
        append_byte(str, peek(PC()));
        break;

    case nes_addr_mode::nes_addr_mode_rel:
        // display the real address directly after accounting for offset
        str.append("$");
        append_word(str, int8_t(peek(PC())) + PC() + 1);
        break;

    case nes_addr_mode::nes_addr_mode_zp:
    case nes_addr_mode::nes_addr_mode_zp_ind_x:
    case nes_addr_mode::nes_addr_mode_zp_ind_y:
    {
        str.append("$");
        uint8_t addr = peek(PC());
        append_byte(str, addr);
        if (addr_mode == nes_addr_mode_zp_ind_x)
        {
            str.append(",X @ ");
            addr += X();
            append_byte(str, addr);
        }
        else if (addr_mode == nes_addr_mode_zp_ind_y)
        {
            str.append(",Y @ ");
            addr += Y();
            append_byte(str, addr);
        }

        str.append(" = ");
        append_byte(str, peek(addr));
        break;
    }
    case nes_addr_mode::nes_addr_mode_abs_jmp:
    case nes_addr_mode::nes_addr_mode_abs:
    case nes_addr_mode::nes_addr_mode_abs_x:
    case nes_addr_mode::nes_addr_mode_abs_y:
    {
        str.append("$");
        uint16_t addr = peek_word(PC());
        append_word(str, addr);
        if (addr_mode != nes_addr_mode_abs_jmp)
        {
            if (addr_mode == nes_addr_mode_abs_x)
            {
                str.append(",X @ ");
                addr += X();
                append_word(str, addr);
            }
            else if (addr_mode == nes_addr_mode_abs_y)
            {
                str.append(",Y @ ");
                addr += Y();
                append_word(str, addr);
            }

            str.append(" = ");
            append_byte(str, peek(addr));
        }
        break;
    }
    case nes_addr_mode::nes_addr_mode_ind:
    {
        uint16_t addr = peek_word(PC());
        str.append("(");
        append_word(str, addr);
        str.append(") = ");

        append_word(str, peek_word(addr));
        break;
    }
    case nes_addr_mode::nes_addr_mode_ind_jmp:
    {
        uint16_t addr = peek_word(PC());
        str.append("(");
        append_word(str, addr);
        str.append(") = ");

        if ((addr & 0xff) == 0xff)
        {
            // Account for JMP hardware bug
            // http://wiki.nesdev.com/w/index.php/Errata
            append_word(str, peek(addr) + (uint16_t(peek(addr & 0xff00)) << 8));
        }
        else
        {
            append_word(str, peek_word(addr));
        }
        break;
    }


    case nes_addr_mode::nes_addr_mode_ind_x:
    {
        uint8_t addr = peek(PC());
        str.append("($");
        append_byte(str, addr);
        str.append(",X) @ ");
        append_byte(str, addr + X());
        uint16_t final_addr = peek((addr + _context.X) & 0xff) + (uint16_t(peek((addr + _context.X + 1) & 0xff)) << 8);
        str.append(" = ");
        append_word(str, final_addr);
        str.append(" = ");
        append_byte(str, peek(final_addr));
        break;
    }
    case nes_addr_mode::nes_addr_mode_ind_y:
    {
        uint8_t addr = peek(PC());
        str.append("($");
        append_byte(str, addr);
        str.append("),Y = ");
        uint16_t final_addr = peek(addr) + (uint16_t(peek((addr + 1) & 0xff)) << 8);
        append_word(str, final_addr);
        str.append(" @ ");
        final_addr += _context.Y;
        append_word(str, final_addr);
        str.append(" = ");
        append_byte(str, peek(final_addr));
        break;
    }

    default:
        assert(false);
    }
}

// Add with carry
void nes_cpu::ADC(nes_addr_mode addr_mode)
{
    uint8_t val = read_operand(decode_operand(addr_mode));
    _ADC(val);
}

void nes_cpu::_ADC(uint8_t val)
{
    uint8_t old_val = A();
    A() = val + A() + get_carry();

    // flags
    set_overflow_flag(is_sign_overflow(old_val, val, A()));
    set_carry_flag(old_val > A());
    calc_alu_flag(A());
}

// Logical AND
void nes_cpu::AND(nes_addr_mode addr_mode)
{
    uint8_t val = read_operand(decode_operand(addr_mode));
    A() &= val;

    // flags    
    calc_alu_flag(A());
}

// Compare 
void nes_cpu::CMP(nes_addr_mode addr_mode)
{
    uint8_t val = read_operand(decode_operand(addr_mode));

    // flags
    uint8_t diff = A() - val;

    set_carry_flag(A() >= val);
    set_zero_flag(diff == 0);
    set_negative_flag(diff & 0x80);
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
    _SBC(val);
}

void nes_cpu::_SBC(uint8_t val)
{
    val = ~val + 1;                     // turn it into a add operand
    val = val -(1 - get_carry());       // account for the carry
    uint8_t old_val = A();
    A() = A() + val;

    // flags
    set_overflow_flag(is_sign_overflow(old_val, val, A()));
    set_carry_flag(A() <= old_val);
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

    // @DOCBUG: 
    // http://obelisk.me.uk/6502/reference.html#ASL incorrectly states ASL detects A == 0
    set_zero_flag(new_val == 0); 
    set_negative_flag(new_val & 0x80);
}

// BCC - Branch if Carry Clear
void nes_cpu::BCC(nes_addr_mode addr_mode) 
{
    int8_t rel = (int8_t) decode_byte();
    if (!get_carry())
        PC() += rel;
}

// BCS - Branch if Carry Set 
void nes_cpu::BCS(nes_addr_mode addr_mode) 
{
    int8_t rel = (int8_t) decode_byte();
    if (get_carry())
        PC() += rel;
}

// BEQ - Branch if Equal
void nes_cpu::BEQ(nes_addr_mode addr_mode) 
{
    int8_t rel = (int8_t) decode_byte();
    if (is_zero()) 
        PC() += rel;
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
    int8_t rel = (int8_t) decode_byte();
    if (is_negative())
        PC() += rel;
}

// BNE - Branch if not equal
void nes_cpu::BNE(nes_addr_mode addr_mode) 
{
    int8_t rel = (int8_t) decode_byte();
    if (!is_zero())
        PC() += rel;
}

// BPL - Branch if positive 
void nes_cpu::BPL(nes_addr_mode addr_mode) 
{
    int8_t rel = (int8_t) decode_byte();
    if (!is_negative())
        PC() += rel;
}

// BRK - Force interrupt
void nes_cpu::BRK(nes_addr_mode addr_mode) {}

// BVC - Branch if overflow clear
void nes_cpu::BVC(nes_addr_mode addr_mode) 
{
    int8_t rel = (int8_t) decode_byte();
    if (!is_overflow())
        PC() += rel;
}

// BVS - Branch if overflow set
void nes_cpu::BVS(nes_addr_mode addr_mode) 
{
    int8_t rel = (int8_t) decode_byte();
    if (is_overflow())
        PC() += rel;
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
void nes_cpu::CPX(nes_addr_mode addr_mode) 
{
    uint8_t val = read_operand(decode_operand(addr_mode));

    // flags
    uint8_t diff = X() - val;

    set_carry_flag(X() >= val);
    set_zero_flag(diff == 0);
    set_negative_flag(diff & 0x80);
}

// CPY - Compare Y register
void nes_cpu::CPY(nes_addr_mode addr_mode) 
{
    uint8_t val = read_operand(decode_operand(addr_mode));

    // flags
    uint8_t diff = Y() - val;

    set_carry_flag(Y() >= val);
    set_zero_flag(diff == 0);
    set_negative_flag(diff & 0x80);
}

// DEC - Decrement memory
void nes_cpu::DEC(nes_addr_mode addr_mode) 
{
    uint16_t addr = decode_operand_addr(addr_mode);
    uint8_t new_val = peek(addr) - 1;
    poke(addr, new_val);

    calc_alu_flag(new_val);
}

// DEX - Decrement X register
void nes_cpu::DEX(nes_addr_mode addr_mode) 
{ 
    X()--; 
    calc_alu_flag(X());
}

// DEY - Decrement Y register
void nes_cpu::DEY(nes_addr_mode addr_mode) 
{ 
    Y()--; 
    calc_alu_flag(Y());
}

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
void nes_cpu::JSR(nes_addr_mode addr_mode) 
{
    // note: we push the actual return address -1, which is the current place (before decoding the 16-bit addr) + 1
    push_word(PC() + 1);

    PC() = decode_operand_addr(addr_mode);
}

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

    // @DOCBUG: 
    // http://obelisk.me.uk/6502/reference.html#ASL incorrectly states ASL detects A == 0
    set_zero_flag(new_val == 0);
    set_negative_flag(new_val & 0x80);
}

// NOP - NOP
void nes_cpu::NOP(nes_addr_mode addr_mode) 
{
    // For effective NOP (op codes that are "effectively" no-op but not the real NOP 0xea)
    // We always needed to decode the parameter
    if (addr_mode != nes_addr_mode::nes_addr_mode_imp)
        decode_operand(addr_mode);
}

// PHA - Push accumulator
void nes_cpu::PHA(nes_addr_mode addr_mode) 
{
    push_byte(A());
}

// PHP - Push processor status
void nes_cpu::PHP(nes_addr_mode addr_mode) 
{
    // http://wiki.nesdev.com/w/index.php/CPU_status_flag_behavior
    // Set bit 5 and 4 to 1 when copy status into from PHP
    push_byte(P() | 0x30);
}

// PLA - Pull accumulator
void nes_cpu::PLA(nes_addr_mode addr_mode) 
{
    A() = pop_byte();

    calc_alu_flag(A());
}

// PLP - Pull processor status
void nes_cpu::PLP(nes_addr_mode addr_mode) 
{
    // http://wiki.nesdev.com/w/index.php/CPU_status_flag_behavior
    // Bit 5 and 4 are ignored when pulled from stack - which means they are preserved
    // @TODO - Nintendulator actually always sets bit 5, not sure which one is correct
    // I'm setting bit 5 to make testing easier
    P() = (pop_byte() & 0xef) | (P() & 0x10) | 0x20;
}

// ROL - Rotate left
void nes_cpu::ROL(nes_addr_mode addr_mode)
{
    operand op = decode_operand(addr_mode);
    uint8_t val = read_operand(op);
    uint8_t new_val = (val << 1) | get_carry();
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
    uint8_t new_val = (val >> 1) | (get_carry() << 7);
    write_operand(op, new_val);

    // flags
    set_carry_flag(val & 0x1);
    set_zero_flag(A() == 0);
    set_negative_flag(new_val & 0x80);
}

// RTI - Return from interrupt
void nes_cpu::RTI(nes_addr_mode addr_mode) 
{
    PLP(nes_addr_mode_imp);

    uint16_t addr = pop_word();
    PC() = addr;
}

// RTS - Return from subroutine
void nes_cpu::RTS(nes_addr_mode addr_mode) 
{
    // See JSR - we pushed actual return address - 1
    uint16_t addr = pop_word() + 1;
    PC() = addr;
}

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

// KIL - Kill?
void nes_cpu::KIL(nes_addr_mode addr_mode)
{
}

//===================================================================================
// Unofficial OP codes
//===================================================================================

void nes_cpu::ALR(nes_addr_mode addr_mode) { assert(false); }
void nes_cpu::ANC(nes_addr_mode addr_mode) { assert(false); }
void nes_cpu::ARR(nes_addr_mode addr_mode) { assert(false); }
void nes_cpu::AXS(nes_addr_mode addr_mode) { assert(false); }

// LAX - LDA value then TAX
void nes_cpu::LAX(nes_addr_mode addr_mode)
{
    // LDA + TAX
    uint8_t val = read_operand(decode_operand(addr_mode));
    X() = A() = val;

    // flags
    calc_alu_flag(X());
}

// SAX - AND A X
void nes_cpu::SAX(nes_addr_mode addr_mode) 
{
    write_operand(decode_operand(addr_mode), A() & X());
}

// DCP - DEC value then CMP value
void nes_cpu::DCP(nes_addr_mode addr_mode) 
{
    // DEC
    auto operand = decode_operand(addr_mode);
    uint8_t val = read_operand(operand);
    val--;
    write_operand(operand, val);

    // CMP
    uint8_t diff = A() - val;

    set_carry_flag(A() >= val);
    set_zero_flag(diff == 0);
    set_negative_flag(diff & 0x80);
}

// ISC - INC value then SBC value 
void nes_cpu::ISC(nes_addr_mode addr_mode) 
{
    // INC
    auto operand = decode_operand(addr_mode);
    uint8_t val = read_operand(operand);
    val++;
    write_operand(operand, val);

    // SBC
    _SBC(val);
}

// RLA - ROL value then AND value
void nes_cpu::RLA(nes_addr_mode addr_mode) 
{
    // ROL
    operand op = decode_operand(addr_mode);
    uint8_t val = read_operand(op);
    uint8_t new_val = (val << 1) | get_carry();
    write_operand(op, new_val);

    set_carry_flag(val & 0x80);

    // AND
    A() &= new_val;

    // flags    
    calc_alu_flag(A());
}

void nes_cpu::RRA(nes_addr_mode addr_mode) 
{ 
    // ROR
    operand op = decode_operand(addr_mode);
    uint8_t val = read_operand(op);
    uint8_t new_val = (val >> 1) | (get_carry() << 7);
    write_operand(op, new_val);

    set_carry_flag(val & 0x1);

    // ADC
    _ADC(new_val);
}

// SLO - ASL value then ORA value
void nes_cpu::SLO(nes_addr_mode addr_mode) 
{ 
    // ASL
    operand op = decode_operand(addr_mode);
    uint8_t val = read_operand(op);
    uint8_t new_val = val << 1;
    write_operand(op, new_val);

    set_carry_flag(val & 0x80);

    // ORA
    A() |= new_val;

    calc_alu_flag(A());
}

// SRE - LSR value then EOR value
void nes_cpu::SRE(nes_addr_mode addr_mode) 
{
    // LSR
    operand op = decode_operand(addr_mode);
    uint8_t val = read_operand(op);
    uint8_t new_val = (val >> 1);
    write_operand(op, new_val);

    // flags
    set_carry_flag(val & 0x1);

    // EOR
    A() ^= new_val;

    // flags
    calc_alu_flag(A());
}

void nes_cpu::XAA(nes_addr_mode addr_mode) { assert(false); }
void nes_cpu::AHX(nes_addr_mode addr_mode) { assert(false); }
void nes_cpu::TAS(nes_addr_mode addr_mode) { assert(false); }
void nes_cpu::LAS(nes_addr_mode addr_mode) { assert(false); }