#include "stdafx.h"
#include "cpu.h"

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
#define IS_OP_CODE_(op, mode) case nes_op_code::op##_base + nes_addr_mode_offset::nes_addr_mode_offset_##mode : op(nes_addr_mode::nes_addr_mode_##mode); break; 
#define IS_OP_CODE(op) \
                IS_OP_CODE_(op, imm) \
                IS_OP_CODE_(op, zp) \
                IS_OP_CODE_(op, zp_ind_x) \
                IS_OP_CODE_(op, abs) \
                IS_OP_CODE_(op, abs_x) \
                IS_OP_CODE_(op, abs_y) \
                IS_OP_CODE_(op, ind_x) \
                IS_OP_CODE_(op, ind_y)

            IS_OP_CODE(ADC)
                IS_OP_CODE(AND)
                IS_OP_CODE(CMP)
                IS_OP_CODE(EOR)
                IS_OP_CODE(ORA)
                IS_OP_CODE(SBC)
                IS_OP_CODE(STA)
                IS_OP_CODE(LDA)

        case nes_op_code::BRK:
            return;

        default:
            break;
        }
    }
}