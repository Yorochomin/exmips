#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "rc.h"
#include "rc_x64.h"
#include "tlb.h"
#include "mem.h"
#include "mips_op.h"
#include "cp0.h"
#include "exception.h"
#include "ExInst_common.h"

// This function is used only for code-generation of an instruction in delay slot
static int genCode32(struct stMachineState *pM, uint8_t *imem, uint8_t *mem, int *pused, uintmx pointer, uintmx inst){

    // NOP instruction
    if( inst == 0 ){
        return 0;
    }

    int orig_used = *pused;
    const uint32_t pc_offset  = ((unsigned long)&(pM->reg.pc)   - (unsigned long)pM);

    const uint32_t hi_offset  = ((unsigned long)&(pM->reg.hi)   - (unsigned long)pM);
    const uint32_t lo_offset  = ((unsigned long)&(pM->reg.lo)   - (unsigned long)pM);

    const uint32_t ll_offset  = ((unsigned long)&(pM->reg.ll_sc)- (unsigned long)pM);

    uint8_t  op  = ((inst>>26) & 0x3f); /* inst[31:26] */
    uint8_t  rs  = ((inst>>21) & 0x1f); /* inst[25:21] */
    uint8_t  rt  = ((inst>>16) & 0x1f); /* inst[20:16] */
    uint8_t  rd  = ((inst>>11) & 0x1f); /* inst[15:11] */
    uint8_t shamt= ((inst>> 6) & 0x1f); /* inst[10: 6] */
    uint8_t funct= ( inst      & 0x3f); /* inst[ 0: 5] */
    uint16_t imm = ( inst      & 0xffff); /* inst[15:0] */

    switch( op ){
        case MIPS32_OP_SB:
            gen_store_insts(pM, mem, pused, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)sb_helper);
            break;

        case MIPS32_OP_SH:
            gen_store_insts(pM, mem, pused, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)sh_helper);
            break;

        case MIPS32_OP_LH:
            if( rt != 0 ){
                gen_load_insts(pM, mem, pused, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)lh_helper);
            }
            break;

        case MIPS32_OP_LHU:
            if( rt != 0 ){
                gen_load_insts(pM, mem, pused, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)lhu_helper);
            }
            break;

        case MIPS32_OP_LB:
            if( rt != 0 ){
                gen_load_insts(pM, mem, pused, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)lb_helper);
            }
            break;

        case MIPS32_OP_LBU: 
            if( rt != 0 ){
                gen_load_insts(pM, mem, pused, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)lbu_helper);
            }
            break;

        case MIPS32_OP_LW: // lw
            if( rt != 0 ){
                gen_load_insts(pM, mem, pused, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)lw_helper);
            }
            break;

        case MIPS32_OP_LL: // ll
            gen_xor_reg_reg          (mem, pused, REG_NUM_AX, REG_NUM_AX);
            gen_mov_imm8_to_reg8     (mem, pused, REG_NUM_AX,         1 );
            gen_mov_reg_to_idx32_base(mem, pused, REG_NUM_AX,  ll_offset, REG_NUM_DI);
            if( rt != 0 ){
                gen_load_insts(pM, mem, pused, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)lw_helper);
            }
            break;

        case MIPS32_OP_SC: // sc
            gen_store_insts(pM, mem, pused, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)sc_helper);
            
            if( rt != 0 ){
                gen_mov_idx32_base_to_reg(mem, pused, REG_NUM_AX,  ll_offset, REG_NUM_DI);
                gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX,       rt*4, REG_NUM_SI);
            }
            break;

        case MIPS32_OP_SW: // sw
            gen_store_insts(pM, mem, pused, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)sw_helper);
            break;

        case MIPS32_OP_REGIMM:
            switch( rt ){
                case 0x0c: // teqi
                    gen_mov_sidx8_base_to_reg  (mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                    gen_cmp_reg_imm32          (mem, pused, REG_NUM_AX, SIGN_EXT16(imm));
                    gen_jnz_rel_pc             (mem, pused, 2+10+1);
                    gen_xor_reg_reg            (mem, pused, REG_NUM_AX, REG_NUM_AX);
                    gen_mov_imm32_to_idx32_base(mem, pused, pc_offset, REG_NUM_DI, pointer);
                    gen_ret                    (mem, pused);
                    break;
                case 0x0e: // tnei
                    gen_mov_sidx8_base_to_reg  (mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                    gen_cmp_reg_imm32          (mem, pused, REG_NUM_AX, SIGN_EXT16(imm));
                    gen_jz_rel_pc              (mem, pused, 2+10+1);
                    gen_xor_reg_reg            (mem, pused, REG_NUM_AX, REG_NUM_AX);
                    gen_mov_imm32_to_idx32_base(mem, pused, pc_offset, REG_NUM_DI, pointer);
                    gen_ret                    (mem, pused);
                    break;
                default:
                    return -1;
            }
            break;

        case MIPS32_OP_SPECIAL:
            switch( funct ){
                case 0x21: // addu
                    if(rd != 0){
                        gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                        gen_add_reg_sidx8_base   (mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
                        gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rd*4, REG_NUM_SI);
                    }
                    break;
                case 0x23: // subu
                    if(rd != 0){
                        gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                        gen_sub_reg_sidx8_base   (mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
                        gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rd*4, REG_NUM_SI);
                    }
                    break;
                case 0x24: // and
                    if(rd != 0){
                        gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                        gen_and_reg_sidx8_base   (mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
                        gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rd*4, REG_NUM_SI);
                    }
                    break;
                case 0x27: // nor
                    if(rd != 0){
                        gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                        gen_or_reg_sidx8_base    (mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
                        gen_not_reg              (mem, pused, REG_NUM_AX);
                        gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rd*4, REG_NUM_SI);
                    }
                    break;
                case 0x25: // or
                    if(rd != 0){
                        gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                        gen_or_reg_sidx8_base    (mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
                        gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rd*4, REG_NUM_SI);
                    }
                    break;
                case 0x26: // xor
                    if(rd != 0){
                        gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                        gen_xor_reg_sidx8_base   (mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
                        gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rd*4, REG_NUM_SI);
                    }
                    break;
                case 0x2a: // slt
                    if(rd != 0){
                        gen_xor_reg_reg          (mem, pused, REG_NUM_AX, REG_NUM_AX);
                        gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_DX, rs*4, REG_NUM_SI);
                        gen_cmp_reg_sidx8_base   (mem, pused, REG_NUM_DX, rt*4, REG_NUM_SI);
                        gen_setl_reg             (mem, pused, REG_NUM_AX);
                        gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rd*4, REG_NUM_SI);
                    }
                    break;
                case 0x2b: // sltu
                    if(rd != 0){
                        gen_xor_reg_reg          (mem, pused, REG_NUM_AX, REG_NUM_AX);
                        gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_DX, rs*4, REG_NUM_SI);
                        gen_cmp_reg_sidx8_base   (mem, pused, REG_NUM_DX, rt*4, REG_NUM_SI);
                        gen_setb_reg             (mem, pused, REG_NUM_AX);
                        gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rd*4, REG_NUM_SI);
                    }
                    break;
                case 0x0a: // movz
                    if(rd != 0){
                        // CMOVENE https://mudongliang.github.io/x86/html/file_module_x86_id_34.html
                        gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_DX, rd*4, REG_NUM_SI);
                        gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_CX, rt*4, REG_NUM_SI);
                        gen_test_reg_reg         (mem, pused, REG_NUM_CX, REG_NUM_CX); // if cx=0, zf will be 1.
                        gen_cmove_reg_sidx8_base (mem, pused, REG_NUM_DX, rs*4, REG_NUM_SI);
                        gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_DX, rd*4, REG_NUM_SI);
                    }
                    break;
                case 0x0b: // movn
                    if(rd != 0){
                        gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_DX, rd*4, REG_NUM_SI);
                        gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_CX, rt*4, REG_NUM_SI);
                        gen_test_reg_reg         (mem, pused, REG_NUM_CX, REG_NUM_CX); // if cx=0, zf will be 1.
                        gen_cmovne_reg_sidx8_base(mem, pused, REG_NUM_DX, rs*4, REG_NUM_SI);
                        gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_DX, rd*4, REG_NUM_SI);
                    }
                    break;
                case 0x00: // sll
                    if( rs != 0x00 ){
                        return -1;
                    }
                    if(rd != 0){
                        gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
                        if( shamt != 0 ){
                            gen_sll_reg_imm8     (mem, pused, REG_NUM_AX, shamt);
                        }
                        gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rd*4, REG_NUM_SI);
                    }
                    break;
                case 0x02: // srl or rotr
                    if( rs == 0 ){ // srl
                        if(rd != 0){
                            gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
                            if( shamt != 0 ){
                                gen_srl_reg_imm8     (mem, pused, REG_NUM_AX, shamt);
                            }
                            gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rd*4, REG_NUM_SI);
                        }
                    }else if( rs == 1 ){ // rotr
                        if(rd != 0){
                            gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
                            if( shamt != 0 ){
                                gen_ror_reg_imm8     (mem, pused, REG_NUM_AX, shamt);
                            }
                            gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rd*4, REG_NUM_SI);
                        }
                    }
                    break;
                case 0x03: // sra
                    if(rd != 0){
                        gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
                        if( shamt != 0 ){
                            gen_sar_reg_imm8     (mem, pused, REG_NUM_AX, shamt);
                        }
                        gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rd*4, REG_NUM_SI);
                    }
                    break;
                case 0x04: // sllv
                    if(rd != 0){
                        gen_mov_imm8_to_reg8     (mem, pused, REG_NUM_CX, 0x1f);
                        gen_and_reg_sidx8_base   (mem, pused, REG_NUM_CX, rs*4, REG_NUM_SI);
                        gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
                        gen_sll_reg_cl           (mem, pused, REG_NUM_AX);
                        gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rd*4, REG_NUM_SI);
                    }
                    break;
                case 0x06: // srlv or rotrv
                    if(shamt == 0){ // srlv
                        if(rd != 0){
                            gen_mov_imm8_to_reg8     (mem, pused, REG_NUM_CX, 0x1f);
                            gen_and_reg_sidx8_base   (mem, pused, REG_NUM_CX, rs*4, REG_NUM_SI);
                            gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
                            gen_srl_reg_cl           (mem, pused, REG_NUM_AX);
                            gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rd*4, REG_NUM_SI);
                        }
                    }else{
                        return -1;
                    }
                    break;
                case 0x07: // srav
                    if( shamt != 0x00 ){
                        return -1;
                    }
                    if(rd != 0){
                        gen_mov_imm8_to_reg8     (mem, pused, REG_NUM_CX, 0x1f);
                        gen_and_reg_sidx8_base   (mem, pused, REG_NUM_CX, rs*4, REG_NUM_SI);
                        gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
                        gen_sar_reg_cl           (mem, pused, REG_NUM_AX);
                        gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rd*4, REG_NUM_SI);
                    }
                    break;
                case 0x10: // mfhi
                    if( rd != 0 ){
                        gen_mov_idx32_base_to_reg(mem, pused, REG_NUM_AX, hi_offset, REG_NUM_DI);
                        gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX,      rd*4, REG_NUM_SI);
                    }
                    break;
                case 0x11: // mthi
                    gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX,      rs*4, REG_NUM_SI);
                    gen_mov_reg_to_idx32_base(mem, pused, REG_NUM_AX, hi_offset, REG_NUM_DI);
                    break;
                case 0x12: // mflo
                    if( rd != 0 ){
                        gen_mov_idx32_base_to_reg(mem, pused, REG_NUM_AX, lo_offset, REG_NUM_DI);
                        gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX,      rd*4, REG_NUM_SI);
                    }
                    break;
                case 0x13: // mtlo
                    gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX,      rs*4, REG_NUM_SI);
                    gen_mov_reg_to_idx32_base(mem, pused, REG_NUM_AX, lo_offset, REG_NUM_DI);
                    break;
                case 0x18:
                    if( shamt == 0x00 && rd == 0x00 ){
                        return -1;
                    }
                    gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                    gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_CX, rt*4, REG_NUM_SI);
                    gen_imul                 (mem, pused, REG_NUM_CX); // signed mult
                    gen_mov_reg_to_idx32_base(mem, pused, REG_NUM_DX, hi_offset, REG_NUM_DI);
                    gen_mov_reg_to_idx32_base(mem, pused, REG_NUM_AX, lo_offset, REG_NUM_DI);
                    break;
                case 0x19:
                    if( shamt == 0x00 && rd == 0x00 ){
                        return -1;
                    }
                    gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                    gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_CX, rt*4, REG_NUM_SI);
                    gen_mul                  (mem, pused, REG_NUM_CX); // unsigned mult
                    gen_mov_reg_to_idx32_base(mem, pused, REG_NUM_DX, hi_offset, REG_NUM_DI);
                    gen_mov_reg_to_idx32_base(mem, pused, REG_NUM_AX, lo_offset, REG_NUM_DI);
                    break;
                case 0x1a: // div
                    gen_xor_reg_reg          (mem, pused, REG_NUM_DX, REG_NUM_DX);
                    gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                    gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_CX, rt*4, REG_NUM_SI);
                    gen_test_reg_reg         (mem, pused, REG_NUM_CX, REG_NUM_CX); // if cx=0, zf will be 1.
                    gen_jz_rel_pc            (mem, pused, 14);
                    gen_idiv                 (mem, pused, REG_NUM_CX); // signed division, 2 bytes
                    gen_mov_reg_to_idx32_base(mem, pused, REG_NUM_DX, hi_offset, REG_NUM_DI); // reminder, 6 bytes
                    gen_mov_reg_to_idx32_base(mem, pused, REG_NUM_AX, lo_offset, REG_NUM_DI); // quotient, 6 bytes
                    break;
                case 0x1b: // divu
                    gen_xor_reg_reg          (mem, pused, REG_NUM_DX, REG_NUM_DX);
                    gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                    gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_CX, rt*4, REG_NUM_SI);
                    gen_test_reg_reg         (mem, pused, REG_NUM_CX, REG_NUM_CX); // if cx=0, zf will be 1.
                    gen_jz_rel_pc            (mem, pused, 14);
                    gen_div                  (mem, pused, REG_NUM_CX); // unsigned division, 2 bytes
                    gen_mov_reg_to_idx32_base(mem, pused, REG_NUM_DX, hi_offset, REG_NUM_DI); // reminder, 6 bytes
                    gen_mov_reg_to_idx32_base(mem, pused, REG_NUM_AX, lo_offset, REG_NUM_DI); // quotient, 6 bytes
                    break;
                case 0x34: // teq (trap if equal)
                    gen_mov_sidx8_base_to_reg  (mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                    gen_cmp_reg_sidx8_base     (mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
                    gen_jnz_rel_pc             (mem, pused, 2+10+1);
                    gen_xor_reg_reg            (mem, pused, REG_NUM_AX, REG_NUM_AX);
                    gen_mov_imm32_to_idx32_base(mem, pused, pc_offset, REG_NUM_DI, pointer);
                    gen_ret                    (mem, pused);
                    break;
                case 0x36: // tne (trap if not equal)
                    gen_mov_sidx8_base_to_reg  (mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                    gen_cmp_reg_sidx8_base     (mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
                    gen_jz_rel_pc              (mem, pused, 2+10+1);
                    gen_xor_reg_reg            (mem, pused, REG_NUM_AX, REG_NUM_AX);
                    gen_mov_imm32_to_idx32_base(mem, pused, pc_offset, REG_NUM_DI, pointer);
                    gen_ret                    (mem, pused);
                    break;
                case 0x0f: // sync
                    // do nothing
                    break;
                default:
                    return -1;
            }
            break;
        case MIPS32_OP_SPECIAL2: // SPECIAL2
            if( funct == 0x02 && shamt == 0x00 ){
                if( rd != 0 ){ // mul 
                    gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                    gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_CX, rt*4, REG_NUM_SI);
                    gen_imul_reg_reg         (mem, pused, REG_NUM_AX, REG_NUM_CX);
                    gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rd*4, REG_NUM_SI);
                }
            }else{
                return -1;
            }
            break;
        case MIPS32_OP_SPECIAL3:
            switch (funct){
                case 0x00: // ext
                    if( rt !=0 ){
                        gen_mov_imm8_to_reg8     (mem, pused, REG_NUM_CX, shamt );
                        gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                        gen_srl_reg_cl           (mem, pused, REG_NUM_AX);
                        if( rd < 31 ){
                            gen_and_ax_imm32     (mem, pused, ((2<<rd)-1) );
                        }
                        gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
                    }
                    break;
                case 0x04: // ins
                    return -1;
                    break;
                case 0x20:
                    if( shamt == 0x10 ){ // seb
                        if(rd != 0){
                            gen_movsx_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
                            gen_mov_reg_to_sidx8_base  (mem, pused, REG_NUM_AX, rd*4, REG_NUM_SI);
                        }
                    }else if( shamt == 0x18 ){ // seh
                        // signed 16-bit -> signed 32-bit
                        if(rd != 0){
                            gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
                            gen_cwde                 (mem, pused);
                            gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rd*4, REG_NUM_SI);
                        }
                    }else if( shamt == 0x02 ){ // wsbh
                        if(rd != 0){
                            gen_mov_imm8_to_reg8     (mem, pused, REG_NUM_CX, 8);
                            gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
                            gen_mov_reg_to_reg       (mem, pused, REG_NUM_DX, REG_NUM_AX);
                            gen_and_reg_imm32        (mem, pused, REG_NUM_DX, 0xff00ff00);
                            gen_srl_reg_cl           (mem, pused, REG_NUM_DX);
                            gen_and_reg_imm32        (mem, pused, REG_NUM_AX, 0x00ff00ff);
                            gen_sll_reg_cl           (mem, pused, REG_NUM_AX);
                            gen_or_reg_reg           (mem, pused, REG_NUM_DX, REG_NUM_AX);
                            gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_DX, rd*4, REG_NUM_SI);
                        }
                    }else{
                        return -1;
                    }
                    break;
                default:
                    return -1;
            }
            break;
        case MIPS32_OP_LUI:
            if(rt != 0){
                gen_mov_imm32_to_sidx8_base(mem, pused, rt*4, REG_NUM_SI, (ZERO_EXT16(imm) << 16));
            }
            break;
        case MIPS32_OP_ADDIU:
            if(rt != 0){
                gen_mov_imm32_to_reg     (mem, pused, REG_NUM_AX, SIGN_EXT16(imm));
                gen_add_reg_sidx8_base   (mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
            }
            break;
        case MIPS32_OP_ORI:
            if(rt != 0){
                gen_mov_imm32_to_reg     (mem, pused, REG_NUM_AX, ZERO_EXT16(imm));
                gen_or_reg_sidx8_base    (mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
            }
            break;
        case MIPS32_OP_XORI:
            if(rt != 0){
                gen_mov_imm32_to_reg     (mem, pused, REG_NUM_AX, ZERO_EXT16(imm));
                gen_xor_reg_sidx8_base   (mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
            }
            break;
        case MIPS32_OP_ANDI:
            if(rt != 0){
                gen_mov_imm32_to_reg     (mem, pused, REG_NUM_AX, ZERO_EXT16(imm));
                gen_and_reg_sidx8_base   (mem, pused, REG_NUM_AX, rs*4, REG_NUM_SI);
                gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
            }
            break;
        case MIPS32_OP_SLTIU:
            if(rt != 0){
                gen_xor_reg_reg          (mem, pused, REG_NUM_AX, REG_NUM_AX);
                gen_cmp_sidx8_base_imm32 (mem, pused, rs*4, REG_NUM_SI, SIGN_EXT16(imm));
                gen_setb_reg             (mem, pused, REG_NUM_AX);
                gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
            }
            break;
        case MIPS32_OP_SLTI:
            if(rt != 0){
                gen_xor_reg_reg          (mem, pused, REG_NUM_AX, REG_NUM_AX);
                gen_cmp_sidx8_base_imm32 (mem, pused, rs*4, REG_NUM_SI, SIGN_EXT16(imm));
                gen_setl_reg             (mem, pused, REG_NUM_AX);
                gen_mov_reg_to_sidx8_base(mem, pused, REG_NUM_AX, rt*4, REG_NUM_SI);
            }
            break;
        case MIPS32_OP_CACHE:
        case MIPS32_OP_PREF:
            // no instruction needed
            break;
        default:
            return -1;
    }

    return (*pused) - orig_used;
}

// It is the main function of the code generator for MIPS32R2 instructions
int compile32(struct stMachineState *pM, uint8_t *imem, uintmx vaddr){
    int count = 0;
    int used = 0;

    uintmx pointer = vaddr;
    int memerror = 0;
    uint32_t inst = 0;

    uint32_t reg_offset = ((unsigned long)&(pM->reg.r[0]) - (unsigned long)pM);
    uint32_t pc_offset  = ((unsigned long)&(pM->reg.pc)   - (unsigned long)pM);
    uint32_t pcd_offset = ((unsigned long)&(pM->reg.pc_delay)-(unsigned long)pM);

    uint32_t hi_offset  = ((unsigned long)&(pM->reg.hi)   - (unsigned long)pM);
    uint32_t lo_offset  = ((unsigned long)&(pM->reg.lo)   - (unsigned long)pM);

    uint32_t ll_offset  = ((unsigned long)&(pM->reg.ll_sc)- (unsigned long)pM);

    uint8_t mem[RC_COMPILE_MAX_BIN_SIZE];

    uintmx paddr = getPhysicalAddress(pM, pointer, 0, &memerror);
    if( memerror ){ used=count=0; goto compile_finished; }

    // After executing instructions generated by the following lines, 
    // the content of RDI is the pointer of struct stMachineState (pM) provided as the argument of generated function,
    // the content of RSI is the pointer of the register file (pM->reg).
    gen_push_reg(mem, &used, REG_NUM_DI); // RDI (64bit) is pushed
    gen_pop_reg (mem, &used, REG_NUM_SI); // 64bit is poped and moved to RSI
    gen_prefix_rex64(mem, &used);         // The prefix to perform 64-bit operation
    gen_add_reg_imm32 (mem, &used, REG_NUM_SI, reg_offset); // 64-bit add

    uint32_t utmp;

    for( ; used < RC_COMPILE_MAX_BIN_SIZE - (128+12); count++){
        if( (pointer & 0xfff) == 0 ){
            paddr = getPhysicalAddress(pM, pointer, 0, &memerror);
            if( memerror ){ goto compile_finished; }
        }

        inst = loadPhysMemWord(pM, paddr);

        uint8_t  op  = ((inst>>26) & 0x3f); /* inst[31:26] */
        uint8_t  rs  = ((inst>>21) & 0x1f); /* inst[25:21] */
        uint8_t  rt  = ((inst>>16) & 0x1f); /* inst[20:16] */
        uint8_t  rd  = ((inst>>11) & 0x1f); /* inst[15:11] */
        uint8_t shamt= ((inst>> 6) & 0x1f); /* inst[10: 6] */
        uint8_t funct= ( inst      & 0x3f); /* inst[ 0: 5] */
        uint16_t imm = ( inst      & 0xffff); /* inst[15:0] */

        switch( op ){
            case MIPS32_OP_SB:
                gen_store_insts(pM, mem, &used, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)sb_helper);
                break;

            case MIPS32_OP_SH:
                gen_store_insts(pM, mem, &used, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)sh_helper);
                break;

            case MIPS32_OP_LH:
                if( rt != 0 ){
                    gen_load_insts(pM, mem, &used, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)lh_helper);
                }
                break;

            case MIPS32_OP_LHU:
                if( rt != 0 ){
                    gen_load_insts(pM, mem, &used, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)lhu_helper);
                }
                break;

            case MIPS32_OP_LB:
                if( rt != 0 ){
                    gen_load_insts(pM, mem, &used, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)lb_helper);
                }
                break;

            case MIPS32_OP_LBU: 
                if( rt != 0 ){
                    gen_load_insts(pM, mem, &used, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)lbu_helper);
                }
                break;

            case MIPS32_OP_LW: // lw
                if( rt != 0 ){
                    gen_load_insts(pM, mem, &used, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)lw_helper);
                }
                break;

            case MIPS32_OP_LL: // ll
                gen_mov_imm32_to_idx32_base(mem, &used, ll_offset, REG_NUM_DI, 1);
                if( rt != 0 ){
                    gen_load_insts(pM, mem, &used, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)lw_helper);
                }
                break;

            case MIPS32_OP_SC: // sc
                gen_store_insts(pM, mem, &used, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)sc_helper);
                
                if( rt != 0 ){
                    gen_mov_idx32_base_to_reg(mem, &used, REG_NUM_AX,  ll_offset, REG_NUM_DI);
                    gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX,       rt*4, REG_NUM_SI);
                }
                break;

            case MIPS32_OP_SW: // sw
                gen_store_insts(pM, mem, &used, imem, pointer, pc_offset, rt, rs, SIGN_EXT16(imm), (uint64_t)sw_helper);
                break;

            case MIPS32_OP_J:
                if( ((pointer+4)&0xfff) != 0 ){
                    int ngenbyte =genCode32(pM,imem, mem, &used, pointer, loadPhysMemWord(pM, paddr+4)); // code generation for branch slot
                    if( ngenbyte >= 0 ){
                        gen_mov_imm8_to_reg8(mem, &used, REG_NUM_AX, 1);
                        uint32_t jumpaddr      = (inst & 0x03ffffff);
                        jumpaddr = ((pointer+4) & ((~((uintmx)0x03ffffff))<<2)) + (jumpaddr<<2);
                        gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, jumpaddr);
                        gen_ret                    (mem, &used);
                        count ++;
                        goto compile_finished;
                    }
                }
                goto compile_finished;
                break;

            case MIPS32_OP_JAL:
                if( ((pointer+4)&0xfff) != 0 ){
                    uint32_t old_used = used;

                    // RD should be modified before delay slot.
                    gen_mov_imm32_to_sidx8_base(mem, &used,      31*4, REG_NUM_SI, pointer+8); //r[31] <- PC+8
                    
                    int ngenbyte =genCode32(pM,imem, mem, &used, pointer, loadPhysMemWord(pM, paddr+4)); // code generation for branch slot
                    if( ngenbyte >= 0 ){
                        gen_mov_imm8_to_reg8(mem, &used, REG_NUM_AX, 1);
                        uint32_t jumpaddr      = (inst & 0x03ffffff);
                        jumpaddr = ((pointer+4) & ((~((uintmx)0x03ffffff))<<2)) + (jumpaddr<<2);
                        gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, jumpaddr);
                        gen_ret                    (mem, &used);
                        count ++;
                        goto compile_finished;
                    }else{
                        used = old_used;
                    }
                }
                goto compile_finished;
                break;

            case MIPS32_OP_JALX:
                if( ((pointer+4)&0xfff) != 0 ){
                    uint32_t old_used = used;

                    // RD should be modified before delay slot.
                    gen_mov_imm32_to_sidx8_base(mem, &used,      31*4, REG_NUM_SI, pointer+8); //r[31] <- PC+8

                    int ngenbyte =genCode32(pM,imem, mem, &used, pointer, loadPhysMemWord(pM, paddr+4)); // code generation for branch slot
                    if( ngenbyte >= 0 ){
                        gen_mov_imm8_to_reg8(mem, &used, REG_NUM_AX, 1);
                        uint32_t jumpaddr      = (inst & 0x03ffffff);
                        jumpaddr = ((pointer+4) & ((~((uintmx)0x03ffffff))<<2)) + (jumpaddr<<2) + 1;
                        gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, jumpaddr);
                        gen_ret                    (mem, &used);
                        count ++;
                        goto compile_finished;
                    }else{
                        used = old_used;
                    }
                }
                goto compile_finished;
                break;
                
            case MIPS32_OP_BNE:
            case MIPS32_OP_BNEL:
                if( rt == 0 ){
                    gen_xor_reg_reg            (mem, &used, REG_NUM_AX, REG_NUM_AX);
                }else{
                    gen_mov_sidx8_base_to_reg  (mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                }
                if( rs == 0 ){
                    gen_cmp_reg_sign_ext_imm8  (mem, &used, REG_NUM_AX, 0);
                }else{
                    gen_cmp_reg_sidx8_base     (mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                }
                gen_jz_rel_pc              (mem, &used, 2+10+1);
                if( ((pointer+4)&0xfff) != 0 ){
                    int rel = used - 1;
                    int ngenbyte =genCode32(pM,imem, mem, &used, pointer, loadPhysMemWord(pM, paddr+4)); // code generation for branch slot
                    if( ngenbyte < 0 ){
                        // Execute branch with c code
                        gen_xor_reg_reg    (mem, &used, REG_NUM_AX, REG_NUM_AX);
                        utmp = pointer;
                    }else{
                        mem[rel] += ngenbyte;
                        gen_mov_imm8_to_reg8(mem, &used, REG_NUM_AX, 1);
                        utmp = pointer + (SIGN_EXT16(imm) << 2) + 4;
                    }
               }else{
                    gen_xor_reg_reg        (mem, &used, REG_NUM_AX, REG_NUM_AX);
                    utmp = pointer;
                }
                gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, utmp);
                gen_ret                    (mem, &used);
                if( op == MIPS32_OP_BNEL ){
                    pointer += 4;
                    paddr   += 4;
                    if( (pointer&0xfff) == 0 ){
                        paddr = getPhysicalAddress(pM, pointer, 0, &memerror);
                        if( memerror ){
                            pointer += 4;
                            paddr   += 4;
                            goto compile_finished;
                        }
                    }
                }
                break;

            case MIPS32_OP_BEQ:
            case MIPS32_OP_BEQL:
                if( rt == 0 ){
                    gen_xor_reg_reg            (mem, &used, REG_NUM_AX, REG_NUM_AX);
                }else{
                    gen_mov_sidx8_base_to_reg  (mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                }
                if( rs == 0 ){
                    gen_cmp_reg_sign_ext_imm8  (mem, &used, REG_NUM_AX, 0);
                }else{
                    gen_cmp_reg_sidx8_base     (mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                }
                gen_jnz_rel_pc             (mem, &used, 2+10+1);
                if( ((pointer+4)&0xfff) != 0 ){
                    int rel = used - 1;
                    int ngenbyte =genCode32(pM,imem, mem, &used, pointer, loadPhysMemWord(pM, paddr+4)); // code generation for branch slot
                    if( ngenbyte < 0 ){
                        // Execute branch with c code
                        gen_xor_reg_reg    (mem, &used, REG_NUM_AX, REG_NUM_AX);
                        utmp = pointer;
                    }else{
                        mem[rel] += ngenbyte;
                        gen_mov_imm8_to_reg8(mem, &used, REG_NUM_AX, 1);
                        utmp = pointer + (SIGN_EXT16(imm) << 2) + 4;
                    }
               }else{
                    gen_xor_reg_reg        (mem, &used, REG_NUM_AX, REG_NUM_AX);
                    utmp = pointer;
                }
                gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, utmp);
                gen_ret                    (mem, &used);
                if( op == MIPS32_OP_BEQL ){
                    pointer += 4;
                    paddr   += 4;
                    if( (pointer&0xfff) == 0 ){
                        paddr = getPhysicalAddress(pM, pointer, 0, &memerror);
                        if( memerror ){
                            pointer += 4;
                            paddr   += 4;
                            goto compile_finished;
                        }
                    }
                }
                break;

            case MIPS32_OP_BGTZ:
            case MIPS32_OP_BGTZL:
                gen_mov_sidx8_base_to_reg  (mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                gen_cmp_reg_sign_ext_imm8  (mem, &used, REG_NUM_AX, 0);
                gen_jle_rel_pc             (mem, &used, 2+10+1);
                if( ((pointer+4)&0xfff) != 0 ){
                    int rel = used - 1;
                    int ngenbyte =genCode32(pM,imem, mem, &used, pointer, loadPhysMemWord(pM, paddr+4)); // code generation for branch slot
                    if( ngenbyte < 0 ){
                        // Execute branch with c code
                        gen_xor_reg_reg    (mem, &used, REG_NUM_AX, REG_NUM_AX);
                        utmp = pointer;
                    }else{
                        mem[rel] += ngenbyte;
                        gen_mov_imm8_to_reg8(mem, &used, REG_NUM_AX, 1);
                        utmp = pointer + (SIGN_EXT16(imm) << 2) + 4;
                    }
               }else{
                    gen_xor_reg_reg        (mem, &used, REG_NUM_AX, REG_NUM_AX);
                    utmp = pointer;
                }
                gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, utmp);
                gen_ret                    (mem, &used);
                if( op == MIPS32_OP_BGTZL ){
                    pointer += 4;
                    paddr   += 4;
                    if( (pointer&0xfff) == 0 ){
                        paddr = getPhysicalAddress(pM, pointer, 0, &memerror);
                        if( memerror ){
                            pointer += 4;
                            paddr   += 4;
                            goto compile_finished;
                        }
                    }
                }
                break;

            case MIPS32_OP_BLEZ:
            case MIPS32_OP_BLEZL:
                gen_mov_sidx8_base_to_reg  (mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                gen_cmp_reg_sign_ext_imm8  (mem, &used, REG_NUM_AX, 0);
                gen_jg_rel_pc              (mem, &used, 2+10+1);
                if( ((pointer+4)&0xfff) != 0 ){
                    int rel = used - 1;
                    int ngenbyte =genCode32(pM,imem, mem, &used, pointer, loadPhysMemWord(pM, paddr+4)); // code generation for branch slot
                    if( ngenbyte < 0 ){
                        // Execute branch with c code
                        gen_xor_reg_reg    (mem, &used, REG_NUM_AX, REG_NUM_AX);
                        utmp = pointer;
                    }else{
                        mem[rel] += ngenbyte;
                        gen_mov_imm8_to_reg8(mem, &used, REG_NUM_AX, 1);
                        utmp = pointer + (SIGN_EXT16(imm) << 2) + 4;
                    }
               }else{
                    gen_xor_reg_reg        (mem, &used, REG_NUM_AX, REG_NUM_AX);
                    utmp = pointer;
                }
                gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, utmp);
                gen_ret                    (mem, &used);
                if( op == MIPS32_OP_BLEZL ){
                    pointer += 4;
                    paddr   += 4;
                    if( (pointer&0xfff) == 0 ){
                        paddr = getPhysicalAddress(pM, pointer, 0, &memerror);
                        if( memerror ){
                            pointer += 4;
                            paddr   += 4;
                            goto compile_finished;
                        }
                    }
                }
                break;

            case MIPS32_OP_REGIMM:
                switch( rt ){
                    case 0x11: // bal, bgezal
                        if( rs == 0 ){ // bal
                            if( ((pointer+4)&0xfff) != 0 ){
                                uint32_t old_used = used;
                                gen_mov_imm32_to_sidx8_base(mem, &used,      31*4, REG_NUM_SI, pointer+8); //r[31] <- PC+8

                                int ngenbyte =genCode32(pM,imem, mem, &used, pointer, loadPhysMemWord(pM, paddr+4)); // code generation for branch slot
                                if( ngenbyte >= 0 ){
                                    gen_mov_imm8_to_reg8(mem, &used, REG_NUM_AX, 1);
                                    utmp = pointer + (SIGN_EXT16(imm) << 2) + 4;
                                    gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, utmp);
                                    gen_ret                    (mem, &used);
                                    count ++;
                                    goto compile_finished;
                                }else{
                                    used = old_used;
                                }
                            }
                            goto compile_finished;
                        }else{ // bgezal
                            gen_mov_imm32_to_sidx8_base(mem, &used, 31*4, REG_NUM_SI, pointer+8); //r[31] <- PC+8
                            gen_mov_sidx8_base_to_reg  (mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                            gen_cmp_reg_sign_ext_imm8  (mem, &used, REG_NUM_AX, 0);
                            gen_jl_rel_pc              (mem, &used, 2+10+1);
                            if( ((pointer+4)&0xfff) != 0 ){
                                int rel = used - 1;
                                int ngenbyte =genCode32(pM,imem, mem, &used, pointer, loadPhysMemWord(pM, paddr+4)); // code generation for branch slot
                                if( ngenbyte < 0 ){
                                    // Execute branch with c code
                                    gen_xor_reg_reg    (mem, &used, REG_NUM_AX, REG_NUM_AX);
                                    utmp = pointer;
                                }else{
                                    mem[rel] += ngenbyte;
                                    gen_mov_imm8_to_reg8(mem, &used, REG_NUM_AX, 1);
                                    utmp = pointer + (SIGN_EXT16(imm) << 2) + 4;
                                }
                           }else{
                                gen_xor_reg_reg        (mem, &used, REG_NUM_AX, REG_NUM_AX);
                                utmp = pointer;
                            }
                            gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, utmp);
                            gen_ret                    (mem, &used);
                        }
                        break;
                    case 0x00: // bltz
                    case 0x02: // bltzl
                    case 0x10: // bltzal
                        if( rt == 0x10 ){
                            gen_mov_imm32_to_sidx8_base(mem, &used, 31*4, REG_NUM_SI, pointer+8); //r[31] <- PC+8
                        }
                        gen_mov_sidx8_base_to_reg  (mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                        gen_cmp_reg_sign_ext_imm8  (mem, &used, REG_NUM_AX, 0);
                        gen_jge_rel_pc             (mem, &used, 2+10+1);
                        if( ((pointer+4)&0xfff) != 0 ){
                            int rel = used - 1;
                            int ngenbyte =genCode32(pM,imem, mem, &used, pointer, loadPhysMemWord(pM, paddr+4)); // code generation for branch slot
                            if( ngenbyte < 0 ){
                                // Execute branch with c code
                                gen_xor_reg_reg    (mem, &used, REG_NUM_AX, REG_NUM_AX);
                                utmp = pointer;
                            }else{
                                mem[rel] += ngenbyte;
                                gen_mov_imm8_to_reg8(mem, &used, REG_NUM_AX, 1);
                                utmp = pointer + (SIGN_EXT16(imm) << 2) + 4;
                            }
                       }else{
                            gen_xor_reg_reg        (mem, &used, REG_NUM_AX, REG_NUM_AX);
                            utmp = pointer;
                        }
                        gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, utmp);
                        gen_ret                    (mem, &used);
                        if( rt == 0x02 ){
                            pointer += 4;
                            paddr   += 4;
                            if( (pointer&0xfff) == 0 ){
                                paddr = getPhysicalAddress(pM, pointer, 0, &memerror);
                                if( memerror ){
                                    pointer += 4;
                                    paddr   += 4;
                                    goto compile_finished;
                                }
                            }
                        }
                        break;
                    case 0x01: // bgez
                    case 0x03: // bgezl
                        gen_mov_sidx8_base_to_reg  (mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                        gen_cmp_reg_sign_ext_imm8  (mem, &used, REG_NUM_AX, 0);
                        gen_jl_rel_pc              (mem, &used, 2+10+1);
                        if( ((pointer+4)&0xfff) != 0 ){
                            int rel = used - 1;
                            int ngenbyte =genCode32(pM,imem, mem, &used, pointer, loadPhysMemWord(pM, paddr+4)); // code generation for branch slot
                            if( ngenbyte < 0 ){
                                // Execute branch with c code
                                gen_xor_reg_reg    (mem, &used, REG_NUM_AX, REG_NUM_AX);
                                utmp = pointer;
                            }else{
                                mem[rel] += ngenbyte;
                                gen_mov_imm8_to_reg8(mem, &used, REG_NUM_AX, 1);
                                utmp = pointer + (SIGN_EXT16(imm) << 2) + 4;
                            }
                       }else{
                            gen_xor_reg_reg        (mem, &used, REG_NUM_AX, REG_NUM_AX);
                            utmp = pointer;
                        }
                        gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, utmp);
                        gen_ret                    (mem, &used);
                        if( rt == 0x03 ){
                            pointer += 4;
                            paddr   += 4;
                            if( (pointer&0xfff) == 0 ){
                                paddr = getPhysicalAddress(pM, pointer, 0, &memerror);
                                if( memerror ){
                                    pointer += 4;
                                    paddr   += 4;
                                    goto compile_finished;
                                }
                            }
                        }
                        break;
                    case 0x0c: // teqi
                        gen_mov_sidx8_base_to_reg  (mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                        gen_cmp_reg_imm32          (mem, &used, REG_NUM_AX, SIGN_EXT16(imm));
                        gen_jnz_rel_pc             (mem, &used, 2+10+1);
                        gen_xor_reg_reg            (mem, &used, REG_NUM_AX, REG_NUM_AX);
                        gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, pointer);
                        gen_ret                    (mem, &used);
                        break;
                    case 0x0e: // tnei
                        gen_mov_sidx8_base_to_reg  (mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                        gen_cmp_reg_imm32          (mem, &used, REG_NUM_AX, SIGN_EXT16(imm));
                        gen_jz_rel_pc              (mem, &used, 2+10+1);
                        gen_xor_reg_reg            (mem, &used, REG_NUM_AX, REG_NUM_AX);
                        gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, pointer);
                        gen_ret                    (mem, &used);
                        break;
                    default:
                        goto compile_finished;
                }
                break;

            case MIPS32_OP_SPECIAL:
                switch( funct ){
                    case 0x08: // jr
                        if( (imm & (1<<10)) && rd == 0 && rt == 0 ){
                            // Hazard barrier vriant of JR instruction (jr.hb)
                            // This instruction should be done with C code
                        }else{
                            if( ((pointer+4)&0xfff) != 0 ){
                                int ngenbyte =genCode32(pM,imem, mem, &used, pointer, loadPhysMemWord(pM, paddr+4)); // code generation for branch slot
                                if( ngenbyte >= 0 ){
                                    gen_mov_sidx8_base_to_reg  (mem, &used, REG_NUM_AX,      rs*4, REG_NUM_SI);
                                    gen_mov_reg_to_idx32_base  (mem, &used, REG_NUM_AX, pc_offset, REG_NUM_DI);
                                    gen_mov_imm8_to_reg8       (mem, &used, REG_NUM_AX, 1);
                                    gen_ret                    (mem, &used);
                                    count ++;
                                }
                            }
                        }
                        goto compile_finished;

                    case 0x09: // jalr
                        if( ((pointer+4)&0xfff) != 0 ){
                            uint32_t old_used = used;

                            // RD should be modified before delay slot.
                            gen_mov_sidx8_base_to_reg  (mem, &used, REG_NUM_AX,      rs*4, REG_NUM_SI);
                            if( rd != 0 ){
                                // rd coubld be the same as rs 
                                // (note that the manual inhibits rs==rd for the concerns for restartability, but reading rs is performed at first in the pseudo code in the manual).
                                // Thus, before storing PC+8 to rd, loading rs is necessary.
                                gen_mov_imm32_to_sidx8_base(mem, &used, rd*4, REG_NUM_SI, pointer+8);
                            }
                            // reg.pc will be overwritten by code generation for load, store instruction.
                            // Thus, we store the jump address in the safe region (pc_delay).
                            gen_mov_reg_to_idx32_base  (mem, &used, REG_NUM_AX, pcd_offset, REG_NUM_DI);
                            
                            int ngenbyte =genCode32(pM,imem, mem, &used, pointer, loadPhysMemWord(pM, paddr+4)); // code generation for branch slot
                            if( ngenbyte >= 0 ){
                                gen_mov_idx32_base_to_reg  (mem, &used, REG_NUM_AX, pcd_offset, REG_NUM_DI);
                                gen_mov_reg_to_idx32_base  (mem, &used, REG_NUM_AX,  pc_offset, REG_NUM_DI);
                                gen_mov_imm8_to_reg8       (mem, &used, REG_NUM_AX, 1);
                                gen_ret                    (mem, &used);
                                count ++;
                            }else{
                                used = old_used;
                            }
                        }
                        goto compile_finished;
    
                    case 0x21: // addu
                        if(rd != 0){
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                            gen_add_reg_sidx8_base   (mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rd*4, REG_NUM_SI);
                        }
                        break;
                    case 0x23: // subu
                        if(rd != 0){
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                            gen_sub_reg_sidx8_base   (mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rd*4, REG_NUM_SI);
                        }
                        break;
                    case 0x24: // and
                        if(rd != 0){
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                            gen_and_reg_sidx8_base   (mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rd*4, REG_NUM_SI);
                        }
                        break;
                    case 0x27: // nor
                        if(rd != 0){
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                            gen_or_reg_sidx8_base    (mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                            gen_not_reg              (mem, &used, REG_NUM_AX);
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rd*4, REG_NUM_SI);
                        }
                        break;
                    case 0x25: // or
                        if(rd != 0){
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                            gen_or_reg_sidx8_base    (mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rd*4, REG_NUM_SI);
                        }
                        break;
                    case 0x26: // xor
                        if(rd != 0){
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                            gen_xor_reg_sidx8_base   (mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rd*4, REG_NUM_SI);
                        }
                        break;
                    case 0x2a: // slt
                        if(rd != 0){
                            gen_xor_reg_reg          (mem, &used, REG_NUM_AX, REG_NUM_AX);
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_DX, rs*4, REG_NUM_SI);
                            gen_cmp_reg_sidx8_base   (mem, &used, REG_NUM_DX, rt*4, REG_NUM_SI);
                            gen_setl_reg             (mem, &used, REG_NUM_AX);
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rd*4, REG_NUM_SI);
                        }
                        break;
                    case 0x2b: // sltu
                        if(rd != 0){
                            gen_xor_reg_reg          (mem, &used, REG_NUM_AX, REG_NUM_AX);
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_DX, rs*4, REG_NUM_SI);
                            gen_cmp_reg_sidx8_base   (mem, &used, REG_NUM_DX, rt*4, REG_NUM_SI);
                            gen_setb_reg             (mem, &used, REG_NUM_AX);
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rd*4, REG_NUM_SI);
                        }
                        break;
                    case 0x0a: // movz
                        if(rd != 0){
                            // CMOVENE https://mudongliang.github.io/x86/html/file_module_x86_id_34.html
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_DX, rd*4, REG_NUM_SI);
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_CX, rt*4, REG_NUM_SI);
                            gen_test_reg_reg         (mem, &used, REG_NUM_CX, REG_NUM_CX); // if cx=0, zf will be 1.
                            gen_cmove_reg_sidx8_base (mem, &used, REG_NUM_DX, rs*4, REG_NUM_SI);
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_DX, rd*4, REG_NUM_SI);
                        }
                        break;
                    case 0x0b: // movn
                        if(rd != 0){
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_DX, rd*4, REG_NUM_SI);
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_CX, rt*4, REG_NUM_SI);
                            gen_test_reg_reg         (mem, &used, REG_NUM_CX, REG_NUM_CX); // if cx=0, zf will be 1.
                            gen_cmovne_reg_sidx8_base(mem, &used, REG_NUM_DX, rs*4, REG_NUM_SI);
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_DX, rd*4, REG_NUM_SI);
                        }
                        break;
                    case 0x00: // sll
                        if( rs != 0x00 ){
                            goto compile_finished;
                        }
                        if(rd != 0){
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                            if( shamt != 0 ){
                                gen_sll_reg_imm8     (mem, &used, REG_NUM_AX, shamt);
                            }
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rd*4, REG_NUM_SI);
                        }
                        break;
                    case 0x02: // srl or rotr
                        if( rs == 0 ){ // srl
                            if(rd != 0){
                                gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                                if( shamt != 0 ){
                                    gen_srl_reg_imm8     (mem, &used, REG_NUM_AX, shamt);
                                }
                                gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rd*4, REG_NUM_SI);
                            }
                        }else if( rs == 1 ){ // rotr
                            if(rd != 0){
                                gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                                if( shamt != 0 ){
                                    gen_ror_reg_imm8     (mem, &used, REG_NUM_AX, shamt);
                                }
                                gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rd*4, REG_NUM_SI);
                            }
                        }
                        break;
                    case 0x03: // sra
                        if(rd != 0){
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                            if( shamt != 0 ){
                                gen_sar_reg_imm8     (mem, &used, REG_NUM_AX, shamt);
                            }
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rd*4, REG_NUM_SI);
                        }
                        break;
                    case 0x04: // sllv
                        if(rd != 0){
                            gen_mov_imm8_to_reg8     (mem, &used, REG_NUM_CX, 0x1f);
                            gen_and_reg_sidx8_base   (mem, &used, REG_NUM_CX, rs*4, REG_NUM_SI);
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                            gen_sll_reg_cl           (mem, &used, REG_NUM_AX);
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rd*4, REG_NUM_SI);
                        }
                        break;
                    case 0x06: // srlv or rotrv
                        if(shamt == 0){ // srlv
                            if(rd != 0){
                                gen_mov_imm8_to_reg8     (mem, &used, REG_NUM_CX, 0x1f);
                                gen_and_reg_sidx8_base   (mem, &used, REG_NUM_CX, rs*4, REG_NUM_SI);
                                gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                                gen_srl_reg_cl           (mem, &used, REG_NUM_AX);
                                gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rd*4, REG_NUM_SI);
                            }
                        }else{
                            goto compile_finished;
                        }
                        break;
                    case 0x07: // srav
                        if( shamt != 0x00 ){
                            goto compile_finished;
                        }
                        if(rd != 0){
                            gen_mov_imm8_to_reg8     (mem, &used, REG_NUM_CX, 0x1f);
                            gen_and_reg_sidx8_base   (mem, &used, REG_NUM_CX, rs*4, REG_NUM_SI);
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                            gen_sar_reg_cl           (mem, &used, REG_NUM_AX);
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rd*4, REG_NUM_SI);
                        }
                        break;
                    case 0x10: // mfhi
                        if( rd != 0 ){
                            gen_mov_idx32_base_to_reg(mem, &used, REG_NUM_AX, hi_offset, REG_NUM_DI);
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX,      rd*4, REG_NUM_SI);
                        }
                        break;
                    case 0x11: // mthi
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX,      rs*4, REG_NUM_SI);
                        gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_AX, hi_offset, REG_NUM_DI);
                        break;
                    case 0x12: // mflo
                        if( rd != 0 ){
                            gen_mov_idx32_base_to_reg(mem, &used, REG_NUM_AX, lo_offset, REG_NUM_DI);
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX,      rd*4, REG_NUM_SI);
                        }
                        break;
                    case 0x13: // mtlo
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX,      rs*4, REG_NUM_SI);
                        gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_AX, lo_offset, REG_NUM_DI);
                        break;
                    case 0x18:
                        if( shamt == 0x00 && rd == 0x00 ){
                            goto compile_finished;
                        }
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_CX, rt*4, REG_NUM_SI);
                        gen_imul                 (mem, &used, REG_NUM_CX); // signed mult
                        gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_DX, hi_offset, REG_NUM_DI);
                        gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_AX, lo_offset, REG_NUM_DI);
                        break;
                    case 0x19:
                        if( shamt == 0x00 && rd == 0x00 ){
                            goto compile_finished;
                        }
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_CX, rt*4, REG_NUM_SI);
                        gen_mul                  (mem, &used, REG_NUM_CX); // unsigned mult
                        gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_DX, hi_offset, REG_NUM_DI);
                        gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_AX, lo_offset, REG_NUM_DI);
                        break;
                    case 0x1a: // div
                        gen_xor_reg_reg          (mem, &used, REG_NUM_DX, REG_NUM_DX);
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_CX, rt*4, REG_NUM_SI);
                        gen_test_reg_reg         (mem, &used, REG_NUM_CX, REG_NUM_CX); // if cx=0, zf will be 1.
                        gen_jz_rel_pc            (mem, &used, 14);
                        gen_idiv                 (mem, &used, REG_NUM_CX); // signed division, 2 bytes
                        gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_DX, hi_offset, REG_NUM_DI); // reminder, 6 bytes
                        gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_AX, lo_offset, REG_NUM_DI); // quotient, 6 bytes
                        break;
                    case 0x1b: // divu
                        gen_xor_reg_reg          (mem, &used, REG_NUM_DX, REG_NUM_DX);
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_CX, rt*4, REG_NUM_SI);
                        gen_test_reg_reg         (mem, &used, REG_NUM_CX, REG_NUM_CX); // if cx=0, zf will be 1.
                        gen_jz_rel_pc            (mem, &used, 14);
                        gen_div                  (mem, &used, REG_NUM_CX); // unsigned division, 2 bytes
                        gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_DX, hi_offset, REG_NUM_DI); // reminder, 6 bytes
                        gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_AX, lo_offset, REG_NUM_DI); // quotient, 6 bytes
                        break;
                    case 0x34: // teq (trap if equal)
                        gen_mov_sidx8_base_to_reg  (mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                        gen_cmp_reg_sidx8_base     (mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                        gen_jnz_rel_pc             (mem, &used, 2+10+1);
                        gen_xor_reg_reg            (mem, &used, REG_NUM_AX, REG_NUM_AX);
                        gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, pointer);
                        gen_ret                    (mem, &used);
                        break;
                    case 0x36: // tne (trap if not equal)
                        gen_mov_sidx8_base_to_reg  (mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                        gen_cmp_reg_sidx8_base     (mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                        gen_jz_rel_pc              (mem, &used, 2+10+1);
                        gen_xor_reg_reg            (mem, &used, REG_NUM_AX, REG_NUM_AX);
                        gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, pointer);
                        gen_ret                    (mem, &used);
                        break;
                    case 0x0f: // sync
                        // do nothing
                        break;
                    default:
                        goto compile_finished;
                }
                break;
            case MIPS32_OP_SPECIAL2: // SPECIAL2
                switch( imm & 0x7ff  ){ // check 10:0
                    case 0x02:
                        if( rd != 0 ){ // mul 
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_CX, rt*4, REG_NUM_SI);
                            gen_imul_reg_reg         (mem, &used, REG_NUM_AX, REG_NUM_CX);
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rd*4, REG_NUM_SI);
                        }
                        break;
                    case 0x00:
                        if( rd == 0 ){ // madd
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_CX, rt*4, REG_NUM_SI);
                            gen_imul                 (mem, &used, REG_NUM_CX); // signed multiplication
                            gen_mov_idx32_base_to_reg(mem, &used, REG_NUM_CX, lo_offset, REG_NUM_DI);
                            gen_add_reg_reg          (mem, &used, REG_NUM_AX, REG_NUM_CX);
                            gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_AX, lo_offset, REG_NUM_DI);
                            gen_mov_idx32_base_to_reg(mem, &used, REG_NUM_CX, hi_offset, REG_NUM_DI);
                            gen_adc_reg_reg          (mem, &used, REG_NUM_DX, REG_NUM_CX);
                            gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_DX, hi_offset, REG_NUM_DI);
                        }else{
                            goto compile_finished;
                        }
                        break;
                    case 0x01:
                        if( rd == 0 ){ // maddu
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_CX, rt*4, REG_NUM_SI);
                            gen_mul                  (mem, &used, REG_NUM_CX); // unsigned multiplication
                            gen_mov_idx32_base_to_reg(mem, &used, REG_NUM_CX, lo_offset, REG_NUM_DI);
                            gen_add_reg_reg          (mem, &used, REG_NUM_AX, REG_NUM_CX);
                            gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_AX, lo_offset, REG_NUM_DI);
                            gen_mov_idx32_base_to_reg(mem, &used, REG_NUM_CX, hi_offset, REG_NUM_DI);
                            gen_adc_reg_reg          (mem, &used, REG_NUM_DX, REG_NUM_CX);
                            gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_DX, hi_offset, REG_NUM_DI);
                        }else{
                            goto compile_finished;
                        }
                        break;
                    case 0x21: // clo (count leading ones)
                        if( rd != 0 ){ // clo
                            gen_mov_imm32_to_reg     (mem, &used, REG_NUM_CX, 32);
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                            gen_not_reg              (mem, &used, REG_NUM_AX);             // bit-wise not (after this, the processing is the same as clz)
                            gen_bsr_reg_reg          (mem, &used, REG_NUM_AX, REG_NUM_AX);
                            gen_jz_rel_pc            (mem, &used, 4);
                            gen_inc_reg              (mem, &used, REG_NUM_AX);             // 2 bytes
                            gen_sub_reg_reg          (mem, &used, REG_NUM_CX, REG_NUM_AX); // 2 bytes
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_CX, rd*4, REG_NUM_SI);
                        }
                        break;
                    case 0x20: // clz (count leading zeros)
                        if( rd != 0 ){ // clz
                            gen_mov_imm32_to_reg     (mem, &used, REG_NUM_CX, 32);
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                            gen_bsr_reg_reg          (mem, &used, REG_NUM_AX, REG_NUM_AX); // it return the most significant position of "1"
                            gen_jz_rel_pc            (mem, &used, 4);
                            gen_inc_reg              (mem, &used, REG_NUM_AX);             // 2 bytes
                            gen_sub_reg_reg          (mem, &used, REG_NUM_CX, REG_NUM_AX); // 2 bytes
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_CX, rd*4, REG_NUM_SI);
                        }
                        break;
                    default:
                        goto compile_finished;
                }
                break;
            case MIPS32_OP_SPECIAL3:
                switch (funct){
                    case 0x00: // ext
                        if( rt !=0 ){
                            gen_mov_imm8_to_reg8     (mem, &used, REG_NUM_CX, shamt );
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                            gen_srl_reg_cl           (mem, &used, REG_NUM_AX);
                            if( rd < 31 ){
                                gen_and_ax_imm32     (mem, &used, ((2<<rd)-1) );
                            }
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                        }
                        break;
                    case 0x04: // ins
                        if( rt != 0 ){
                            utmp = (1<<((rd+1)-shamt)) -1;
                            utmp = (utmp << shamt);
                            if( shamt==0 && rd==31 ){
                                // pM->reg.r[rt] = pM->reg.r[rs];

                                gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                                gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                            }else{
                                // pM->reg.r[rt] = ( (pM->reg.r[rt] & (~utmp)) | ((pM->reg.r[rs] << shamt) & utmp) );

                                gen_mov_imm32_to_reg     (mem, &used, REG_NUM_CX, ~utmp);
                                gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_DX, rt*4, REG_NUM_SI);
                                gen_and_reg_reg          (mem, &used, REG_NUM_DX, REG_NUM_CX);

                                gen_not_reg              (mem, &used, REG_NUM_CX);
                                gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                                gen_sll_reg_imm8         (mem, &used, REG_NUM_AX, shamt);
                                gen_and_reg_reg          (mem, &used, REG_NUM_AX, REG_NUM_CX);

                                gen_or_reg_reg           (mem, &used, REG_NUM_AX, REG_NUM_DX);
                                gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                            }
                        }
                        break;
                    case 0x20:
                        if( shamt == 0x10 ){ // seb
                            if(rd != 0){
                                gen_movsx_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                                gen_mov_reg_to_sidx8_base  (mem, &used, REG_NUM_AX, rd*4, REG_NUM_SI);
                            }
                        }else if( shamt == 0x18 ){ // seh
                            // signed 16-bit -> signed 32-bit
                            if(rd != 0){
                                gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                                gen_cwde                 (mem, &used);
                                gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rd*4, REG_NUM_SI);
                            }
                        }else if( shamt == 0x02 ){ // wsbh
                            if(rd != 0){
                                gen_mov_imm8_to_reg8     (mem, &used, REG_NUM_CX, 8);
                                gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                                gen_mov_reg_to_reg       (mem, &used, REG_NUM_DX, REG_NUM_AX);
                                gen_and_reg_imm32        (mem, &used, REG_NUM_DX, 0xff00ff00);
                                gen_srl_reg_cl           (mem, &used, REG_NUM_DX);
                                gen_and_reg_imm32        (mem, &used, REG_NUM_AX, 0x00ff00ff);
                                gen_sll_reg_cl           (mem, &used, REG_NUM_AX);
                                gen_or_reg_reg           (mem, &used, REG_NUM_DX, REG_NUM_AX);
                                gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_DX, rd*4, REG_NUM_SI);
                            }
                        }else{
                            goto compile_finished;
                        }
                        break;
                    default:
                        goto compile_finished;
                }
                break;
            case MIPS32_OP_LUI:
                if(rt != 0){
                    gen_mov_imm32_to_sidx8_base(mem, &used, rt*4, REG_NUM_SI, (ZERO_EXT16(imm) << 16));
                }
                break;
            case MIPS32_OP_ADDIU:
                if(rt != 0){
                    gen_mov_imm32_to_reg     (mem, &used, REG_NUM_AX, SIGN_EXT16(imm));
                    gen_add_reg_sidx8_base   (mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                    gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                }
                break;
            case MIPS32_OP_ORI:
                if(rt != 0){
                    gen_mov_imm32_to_reg     (mem, &used, REG_NUM_AX, ZERO_EXT16(imm));
                    gen_or_reg_sidx8_base    (mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                    gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                }
                break;
            case MIPS32_OP_XORI:
                if(rt != 0){
                    gen_mov_imm32_to_reg     (mem, &used, REG_NUM_AX, ZERO_EXT16(imm));
                    gen_xor_reg_sidx8_base   (mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                    gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                }
                break;
            case MIPS32_OP_ANDI:
                if(rt != 0){
                    gen_mov_imm32_to_reg     (mem, &used, REG_NUM_AX, ZERO_EXT16(imm));
                    gen_and_reg_sidx8_base   (mem, &used, REG_NUM_AX, rs*4, REG_NUM_SI);
                    gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                }
                break;
            case MIPS32_OP_SLTIU:
                if(rt != 0){
                    gen_xor_reg_reg          (mem, &used, REG_NUM_AX, REG_NUM_AX);
                    gen_cmp_sidx8_base_imm32 (mem, &used, rs*4, REG_NUM_SI, SIGN_EXT16(imm));
                    gen_setb_reg             (mem, &used, REG_NUM_AX);
                    gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                }
                break;
            case MIPS32_OP_SLTI:
                if(rt != 0){
                    gen_xor_reg_reg          (mem, &used, REG_NUM_AX, REG_NUM_AX);
                    gen_cmp_sidx8_base_imm32 (mem, &used, rs*4, REG_NUM_SI, SIGN_EXT16(imm));
                    gen_setl_reg             (mem, &used, REG_NUM_AX);
                    gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, rt*4, REG_NUM_SI);
                }
                break;
            case MIPS32_OP_CACHE:
            case MIPS32_OP_PREF:
                // no instruction needed
                break;
            default:
                goto compile_finished;
        }

        pointer +=4;
        paddr   +=4;
    }

compile_finished:
    if(count > 0){
        gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, pointer);
    }
    gen_xor_reg_reg(mem, &used, REG_NUM_AX, REG_NUM_AX);
    gen_ret(mem, &used);

    if( count >= 4 ){
        memcpy(imem, mem, used);

        /* serialize data and instructions */
        asm volatile ("mfence" ::: "memory");
        asm volatile (
            "xorl %%eax,%%eax;"
            "cpuid" ::: "%eax", "%ebx", "%ecx", "%edx");
    }else{
        return 0;
    }

    return count;
}
