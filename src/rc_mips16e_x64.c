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

#include "ExInst_common.h"


#define WITH_PREXIX(i)   ((i) & 0xffff0000)

int compile16(struct stMachineState *pM, uint8_t *imem, uintmx vaddr){
    int count = 0;
    int used = 0;

    uintmx pointer = (vaddr & ~1);
    int memerror;
    uint32_t inst32 = 0;

    uint32_t reg_offset = ((unsigned long)&(pM->reg.r[0]) - (unsigned long)pM);
    uint32_t pc_offset  = ((unsigned long)&(pM->reg.pc)   - (unsigned long)pM);

    uint32_t hi_offset  = ((unsigned long)&(pM->reg.hi)   - (unsigned long)pM);
    uint32_t lo_offset  = ((unsigned long)&(pM->reg.lo)   - (unsigned long)pM);

    uint8_t mem[RC_COMPILE_MAX_BIN_SIZE];

    uint32_t utmp;

    uintmx paddr = getPhysicalAddress(pM, pointer, 0, &memerror);
    if( memerror ){ used=count=0; goto compile_finished; }

    gen_push_reg(mem, &used, REG_NUM_DI);
    gen_pop_reg (mem, &used, REG_NUM_SI);
    gen_prefix_rex64(mem, &used);
    gen_add_reg_imm32 (mem, &used, REG_NUM_SI, reg_offset); // 64-bit add

    for( ; used < RC_COMPILE_MAX_BIN_SIZE - 64; count++){
        if( (pointer&0xfff) == 0 ){
            paddr = getPhysicalAddress(pM, pointer, 0, &memerror);
            if( memerror ){ goto compile_finished; }
        }
        inst32     = loadPhysMemHalfWord(pM, paddr);
        uint8_t op = ((inst32>>11) & 0x1f); /* inst[15:11] */

        if( op == MIPS16E_OP_JAL ){
            goto compile_finished;
        }

        if( op == MIPS16E_OP_EXTEND ){
            if( ((pointer+2)&0xfff) == 0 ){
                paddr = getPhysicalAddress(pM, pointer+2, 0, &memerror) -2;
                if( memerror ){ goto compile_finished; }
            }
            inst32 = (inst32<<16) + loadPhysMemHalfWord(pM, paddr+2);
        }

        uint8_t  op5  = ((inst32>>11) & 0x1f); /* inst[15:11] */
        uint8_t  rx   = ((inst32>> 8) & 0x07); /* inst[10: 8] */
        uint8_t  ry   = ((inst32>> 5) & 0x07); /* inst[ 7: 5] */
        uint8_t  rz   = ((inst32>> 2) & 0x07); /* inst[ 4: 2] */
        uint8_t  sa3  = ((inst32>> 2) & 0x07); /* inst[ 4: 2] */
        uint8_t funct5= ( inst32      & 0x1f); /* inst[ 5: 0] */
        uint8_t  imm8 = ( inst32      & 0xff); /* inst[ 7: 0] */

        uint16_t imm16 = (((inst32>>5)&(0x1f<<11)) | ((inst32>>16)&(0x3f<<5)) | funct5);
        uint16_t imm15 = (((inst32>>5)&(0x0f<<11)) | ((inst32>>16)&(0x7f<<4)) | (inst32&0xf));

        switch( op5 ){
            case MIPS16E_OP_LB:
                if( WITH_PREXIX(inst32) ){
                    utmp = SIGN_EXT16(imm16);
                }else{
                    utmp = funct5;
                }
                gen_load_insts(pM, mem, &used, imem, pointer+1, pc_offset, XLAT(ry), XLAT(rx), utmp, (uint64_t)lb_helper);
                break;
            case MIPS16E_OP_LBU:
                if( WITH_PREXIX(inst32) ){
                    utmp = SIGN_EXT16(imm16);
                }else{
                    utmp = funct5;
                }
                gen_load_insts(pM, mem, &used, imem, pointer+1, pc_offset, XLAT(ry), XLAT(rx), utmp, (uint64_t)lbu_helper);
                break;
            case MIPS16E_OP_LH:
                if( WITH_PREXIX(inst32) ){
                    utmp = SIGN_EXT16(imm16);
                }else{
                    utmp = (funct5<<1);
                }
                gen_load_insts(pM, mem, &used, imem, pointer+1, pc_offset, XLAT(ry), XLAT(rx), utmp, (uint64_t)lh_helper);
                break;
            case MIPS16E_OP_LHU:
                if( WITH_PREXIX(inst32) ){
                    utmp = SIGN_EXT16(imm16);
                }else{
                    utmp = (funct5<<1);
                }
                gen_load_insts(pM, mem, &used, imem, pointer+1, pc_offset, XLAT(ry), XLAT(rx), utmp, (uint64_t)lhu_helper);
                break;
            case MIPS16E_OP_LW:
                if( WITH_PREXIX(inst32) ){
                    utmp = SIGN_EXT16(imm16);
                }else{
                    utmp = (funct5<<2);
                }
                gen_load_insts(pM, mem, &used, imem, pointer+1, pc_offset, XLAT(ry), XLAT(rx), utmp, (uint64_t)lw_helper);
                break;
            case MIPS16E_OP_LWSP:
                if( WITH_PREXIX(inst32) ){
                    utmp = SIGN_EXT16(imm16);
                }else{
                    utmp = (ZERO_EXT8(imm8)<<2);
                }
                gen_load_insts(pM, mem, &used, imem, pointer+1, pc_offset, XLAT(rx), 29, utmp, (uint64_t)lw_helper);
                break;
            case MIPS16E_OP_LWPC:
                if( WITH_PREXIX(inst32) ){
                    utmp = (pointer & ~((uintmx)3)) + SIGN_EXT16(imm16);
                }else{
                    utmp = (pointer & ~((uintmx)3)) + (ZERO_EXT8(imm8)<<2);
                }
                gen_load_insts(pM, mem, &used, imem, pointer+1, pc_offset, XLAT(rx), 0, utmp, (uint64_t)lw_helper);
                break;
            case MIPS16E_OP_SB:
                if( WITH_PREXIX(inst32) ){
                    utmp = SIGN_EXT16(imm16);
                }else{
                    utmp = funct5;
                }
                gen_store_insts(pM, mem, &used, imem, pointer+1, pc_offset, XLAT(ry), XLAT(rx), utmp, (uint64_t)sb_helper);
                break;
            case MIPS16E_OP_SH:
                if( WITH_PREXIX(inst32) ){
                    utmp = SIGN_EXT16(imm16);
                }else{
                    utmp = (funct5<<1);
                }
                gen_store_insts(pM, mem, &used, imem, pointer+1, pc_offset, XLAT(ry), XLAT(rx), utmp, (uint64_t)sh_helper);
                break;
            case MIPS16E_OP_SW:
                if( WITH_PREXIX(inst32) ){
                    utmp = SIGN_EXT16(imm16);
                }else{
                    utmp = (funct5<<2);
                }
                gen_store_insts(pM, mem, &used, imem, pointer+1, pc_offset, XLAT(ry), XLAT(rx), utmp, (uint64_t)sw_helper);
                break;
            case MIPS16E_OP_SWSP:
                if( WITH_PREXIX(inst32) ){
                    utmp = SIGN_EXT16(imm16);
                }else{
                    utmp = (ZERO_EXT8(imm8)<<2);
                }
                gen_store_insts(pM, mem, &used, imem, pointer+1, pc_offset, XLAT(rx), 29, utmp, (uint64_t)sw_helper);
                break;

            case MIPS16E_OP_B:
                if( WITH_PREXIX(inst32) ){
                    utmp = pointer + (SIGN_EXT16(imm16) << 1) + 4;
                }else{
                    utmp = pointer + (SIGN_EXT11(inst32 & 0x7ff)  << 1) + 2;
                }
                // To avoid unrolling of loop, only branches to foward instructions are condiered.
                if( utmp > pointer ){
                    if( (pointer & (~0xfff)) != (utmp & (~0xfff)) ){
                        paddr = getPhysicalAddress(pM, utmp, 0, &memerror);
                        if( memerror ){ goto compile_finished; }
                    }else{
                        paddr = paddr + utmp - pointer;
                    }
                    if( WITH_PREXIX(inst32) ){
                        paddr -= 4; pointer = utmp - 4;
                    }else{
                        paddr -= 2; pointer = utmp - 2;
                    }
                }else{
                    gen_mov_imm8_to_reg8       (mem, &used, REG_NUM_AX, 1);
                    gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, utmp +1);
                    gen_ret                    (mem, &used);
                    // TODO: update of pointer and paddr
                    count++;
                    goto compile_finished;
                }
                break;

            case MIPS16E_OP_BNEZ:
                if( WITH_PREXIX(inst32) ){
                    utmp = pointer + 1 + (SIGN_EXT16(imm16) << 1) + 4;
                }else{
                    utmp = pointer + 1 + (SIGN_EXT8(imm8) << 1) + 2;
                }
                gen_xor_reg_reg            (mem, &used, REG_NUM_AX, REG_NUM_AX);
                gen_cmp_reg_sidx8_base     (mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                gen_jz_rel_pc              (mem, &used, 2+10+1);
                gen_mov_imm8_to_reg8       (mem, &used, REG_NUM_AX, 1);
                gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, utmp);
                gen_ret                    (mem, &used);
                break;

            case MIPS16E_OP_BEQZ:
                if( WITH_PREXIX(inst32) ){
                    utmp = pointer + 1 + (SIGN_EXT16(imm16) << 1) + 4;
                }else{
                    utmp = pointer + 1 + (SIGN_EXT8(imm8) << 1) + 2;
                }
                gen_xor_reg_reg            (mem, &used, REG_NUM_AX, REG_NUM_AX);
                gen_cmp_reg_sidx8_base     (mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                gen_jnz_rel_pc             (mem, &used, 2+10+1);
                gen_mov_imm8_to_reg8       (mem, &used, REG_NUM_AX, 1);
                gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, utmp);
                gen_ret                    (mem, &used);
                break;

            case MIPS16E_OP_ADDIUSP:
                if( WITH_PREXIX(inst32) ){
                    utmp = SIGN_EXT16(imm16);
                }else{
                    utmp = (ZERO_EXT8(imm8)<<2);
                }
                gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX,       29*4, REG_NUM_SI);
                gen_add_reg_imm32        (mem, &used, REG_NUM_AX, utmp);
                gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                break;
            case MIPS16E_OP_ADDIUPC:
                if( WITH_PREXIX(inst32) ){
                    utmp = SIGN_EXT16(imm16);
                }else{
                    utmp = (ZERO_EXT8(imm8)<<2);
                }
                gen_mov_imm32_to_sidx8_base(mem, &used, XLAT(rx)*4, REG_NUM_SI, utmp + (pointer & ~((uintmx)3)) );
                break;
            case MIPS16E_OP_SHIFT:
                switch( funct5 & 3 ){
                    case 0:// SLL 
                        if( WITH_PREXIX(inst32) ){
                            utmp = ((inst32>>22) & 0x1f);
                        }else{
                            utmp = (sa3==0) ? 8 : sa3;
                        }
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, XLAT(ry)*4, REG_NUM_SI);
                        gen_sll_reg_imm8         (mem, &used, REG_NUM_AX, utmp);
                        gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                        break;
                    case 2: // SRL 
                        if( WITH_PREXIX(inst32) ){
                            utmp = ((inst32>>22) & 0x1f);
                        }else{
                            utmp = (sa3==0) ? 8 : sa3;
                        }
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, XLAT(ry)*4, REG_NUM_SI);
                        gen_srl_reg_imm8         (mem, &used, REG_NUM_AX, utmp);
                        gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                        break;
                    default:
                        goto compile_finished;
                }
                break;
            case MIPS16E_OP_RRIA:
                if( inst32 & (1<<4) ){
                    goto compile_finished;
                }else{
                    if( WITH_PREXIX(inst32) ){
                        utmp = SIGN_EXT15(imm15);
                    }else{
                        utmp = SIGN_EXT4(inst32 & 0xf);
                    }
                    gen_mov_imm32_to_reg     (mem, &used, REG_NUM_AX, utmp);
                    gen_add_reg_sidx8_base   (mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                    gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, XLAT(ry)*4, REG_NUM_SI);
                }
                break;
            case MIPS16E_OP_ADDIU8:
                if( WITH_PREXIX(inst32) ){
                    utmp = SIGN_EXT16(imm16);
                }else{
                    utmp = SIGN_EXT8(imm8);
                }
                gen_add_sidx8_base_imm32(mem, &used, XLAT(rx)*4, REG_NUM_SI, utmp);
                break;
            case MIPS16E_OP_SLTI:
                if( WITH_PREXIX(inst32) ){
                    utmp = SIGN_EXT16(imm16);
                }else{
                    utmp = ZERO_EXT8(imm8);
                }
                gen_xor_reg_reg          (mem, &used, REG_NUM_AX, REG_NUM_AX);
                gen_cmp_sidx8_base_imm32 (mem, &used, XLAT(rx)*4, REG_NUM_SI, utmp);
                gen_setl_reg             (mem, &used, REG_NUM_AX);
                gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX,       24*4, REG_NUM_SI);
                break;
            case MIPS16E_OP_SLTIU:
                if( WITH_PREXIX(inst32) ){
                    utmp = SIGN_EXT16(imm16);
                }else{
                    utmp = ZERO_EXT8(imm8);
                }
                gen_xor_reg_reg          (mem, &used, REG_NUM_AX, REG_NUM_AX);
                gen_cmp_sidx8_base_imm32 (mem, &used, XLAT(rx)*4, REG_NUM_SI, utmp);
                gen_setb_reg             (mem, &used, REG_NUM_AX);
                gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX,       24*4, REG_NUM_SI);
                break;
            case MIPS16E_OP_I8:
                switch( rx ){ 
                    case 0: // bteqz
                        if( WITH_PREXIX(inst32) ){
                            utmp = pointer + 1 + (SIGN_EXT16(imm16) << 1) + 4;
                        }else{
                            utmp = pointer + 1 + (SIGN_EXT8(imm8) << 1) + 2;
                        }
                        gen_xor_reg_reg            (mem, &used, REG_NUM_AX, REG_NUM_AX);
                        gen_cmp_reg_sidx8_base     (mem, &used, REG_NUM_AX, 24*4, REG_NUM_SI);
                        gen_jnz_rel_pc             (mem, &used, 2+10+1);
                        gen_mov_imm8_to_reg8       (mem, &used, REG_NUM_AX, 1);
                        gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, utmp);
                        gen_ret                    (mem, &used);
                        break;
                    case 1: // btnez
                        if( WITH_PREXIX(inst32) ){
                            utmp = pointer + 1 + (SIGN_EXT16(imm16) << 1) + 4;
                        }else{
                            utmp = pointer + 1 + (SIGN_EXT8(imm8) << 1) + 2;
                        }
                        gen_xor_reg_reg            (mem, &used, REG_NUM_AX, REG_NUM_AX);
                        gen_cmp_reg_sidx8_base     (mem, &used, REG_NUM_AX, 24*4, REG_NUM_SI);
                        gen_jz_rel_pc              (mem, &used, 2+10+1);
                        gen_mov_imm8_to_reg8       (mem, &used, REG_NUM_AX, 1);
                        gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, utmp);
                        gen_ret                    (mem, &used);
                        break;
                    case 3: // ADJSP
                        if( WITH_PREXIX(inst32) ){
                            utmp = SIGN_EXT16(imm16);
                        }else{
                            utmp = (SIGN_EXT8(imm8)<<3);
                        }
                        gen_add_sidx8_base_imm32(mem, &used, 29*4, REG_NUM_SI, utmp);
                        break;
                    case 5: // move reg, reg
                        // This instruction has no extended version
                        if( (ry | (funct5&0x18)) != 0 ){
                            gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, XLAT(funct5 & 0x7)  *4, REG_NUM_SI);
                            gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, (ry | (funct5&0x18))*4, REG_NUM_SI);
                        }
                        break;
                    case 7: // move reg, reg
                        // This instruction has no extended version
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX,  funct5  *4, REG_NUM_SI);
                        gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX,  XLAT(ry)*4, REG_NUM_SI);
                        break;
                    case 2: /* SWRASP */
                        // This instruction is unchecked because it does not appear.
                        if( WITH_PREXIX(inst32) ){
                            utmp = SIGN_EXT16(imm16);
                        }else{
                            utmp = (ZERO_EXT8(imm8)<<2);
                        }
                        gen_store_insts(pM, mem, &used, imem, pointer+1, pc_offset, 31, 29, utmp, (uint64_t)sw_helper);
                        break;
                    default:
                        goto compile_finished;
                }
                break;
            case MIPS16E_OP_LI:
                if( WITH_PREXIX(inst32) ){
                    utmp = ZERO_EXT16(imm16);
                }else{
                    utmp = ZERO_EXT8(imm8);
                }
                gen_mov_imm32_to_sidx8_base(mem, &used, XLAT(rx)*4, REG_NUM_SI, utmp);
                break;
            case MIPS16E_OP_CMPI:
                if( WITH_PREXIX(inst32) ){
                    utmp = ZERO_EXT16(imm16);
                }else{
                    utmp = ZERO_EXT8(imm8);
                }
                gen_mov_imm32_to_reg     (mem, &used, REG_NUM_AX, utmp);
                gen_xor_reg_sidx8_base   (mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX,       24*4, REG_NUM_SI);
                break;
            case MIPS16E_OP_RRR:
                switch(inst32 & 3){
                    case 1:
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX,  XLAT(rx)*4, REG_NUM_SI);
                        gen_add_reg_sidx8_base   (mem, &used, REG_NUM_AX,  XLAT(ry)*4, REG_NUM_SI);
                        gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX,  XLAT(rz)*4, REG_NUM_SI);
                        break;
                    case 3:
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX,  XLAT(rx)*4, REG_NUM_SI);
                        gen_sub_reg_sidx8_base   (mem, &used, REG_NUM_AX,  XLAT(ry)*4, REG_NUM_SI);
                        gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX,  XLAT(rz)*4, REG_NUM_SI);
                        break;
                    default:
                        goto compile_finished;
                }
                break;
            case MIPS16E_OP_RR:
                switch(funct5){
                    case MIPS16E_RRFUNCT_JR:
                        switch( ry ){
                            case 4: // jrc
                                gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX,  XLAT(rx)*4, REG_NUM_SI);
                                gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_AX,   pc_offset, REG_NUM_DI);
                                gen_mov_imm8_to_reg8     (mem, &used, REG_NUM_AX, 1);
                                gen_ret                  (mem, &used);
                                count++;
                                break;
                            case 5: // jrc $ra
                                gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX,        31*4, REG_NUM_SI);
                                gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_AX,   pc_offset, REG_NUM_DI);
                                gen_mov_imm8_to_reg8     (mem, &used, REG_NUM_AX, 1);
                                gen_ret                  (mem, &used);
                                count++;
                                break;
                            case 6: // jalrc
                                gen_mov_sidx8_base_to_reg  (mem, &used, REG_NUM_AX,  XLAT(rx)*4, REG_NUM_SI);
                                gen_mov_reg_to_idx32_base  (mem, &used, REG_NUM_AX,   pc_offset, REG_NUM_DI);
                                gen_mov_imm32_to_sidx8_base(mem, &used,       31*4,  REG_NUM_SI, pointer + 1 + 2);
                                gen_mov_imm8_to_reg8       (mem, &used, REG_NUM_AX, 1);
                                gen_ret                    (mem, &used);
                                count++;
                                break;
                        }
                        goto compile_finished;

                    case MIPS16E_RRFUNCT_SLT:
                        gen_xor_reg_reg          (mem, &used, REG_NUM_AX, REG_NUM_AX);
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_DX, XLAT(rx)*4, REG_NUM_SI);
                        gen_cmp_reg_sidx8_base   (mem, &used, REG_NUM_DX, XLAT(ry)*4, REG_NUM_SI);
                        gen_setl_reg             (mem, &used, REG_NUM_AX);
                        gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX,       24*4, REG_NUM_SI);
                        break;
                    case MIPS16E_RRFUNCT_SLTU:
                        gen_xor_reg_reg          (mem, &used, REG_NUM_AX, REG_NUM_AX);
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_DX, XLAT(rx)*4, REG_NUM_SI);
                        gen_cmp_reg_sidx8_base   (mem, &used, REG_NUM_DX, XLAT(ry)*4, REG_NUM_SI);
                        gen_setb_reg             (mem, &used, REG_NUM_AX);
                        gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX,       24*4, REG_NUM_SI);
                        break;
                    case MIPS16E_RRFUNCT_SLLV:
                        gen_mov_imm8_to_reg8     (mem, &used, REG_NUM_CX, 0x1f);
                        gen_and_reg_sidx8_base   (mem, &used, REG_NUM_CX, XLAT(rx)*4, REG_NUM_SI);
                        gen_sll_sidx8_base_cl    (mem, &used, XLAT(ry)*4, REG_NUM_SI); 
                        break;
                    case MIPS16E_RRFUNCT_SRLV:
                        gen_mov_imm8_to_reg8     (mem, &used, REG_NUM_CX, 0x1f);
                        gen_and_reg_sidx8_base   (mem, &used, REG_NUM_CX, XLAT(rx)*4, REG_NUM_SI);
                        gen_srl_sidx8_base_cl    (mem, &used, XLAT(ry)*4, REG_NUM_SI);
                        break;
                    case MIPS16E_RRFUNCT_CMP:
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                        gen_xor_reg_sidx8_base   (mem, &used, REG_NUM_AX, XLAT(ry)*4, REG_NUM_SI);
                        gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX,       24*4, REG_NUM_SI);
                        break;
                    case MIPS16E_RRFUNCT_NEG:
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, XLAT(ry)*4, REG_NUM_SI);
                        gen_neg_reg              (mem, &used, REG_NUM_AX);
                        gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                        break;
                    case MIPS16E_RRFUNCT_AND:
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, XLAT(ry)*4, REG_NUM_SI);
                        gen_and_sidx8_base_reg   (mem, &used, XLAT(rx)*4, REG_NUM_SI, REG_NUM_AX);
                        break;
                    case MIPS16E_RRFUNCT_OR:
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, XLAT(ry)*4, REG_NUM_SI);
                        gen_or_sidx8_base_reg    (mem, &used, XLAT(rx)*4, REG_NUM_SI, REG_NUM_AX);
                        break;
                    case MIPS16E_RRFUNCT_XOR:
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, XLAT(ry)*4, REG_NUM_SI);
                        gen_xor_sidx8_base_reg   (mem, &used, XLAT(rx)*4, REG_NUM_SI, REG_NUM_AX);
                        break;
                    case MIPS16E_RRFUNCT_NOT:
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, XLAT(ry)*4, REG_NUM_SI);
                        gen_not_reg              (mem, &used, REG_NUM_AX);
                        gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                        break;
                    case MIPS16E_RRFUNCT_MFHI:
                        gen_mov_idx32_base_to_reg(mem, &used, REG_NUM_AX,  hi_offset, REG_NUM_DI);
                        gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                        break;
                    case MIPS16E_RRFUNCT_CNVT:
                        switch(ry){
                            case 0: // zeb
                                gen_xor_reg_reg          (mem, &used, REG_NUM_AX, REG_NUM_AX);
                                gen_mov_imm8_to_reg8     (mem, &used, REG_NUM_AX, 0xff);
                                gen_and_reg_sidx8_base   (mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                                gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                                break;
                            case 1: // zeh
                                gen_mov_imm32_to_reg     (mem, &used, REG_NUM_AX, 0xffff);
                                gen_and_reg_sidx8_base   (mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                                gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                                break;
                            case 4: // seb
                                gen_movsx_sidx8_base_to_reg(mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                                gen_mov_reg_to_sidx8_base  (mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                                break;
                            case 5: // seh
                                gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX,  XLAT(rx)*4, REG_NUM_SI);
                                gen_cwde                 (mem, &used);
                                gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX,  XLAT(rx)*4, REG_NUM_SI);
                                break;
                            default:
                                break;
                        }
                        break;
                    case MIPS16E_RRFUNCT_MFLO:
                        gen_mov_idx32_base_to_reg(mem, &used, REG_NUM_AX,  lo_offset, REG_NUM_DI);
                        gen_mov_reg_to_sidx8_base(mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                        break;
                    case MIPS16E_RRFUNCT_MULT:
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_CX, XLAT(ry)*4, REG_NUM_SI);
                        gen_imul                 (mem, &used, REG_NUM_CX);
                        gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_DX, hi_offset, REG_NUM_DI);
                        gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_AX, lo_offset, REG_NUM_DI);
                        break;
                    case MIPS16E_RRFUNCT_MULTU:
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_CX, XLAT(ry)*4, REG_NUM_SI);
                        gen_mul                  (mem, &used, REG_NUM_CX);
                        gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_DX, hi_offset, REG_NUM_DI);
                        gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_AX, lo_offset, REG_NUM_DI);
                        break;
                    case MIPS16E_RRFUNCT_DIV:
                        gen_xor_reg_reg          (mem, &used, REG_NUM_DX, REG_NUM_DX);
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_CX, XLAT(ry)*4, REG_NUM_SI);
                        gen_test_reg_reg         (mem, &used, REG_NUM_CX, REG_NUM_CX); // if cx=0, zf will be 1.
                        gen_jz_rel_pc            (mem, &used, 14);
                        gen_idiv                 (mem, &used, REG_NUM_CX); // signed division, 2 bytes
                        gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_DX, hi_offset, REG_NUM_DI); // reminder, 6 bytes
                        gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_AX, lo_offset, REG_NUM_DI); // quotient, 6 bytes
                        break;
                    case MIPS16E_RRFUNCT_DIVU:
                        gen_xor_reg_reg          (mem, &used, REG_NUM_DX, REG_NUM_DX);
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_AX, XLAT(rx)*4, REG_NUM_SI);
                        gen_mov_sidx8_base_to_reg(mem, &used, REG_NUM_CX, XLAT(ry)*4, REG_NUM_SI);
                        gen_test_reg_reg         (mem, &used, REG_NUM_CX, REG_NUM_CX); // if cx=0, zf will be 1.
                        gen_jz_rel_pc            (mem, &used, 14);
                        gen_div                  (mem, &used, REG_NUM_CX); // unsigned division, 2 bytes
                        gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_DX, hi_offset, REG_NUM_DI); // reminder, 6 bytes
                        gen_mov_reg_to_idx32_base(mem, &used, REG_NUM_AX, lo_offset, REG_NUM_DI); // quotient, 6 bytes
                        break;
                    default:
                        goto compile_finished;
                }
                break;
            default:
                goto compile_finished;
        }

        paddr   += (WITH_PREXIX(inst32) ? 4 : 2);
        pointer += (WITH_PREXIX(inst32) ? 4 : 2);
    }

compile_finished:
    if(count > 0){
        gen_mov_imm32_to_idx32_base(mem, &used, pc_offset, REG_NUM_DI, pointer + 1 );
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
        //__asm volatile("serialize");
    }else{
        return 0;
    }

/*
    if( count > 0 ){
        printf("<%d 0x%08x>", count, inst);
    }
*/

    return count;
}

