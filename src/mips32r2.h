#include <stdio.h>
#include <stdint.h>
#include <signal.h>

#include "mips.h"
#include "mips_op.h"
#include "ExInst_common.h"

#include "misc.h"
#include "logfile.h"
#include "mem.h"
#include "cp0.h"
#include "tlb.h"
#include "exception.h"

#define MIPS32R2_EXEC_FAIL -1
#define MIPS32R2_EXEC_OK   0

#define LOGLEVEL_EMU_ERROR_MIPS32R2  (LOGCAT_EMU | LOGLV_ERROR)

#define UNKNOWN_INSTRUCTION(additional_msg)   \
do{                                           \
    fprintf(stderr, "Unknown instruction (%s)\n", additional_msg);                                         \
    fprintf(stderr, "inst=0x%x op=0x%x rs=0x%x rt=0x%x rd=0x%x shamt=0x%x funct=0x%x\n", inst, op,rs,rt,rd,shamt, funct);  \
    logfile_printf(LOGLEVEL_EMU_ERROR_MIPS32R2, "Unknown instruction (%s)\n", additional_msg);                      \
    return MIPS32R2_EXEC_FAIL;                                                                             \
}while(0)

static inline int execMIPS32eInst(struct stMachineState *pM, uintmx inst){

    uint8_t  op  = ((inst>>26) & 0x3f); /* inst[31:26] */
    uint8_t  rs  = ((inst>>21) & 0x1f); /* inst[25:21] */
    uint8_t  rt  = ((inst>>16) & 0x1f); /* inst[20:16] */
    uint8_t  rd  = ((inst>>11) & 0x1f); /* inst[15:11] */
    uint8_t shamt= ((inst>> 6) & 0x1f); /* inst[10: 6] */
    uint8_t funct= ( inst      & 0x3f); /* inst[ 5: 0] */
    uint16_t imm = ( inst      & 0xffff); /* inst[15:0] */

    uintmx jumpaddr, loadData;
    uintmx utmp;
    int accError;

    if( inst == 0 ){
        if(DEBUG) EXI_LOG_PRINTF("nop\n");
        UPDATE_PC_NEXT(pM);
        return MIPS32R2_EXEC_OK;
    }

    switch( op ){

        case MIPS32_OP_SPECIAL:
            switch( funct ){
                case 0x20: // add 
                    if(DEBUG) EXI_LOG_PRINTF("add %s, %s, %s\n", regStr[rd], regStr[rs], regStr[rt]);
                    pM->reg.r[rd] = pM->reg.r[rs] + pM->reg.r[rt];
                    if(    (0==((pM->reg.r[rs] | pM->reg.r[rt]) & 0x80000000) && (0!=(pM->reg.r[rd] & 0x80000000))) || // pos + pos -> neg
                        (0!=((pM->reg.r[rs] & pM->reg.r[rt]) & 0x80000000) && (0==(pM->reg.r[rd] & 0x80000000)))    // neg + neg -> pos
                    ){ 
                        prepareException(pM, EXCEPT_CODE_INTEGER_OVERFLOW, 0);
                    }else{
                        UPDATE_PC_NEXT(pM);
                    }
                    // TODO: overflow check is necessary
                    break;
                case 0x21: // addu
                    if(DEBUG) EXI_LOG_PRINTF("addu %s, %s, %s\n", regStr[rd], regStr[rs], regStr[rt]);
                    pM->reg.r[rd] = pM->reg.r[rs] + pM->reg.r[rt];
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x24: // and
                    if(DEBUG) EXI_LOG_PRINTF("and %s, %s, %s\n", regStr[rd], regStr[rs], regStr[rt]);
                    pM->reg.r[rd] = (pM->reg.r[rs] & pM->reg.r[rt]);
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x08: // jr
                    if( (imm & (1<<10)) && rd == 0 && rt == 0 ){
                        // Hazard barrier vriant of JR instruction 

                        #if USE_RUNTIME_COMPILATION
                            // When making a write to the instruction stream visible,
                            // Following 3 steps are necessary
                            // 
                            // 1. cache or synci instructions to synchronize cache
                            // 2. sync instruction for synchronization
                            // 3. jalr.hb / jr.hb / eret
                            //
                            // Thus, calling rc_clear at only one of the above steps is necessary.
                            rc_clear();
                        #endif
                        UPDATE_PC_NEXT_WITH_DELAYED_IMM(pM, pM->reg.r[rs]);
                        if(DEBUG) EXI_LOG_PRINTF("jr.hb %s(=0x%x)\n", regStr[rs], pM->reg.r[rs]);
                    }else{
                        UPDATE_PC_NEXT_WITH_DELAYED_IMM(pM, pM->reg.r[rs]);
                        if(DEBUG) EXI_LOG_PRINTF("jr %s(=0x%x)\n", regStr[rs], pM->reg.r[rs]);
                    }
                    break;
                case 0x09: // jalr / jalr.hb
                    if( shamt & 0x10 ){
                        if(DEBUG) EXI_LOG_PRINTF("jalr.hb %s, %s(=0x%x)\n", regStr[rd], regStr[rs], pM->reg.r[rs]);
                        // hazard barrier
                        // to make the new instructions effective after writting a new instruction stream
                        rc_clear();
                    }else{
                        if(DEBUG) EXI_LOG_PRINTF("jalr %s, %s(=0x%x)\n", regStr[rd], regStr[rs], pM->reg.r[rs]);
                    }
                    jumpaddr = pM->reg.r[rs];
                    pM->reg.r[rd] = pM->reg.pc + 8;
                    UPDATE_PC_NEXT_WITH_DELAYED_IMM(pM, jumpaddr);
                    break;
                case 0x27: // nor
                    if(DEBUG) EXI_LOG_PRINTF("nor %s, %s, %s\n", regStr[rd], regStr[rs], regStr[rt]);
                    pM->reg.r[rd] = ~(pM->reg.r[rs] | pM->reg.r[rt]);
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x25: // or
                    if(DEBUG) EXI_LOG_PRINTF("or %s, %s, %s\n", regStr[rd], regStr[rs], regStr[rt]);
                    pM->reg.r[rd] = (pM->reg.r[rs] | pM->reg.r[rt]);
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x26: // xor
                    if(DEBUG) EXI_LOG_PRINTF("xor %s, %s, %s\n", regStr[rd], regStr[rs], regStr[rt]);
                    pM->reg.r[rd] = (pM->reg.r[rs] ^ pM->reg.r[rt]);
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x2a: // slt
                    if(DEBUG) EXI_LOG_PRINTF("slt %s, %s, %s\n", regStr[rd], regStr[rs], regStr[rt]);
                    pM->reg.r[rd] = (((intmx)pM->reg.r[rs]) < ((intmx)pM->reg.r[rt])) ? 1 : 0;
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x2b: // sltu
                    if(DEBUG) EXI_LOG_PRINTF("sltu %s, %s, %s\n", regStr[rd], regStr[rs], regStr[rt]);
                    pM->reg.r[rd] = (((uintmx)pM->reg.r[rs]) < ((uintmx)pM->reg.r[rt])) ? 1 : 0;
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x0a: // movz
                    if(DEBUG) EXI_LOG_PRINTF("movz %s, %s, %s\n", regStr[rd], regStr[rs], regStr[rt]);
                    if(pM->reg.r[rt] == 0){
                        pM->reg.r[rd] = pM->reg.r[rs];
                    }
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x0b: // movn
                    if(DEBUG) EXI_LOG_PRINTF("movn %s, %s, %s\n", regStr[rd], regStr[rs], regStr[rt]);
                    if(pM->reg.r[rt] != 0){
                        pM->reg.r[rd] = pM->reg.r[rs];
                    }
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x00: // sll
                    if( inst == 0x00000000 ){
                        if(DEBUG) EXI_LOG_PRINTF("nop\n");
                        UPDATE_PC_NEXT(pM);
                    }else if( inst == 0x000000c0 ){
                        if(DEBUG) EXI_LOG_PRINTF("ehb\n");
                        UPDATE_PC_NEXT(pM);
                    }else if(rs == 0x00){
                        if(DEBUG) EXI_LOG_PRINTF("sll %s, %s, 0x%x\n", regStr[rd], regStr[rt], shamt);
                        pM->reg.r[rd] = (pM->reg.r[rt] << shamt);
                        UPDATE_PC_NEXT(pM);
                    }else{
                        UNKNOWN_INSTRUCTION("op=0x00 funct=0x00");
                    }
                    break;
                case 0x02: // srl or rotr
                    if( rs == 0 ){ // srl
                        if(DEBUG) EXI_LOG_PRINTF("srl %s, %s, 0x%x\n", regStr[rd], regStr[rt], shamt);
                        pM->reg.r[rd] = (((uint64_t)((uintmx)pM->reg.r[rt])) >> shamt);
                        UPDATE_PC_NEXT(pM);

                    }else if( rs == 1 ){ // rotr
                        if(DEBUG) EXI_LOG_PRINTF("rotr %s, %s, 0x%x\n", regStr[rd], regStr[rt], shamt);
                        if( shamt != 0 ){
                            utmp = (((uint64_t)((uintmx)pM->reg.r[rt])) >> shamt);
                            utmp|= (((uintmx)pM->reg.r[rt]) << (32-shamt));
                        }else{
                            utmp = pM->reg.r[rt];
                        }
                        pM->reg.r[rd] = utmp;
                        UPDATE_PC_NEXT(pM);
                    }else{
                        UNKNOWN_INSTRUCTION("right shift 0x02");
                    }
                    break;
                case 0x03: // sra
                    if(DEBUG) EXI_LOG_PRINTF("sra %s, %s, 0x%x\n", regStr[rd], regStr[rt], shamt);
                    pM->reg.r[rd] = (((int64_t)((intmx)pM->reg.r[rt])) >> shamt);
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x04: // sllv
                    if(DEBUG) EXI_LOG_PRINTF("sllv %s, %s, %s\n", regStr[rd], regStr[rt], regStr[rs]);
                    pM->reg.r[rd] = (pM->reg.r[rt] << ((pM->reg.r[rs]) & 0x1f));
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x06: // srlv or rotrv
                    if(shamt == 0){ // srlv
                        if(DEBUG) EXI_LOG_PRINTF("srlv %s, %s, %s\n", regStr[rd], regStr[rt], regStr[rs]);
                        pM->reg.r[rd] = (((uint64_t)((uintmx)pM->reg.r[rt])) >> ((pM->reg.r[rs]) & 0x1f));
                        UPDATE_PC_NEXT(pM);
                    }else if(shamt == 1){ // rotrv
                        if(DEBUG) EXI_LOG_PRINTF("rotrv %s, %s, %s\n", regStr[rd], regStr[rt], regStr[rs]);
                        uintmx sa = (pM->reg.r[rt] & 0x1f);
                        if( sa != 0 ){
                            utmp = (((uint64_t)((uintmx)pM->reg.r[rt])) >> sa);
                            utmp|= (((uintmx)pM->reg.r[rt]) << (32-sa));
                        }else{
                            utmp = pM->reg.r[rt];
                        }
                        pM->reg.r[rd] = utmp;
                        UPDATE_PC_NEXT(pM);
                    }else{
                        UNKNOWN_INSTRUCTION("right shift 0x06");
                    }
                    break;
                case 0x07: // srav
                    if(shamt == 0){
                        if(DEBUG) EXI_LOG_PRINTF("srav %s, %s, %s\n", regStr[rd], regStr[rt], regStr[rs]);
                        pM->reg.r[rd] = (((int64_t)((intmx)pM->reg.r[rt])) >> ((pM->reg.r[rs]) & 0x1f));
                        UPDATE_PC_NEXT(pM);
                    }else{
                        UNKNOWN_INSTRUCTION("right shift 0x07");
                    }
                    break;
                case 0x22: // sub
                    if(DEBUG) EXI_LOG_PRINTF("sub %s, %s, %s\n", regStr[rd], regStr[rs], regStr[rt]);
                    pM->reg.r[rd] = pM->reg.r[rs] - pM->reg.r[rt];
                    if(    ((pM->reg.r[rs] ^ pM->reg.r[rt]) & 0x80000000) && ((pM->reg.r[rs] ^ pM->reg.r[rd]) & 0x80000000) ){ 
                        prepareException(pM, EXCEPT_CODE_INTEGER_OVERFLOW, 0);
                    }else{
                        UPDATE_PC_NEXT(pM);
                    }
                    // TODO: overflow check is necessary
                    break;
                case 0x23: // subu
                    if(DEBUG) EXI_LOG_PRINTF("subu %s, %s, %s\n", regStr[rd], regStr[rs], regStr[rt]);
                    pM->reg.r[rd] = pM->reg.r[rs] - pM->reg.r[rt];
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x10: // mfhi
                    if(DEBUG) EXI_LOG_PRINTF("mfhi %s\n", regStr[rd]);
                    pM->reg.r[rd] = pM->reg.hi;
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x11: // mthi
                    if(DEBUG) EXI_LOG_PRINTF("mthi %s\n", regStr[rs]);
                    pM->reg.hi = pM->reg.r[rs];
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x12: // mflo
                    if(DEBUG) EXI_LOG_PRINTF("mflo %s\n", regStr[rd]);
                    pM->reg.r[rd] = pM->reg.lo;
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x13: // mtlo
                    if(DEBUG) EXI_LOG_PRINTF("mtlo %s\n", regStr[rs]);
                    pM->reg.lo = pM->reg.r[rs];
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x18:
                    if( shamt == 0x00 && rd == 0x00 ){
                        if(DEBUG) EXI_LOG_PRINTF("mult %s, %s\n", regStr[rs], regStr[rt]);
                        int64_t mul_tmp = ((int64_t)((intmx)(pM->reg.r[rs]))) * ((int64_t)((intmx)(pM->reg.r[rt])));
                        pM->reg.hi = (uintmx)((mul_tmp>>16)>>16);
                        pM->reg.lo = (uintmx)mul_tmp;
                        UPDATE_PC_NEXT(pM);
                    }else{
                        UNKNOWN_INSTRUCTION("op=0x00, funct=0x19");
                    }
                    break;
                case 0x19:
                    if( shamt == 0x00 && rd == 0x00 ){
                        if(DEBUG) EXI_LOG_PRINTF("multu %s, %s\n", regStr[rs], regStr[rt]);
                        uint64_t mul_tmp = ((uint64_t)((uintmx)(pM->reg.r[rs]))) * ((uint64_t)((uintmx)(pM->reg.r[rt])));
                        pM->reg.hi = (uintmx)((mul_tmp>>16)>>16);
                        pM->reg.lo = (uintmx)mul_tmp;
                        UPDATE_PC_NEXT(pM);
                    }else{
                        UNKNOWN_INSTRUCTION("op=0x00, funct=0x19");
                    }
                    break;
                case 0x1a: // div
                    if(DEBUG) EXI_LOG_PRINTF("div %s, %s\n", regStr[rs], regStr[rt]);
                    if( pM->reg.r[rt] == 0 ){
                        // zero division
                        pM->reg.lo = 0; // q
                        pM->reg.hi = 0; // r
                    }else{
                        pM->reg.lo = ((((intmx)(pM->reg.r[rs])) / ((intmx)(pM->reg.r[rt])))); // q
                        pM->reg.hi = ((((intmx)(pM->reg.r[rs])) % ((intmx)(pM->reg.r[rt])))); // r
                    }
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x1b: // divu
                    if(DEBUG) EXI_LOG_PRINTF("divu %s, %s\n", regStr[rs], regStr[rt]);
                    if( pM->reg.r[rt] == 0 ){
                        // zero division
                        pM->reg.lo = 0; // q
                        pM->reg.hi = 0; // r
                    }else{
                        pM->reg.lo = ((uintmx)(pM->reg.r[rs])) / ((uintmx)(pM->reg.r[rt])); // q
                        pM->reg.hi = ((uintmx)(pM->reg.r[rs])) % ((uintmx)(pM->reg.r[rt])); // r
                    }
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x30: // tge (trap if greater or equal)
                    if(DEBUG) EXI_LOG_PRINTF("tge %s, %s, 0x%x\n", regStr[rs], regStr[rt], (inst>>6)&(0x3ff));
                    if( ((intmx)pM->reg.r[rs]) >= ((intmx)pM->reg.r[rt]) ){
                        prepareException(pM, EXCEPT_CODE_TRAP, 0);
                    }else{
                        UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                    }
                    break;
                case 0x31: // tgeu (trap if greater or equal unsigned)
                    if(DEBUG) EXI_LOG_PRINTF("tgeu %s, %s, 0x%x\n", regStr[rs], regStr[rt], (inst>>6)&(0x3ff));
                    if( ((uintmx)pM->reg.r[rs]) >= ((uintmx)pM->reg.r[rt]) ){
                        prepareException(pM, EXCEPT_CODE_TRAP, 0);
                    }else{
                        UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                    }
                    break;
                case 0x32: // tlt (trap if less than)
                    if(DEBUG) EXI_LOG_PRINTF("tlt %s, %s, 0x%x\n", regStr[rs], regStr[rt], (inst>>6)&(0x3ff));
                    if( ((intmx)pM->reg.r[rs]) < ((intmx)pM->reg.r[rt]) ){
                        prepareException(pM, EXCEPT_CODE_TRAP, 0);
                    }else{
                        UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                    }
                    break;
                case 0x33: // tltu (trap if less than unsigned)
                    if(DEBUG) EXI_LOG_PRINTF("tltu %s, %s, 0x%x\n", regStr[rs], regStr[rt], (inst>>6)&(0x3ff));
                    if( ((uintmx)pM->reg.r[rs]) < ((uintmx)pM->reg.r[rt]) ){
                        prepareException(pM, EXCEPT_CODE_TRAP, 0);
                    }else{
                        UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                    }
                    break;
                case 0x34: // teq (trap if equal)
                    if(DEBUG) EXI_LOG_PRINTF("teq %s, %s, 0x%x\n", regStr[rs], regStr[rt], (inst>>6)&(0x3ff));
                    if( pM->reg.r[rs] == pM->reg.r[rt] ){
                        prepareException(pM, EXCEPT_CODE_TRAP, 0);
                    }else{
                        UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                    }
                    break;
                case 0x36: // tne (trap if not equal)
                    if(DEBUG) EXI_LOG_PRINTF("tne %s, %s, 0x%x\n", regStr[rs], regStr[rt], (inst>>6)&(0x3ff));
                    if( pM->reg.r[rs] != pM->reg.r[rt] ){
                        prepareException(pM, EXCEPT_CODE_TRAP, 0);
                    }else{
                        UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                    }
                    break;
                case 0x0c: // syscall
                    if(DEBUG) EXI_LOG_PRINTF("syscall\n");
                    //printf("syscall v0:%x a0:%x a1:%x\n", pM->reg.r[2], pM->reg.r[4], pM->reg.r[5]);
                    prepareException(pM, EXCEPT_CODE_SYSCALL, 0);
                    break;
                case 0x0f: // sync
                    if(DEBUG) EXI_LOG_PRINTF("sync %d\n", shamt);
/*
                    #if USE_RUNTIME_COMPILATION
                        rc_clear();
                    #endif
*/
                    UPDATE_PC_NEXT(pM);
                    break;
                default:
                    UNKNOWN_INSTRUCTION("op=0x00");
            }
            break;
        case MIPS32_OP_ADDI: // addi
            if(DEBUG) EXI_LOG_PRINTF("addi %s, %s, 0x%x\n", regStr[rt], regStr[rs], imm);
            utmp = SIGN_EXT16(imm);
            pM->reg.r[rt] = pM->reg.r[rs] + utmp;
            if(    (0==((pM->reg.r[rs] | utmp) & 0x80000000) && (0!=(pM->reg.r[rd] & 0x80000000))) || // pos + pos -> neg
                (0!=((pM->reg.r[rs] & utmp) & 0x80000000) && (0==(pM->reg.r[rd] & 0x80000000)))    // neg + neg -> pos
            ){ 
                prepareException(pM, EXCEPT_CODE_INTEGER_OVERFLOW, 0);
            }else{
                UPDATE_PC_NEXT(pM);
            }
            // TODO: overflow check is necessary
            break;
        case MIPS32_OP_ADDIU: // addiu
            if(DEBUG) EXI_LOG_PRINTF("addiu %s, %s, 0x%x\n", regStr[rt], regStr[rs], imm);
            pM->reg.r[rt] = pM->reg.r[rs] + SIGN_EXT16(imm);
            UPDATE_PC_NEXT(pM);
            break;
        case MIPS32_OP_ANDI: // andi
            if(DEBUG) EXI_LOG_PRINTF("andi %s, %s, 0x%x\n", regStr[rt], regStr[rs], imm);
            pM->reg.r[rt] = (pM->reg.r[rs] & ZERO_EXT16(imm));
            UPDATE_PC_NEXT(pM);
            break;
        case MIPS32_OP_BEQ: // beq
            if(DEBUG) EXI_LOG_PRINTF("beq %s, %s, 0x%x(=%x)\n", regStr[rt], regStr[rs], imm, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4);
            if( pM->reg.r[rs] == pM->reg.r[rt] ){
                UPDATE_PC_NEXT_WITH_DELAYED_IMM(pM, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4 );
            }else{
                UPDATE_PC_NEXT_BRANCH_FAIL(pM);
            }
            break;
        case MIPS32_OP_BEQL: // beql
            if(DEBUG) EXI_LOG_PRINTF("beql %s, %s, 0x%x(=%x)\n", regStr[rt], regStr[rs], imm, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4);
            if( pM->reg.r[rs] == pM->reg.r[rt] ){
                UPDATE_PC_NEXT_WITH_DELAYED_IMM(pM, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4 );
            }else{
                UPDATE_PC_NEXT(pM);
                UPDATE_PC_NEXT_BRANCH_FAIL(pM);
            }
            break;
        case MIPS32_OP_BNE: // bne
            if(DEBUG) EXI_LOG_PRINTF("bne %s, %s, 0x%x(=%x)\n", regStr[rt], regStr[rs], imm, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4);
            if( pM->reg.r[rs] != pM->reg.r[rt] ){
                UPDATE_PC_NEXT_WITH_DELAYED_IMM(pM, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4 );
            }else{
                UPDATE_PC_NEXT_BRANCH_FAIL(pM);
            }
            break;
        case MIPS32_OP_BLEZ: // blez
            if( rt == 0 ){
                if(DEBUG) EXI_LOG_PRINTF("blez %s, 0x%x(=%x)\n", regStr[rs], imm, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4);
                if( ((intmx)pM->reg.r[rs]) <= 0 ){
                    UPDATE_PC_NEXT_WITH_DELAYED_IMM(pM, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4 );
                }else{
                    UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                }
            }else{
                UNKNOWN_INSTRUCTION("op=0x06");
            }
            break;
        case MIPS32_OP_BLEZL: // blezl
            if( rt == 0 ){
                if(DEBUG) EXI_LOG_PRINTF("blez %s, 0x%x(=%x)\n", regStr[rs], imm, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4);
                if( ((intmx)pM->reg.r[rs]) <= 0 ){
                    UPDATE_PC_NEXT_WITH_DELAYED_IMM(pM, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4 );
                }else{
                    UPDATE_PC_NEXT(pM);
                    UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                }
            }else{
                UNKNOWN_INSTRUCTION("op=0x16");
            }
            break;
        case MIPS32_OP_BNEL: // bnel
            if(DEBUG) EXI_LOG_PRINTF("bnel %s, %s, 0x%x(=%x)\n", regStr[rt], regStr[rs], imm, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4);
            if( pM->reg.r[rs] != pM->reg.r[rt] ){
                UPDATE_PC_NEXT_WITH_DELAYED_IMM(pM, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4 );
            }else{
                UPDATE_PC_NEXT(pM);
                UPDATE_PC_NEXT_BRANCH_FAIL(pM);
            }
            break;
        case MIPS32_OP_BGTZ: // bgtz
            if(DEBUG) EXI_LOG_PRINTF("bgtz %s, 0x%x(=%x)\n", regStr[rs], imm, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4);
            if( ((intmx)pM->reg.r[rs]) > 0 ){
                UPDATE_PC_NEXT_WITH_DELAYED_IMM(pM, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4 );
            }else{
                UPDATE_PC_NEXT_BRANCH_FAIL(pM);
            }
            break;
        case MIPS32_OP_BGTZL: // bgtzl (changed in MIPS32R6)
            if( rt == 0x00 ){
                if(DEBUG) EXI_LOG_PRINTF("bgtzl %s, 0x%x(=%x)\n", regStr[rs], imm, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4);
                if( ((intmx)pM->reg.r[rs]) > 0 ){
                    UPDATE_PC_NEXT_WITH_DELAYED_IMM(pM, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4 );
                }else{
                    UPDATE_PC_NEXT(pM);
                    UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                }
                break;
            }else{
                UNKNOWN_INSTRUCTION("op=0x17");
            }
        case MIPS32_OP_J: // j
            jumpaddr    = (inst & 0x03ffffff);
            UPDATE_PC_NEXT_WITH_DELAYED_IMM( pM, ((pM->reg.pc+4) & ((~((uintmx)0x03ffffff))<<2)) + (jumpaddr<<2) );
            if(DEBUG) EXI_LOG_PRINTF("j 0x%x(=%x)\n", jumpaddr, pM->reg.pc_delay);
            break;
        case MIPS32_OP_JAL: // jal
            jumpaddr      = (inst & 0x03ffffff);
            pM->reg.r[31] = pM->reg.pc + 8;
            UPDATE_PC_NEXT_WITH_DELAYED_IMM( pM, ((pM->reg.pc+4) & ((~((uintmx)0x03ffffff))<<2)) + (jumpaddr<<2) );
            if(DEBUG) EXI_LOG_PRINTF("jal 0x%x(=%x)\n", jumpaddr, pM->reg.pc_delay);
            break;
        case MIPS32_OP_JALX: // jalx
            jumpaddr      = (inst & 0x03ffffff);
            pM->reg.r[31] = pM->reg.pc + 8;
            UPDATE_PC_NEXT_WITH_DELAYED_IMM( pM, ((pM->reg.pc+4) & ((~((uintmx)0x03ffffff))<<2)) + (jumpaddr<<2) + 1 );
            if(DEBUG) EXI_LOG_PRINTF("jalx 0x%x(=%x)\n", jumpaddr, pM->reg.pc_delay);
            break;
        case MIPS32_OP_REGIMM: // REGIMM
            switch( rt ){
                case 0x00: // bltz
                    if(DEBUG) EXI_LOG_PRINTF("bltz %s, 0x%x(=%x)\n", regStr[rs], imm, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4);
                    if( ((intmx)pM->reg.r[rs]) < 0 ){
                        UPDATE_PC_NEXT_WITH_DELAYED_IMM(pM, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4 );
                    }else{
                        UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                    }
                    break;
                case 0x02: // bltzl
                    if(DEBUG) EXI_LOG_PRINTF("bltzl %s, 0x%x(=%x)\n", regStr[rs], imm, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4);
                    if( ((intmx)pM->reg.r[rs]) < 0 ){
                        UPDATE_PC_NEXT_WITH_DELAYED_IMM(pM, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4 );
                    }else{
                        UPDATE_PC_NEXT(pM);
                        UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                    }
                    break;
                case 0x10: // bltzal
                    if(DEBUG) EXI_LOG_PRINTF("bltzal %s, 0x%x(=%x)\n", regStr[rs], imm, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4);
                    pM->reg.r[31] = pM->reg.pc + 8;
                    if( ((intmx)pM->reg.r[rs]) < 0 ){
                        UPDATE_PC_NEXT_WITH_DELAYED_IMM(pM, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4 );
                    }else{
                        UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                    }
                    break;
                case 0x01: // bgez
                    if(DEBUG) EXI_LOG_PRINTF("bgez %s, 0x%x(=%x)\n", regStr[rs], imm, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4);
                    if( ((intmx)pM->reg.r[rs]) >= 0 ){
                        UPDATE_PC_NEXT_WITH_DELAYED_IMM(pM, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4 );
                    }else{
                        UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                    }
                    break;
                case 0x03: // bgezl
                    if(DEBUG) EXI_LOG_PRINTF("bgezl %s, 0x%x(=%x)\n", regStr[rs], imm, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4);
                    if( ((intmx)pM->reg.r[rs]) >= 0 ){
                        UPDATE_PC_NEXT_WITH_DELAYED_IMM(pM, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4 );
                    }else{
                        UPDATE_PC_NEXT(pM);
                        UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                    }
                    break;
                case 0x11: // bal, bgezal
                    if( rs == 0 ){ //bal
                        if(DEBUG) EXI_LOG_PRINTF("bal 0x%x(=%x)\n", imm, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4);
                        pM->reg.r[31] = pM->reg.pc + 8;
                        UPDATE_PC_NEXT_WITH_DELAYED_IMM( pM, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4 );
                    }else{ // bgezal
                        if(DEBUG) EXI_LOG_PRINTF("bgezal %s, 0x%x(=%x)\n", regStr[rs], imm, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4);
                        pM->reg.r[31] = pM->reg.pc + 8;
                        if( ((intmx)pM->reg.r[rs]) >= 0 ){
                            UPDATE_PC_NEXT_WITH_DELAYED_IMM(pM, pM->reg.pc + (SIGN_EXT16(imm) << 2) + 4 );
                        }else{
                            UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                        }
                    }
                    break;
                case 0x08: // tgei (trap if greater or equal immediate)
                    if(DEBUG) EXI_LOG_PRINTF("tgei %s, 0x%x\n", regStr[rs], imm);
                    if( ((intmx)pM->reg.r[rs]) >= ((intmx)SIGN_EXT16(imm)) ){
                        prepareException(pM, EXCEPT_CODE_TRAP, 0);
                    }else{
                        UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                    }
                    break;
                case 0x09: // tgeiu (trap if greater or equal immediate unsigned)
                    if(DEBUG) EXI_LOG_PRINTF("tgeiu %s, 0x%x\n", regStr[rs], imm);
                    if( ((uintmx)pM->reg.r[rs]) >= ((uintmx)SIGN_EXT16(imm)) ){
                        prepareException(pM, EXCEPT_CODE_TRAP, 0);
                    }else{
                        UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                    }
                    break;
                case 0x0a: // tlti (trap if less than immediate)
                    if(DEBUG) EXI_LOG_PRINTF("tlti %s, 0x%x\n", regStr[rs], imm);
                    if( ((intmx)pM->reg.r[rs]) < ((intmx)SIGN_EXT16(imm)) ){
                        prepareException(pM, EXCEPT_CODE_TRAP, 0);
                    }else{
                        UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                    }
                    break;
                case 0x0b: // tltiu (trap if less than immediate unsigned)
                    if(DEBUG) EXI_LOG_PRINTF("tltiu %s, 0x%x\n", regStr[rs], imm);
                    if( ((uintmx)pM->reg.r[rs]) < ((uintmx)SIGN_EXT16(imm)) ){
                        prepareException(pM, EXCEPT_CODE_TRAP, 0);
                    }else{
                        UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                    }
                    break;
                case 0x0c: // teqi
                    if(DEBUG) EXI_LOG_PRINTF("teqi %s, 0x%x\n", regStr[rs], imm);
                    if( pM->reg.r[rs] == (intmx)SIGN_EXT16(imm) ){
                        prepareException(pM, EXCEPT_CODE_TRAP, 0);
                    }else{
                        UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                    }
                    break;
                case 0x0e: // tnei
                    if(DEBUG) EXI_LOG_PRINTF("tnei %s, 0x%x\n", regStr[rs], imm);
                    if( pM->reg.r[rs] != SIGN_EXT16(imm) ){
                        prepareException(pM, EXCEPT_CODE_TRAP, 0);
                    }else{
                        UPDATE_PC_NEXT_BRANCH_FAIL(pM);
                    }
                    break;
                case 0x1f:
                    if(DEBUG) EXI_LOG_PRINTF("synci 0x%x(%s)\n", imm, regStr[rs]);
                    UPDATE_PC_NEXT32(pM);
                default:
                    UNKNOWN_INSTRUCTION("op=0x01");
            }
            break;
        case MIPS32_OP_LB: // lb
            utmp = pM->reg.r[rs] + SIGN_EXT16(imm);
            if(DEBUG) EXI_LOG_PRINTF("lb %s, 0x%x(%s) (=0x%x)\n", regStr[rt], imm, regStr[rs], utmp);
            loadData = SIGN_EXT8( loadByte(pM, utmp, &accError) );
            if( accError ){
                prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
            }else{
                pM->reg.r[rt] = loadData;
                UPDATE_PC_NEXT(pM);
            }
            break;
        case MIPS32_OP_LBU: // lbu
            utmp = pM->reg.r[rs] + SIGN_EXT16(imm);
            if(DEBUG) EXI_LOG_PRINTF("lbu %s, 0x%x(%s) (=0x%x)\n", regStr[rt], imm, regStr[rs], utmp);
            loadData = loadByte(pM, utmp, &accError);
            if( accError ){
                prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
            }else{
                pM->reg.r[rt] = loadData;
                UPDATE_PC_NEXT(pM);
            }
            break;
        case MIPS32_OP_LH: // lh
            utmp = pM->reg.r[rs] + SIGN_EXT16(imm);
            if(DEBUG) EXI_LOG_PRINTF("lh %s, 0x%x(%s) (=0x%x)\n", regStr[rt], imm, regStr[rs], utmp);
            loadData = SIGN_EXT16(loadHalfWord(pM, utmp, &accError));
            if( accError ){
                prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
            }else{
                pM->reg.r[rt] = loadData;
                UPDATE_PC_NEXT(pM);
            }
            break;
        case MIPS32_OP_LHU: // lhu
            utmp = pM->reg.r[rs] + SIGN_EXT16(imm);
            if(DEBUG) EXI_LOG_PRINTF("lhu %s, 0x%x(%s) (=0x%x)\n", regStr[rt], imm, regStr[rs], utmp);
            loadData = ZERO_EXT16(loadHalfWord(pM, utmp, &accError));
            if( accError ){
                prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
            }else{
                pM->reg.r[rt] = loadData;
                UPDATE_PC_NEXT(pM);
            }
            break;
        case MIPS32_OP_LW: // lw
            utmp = pM->reg.r[rs] + SIGN_EXT16(imm);
            if(DEBUG) EXI_LOG_PRINTF("lw %s, 0x%x(%s) (=0x%x)\n", regStr[rt], imm, regStr[rs], utmp);
            loadData = loadWord(pM, utmp, &accError);
            if( accError ){
                prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
            }else{
                pM->reg.r[rt] = loadData;
                UPDATE_PC_NEXT(pM);
            }
            break;
        case MIPS32_OP_SB: // sb
            utmp = pM->reg.r[rs] + SIGN_EXT16(imm);
            if(DEBUG) EXI_LOG_PRINTF("sb %s, 0x%x(%s) (=0x%x)\n", regStr[rt], imm, regStr[rs], utmp);
            storeByte(pM, utmp, pM->reg.r[rt], &accError);
            if( accError ){
                prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
            }else{
                UPDATE_PC_NEXT(pM);
            }
            break;
        case MIPS32_OP_SH: // sh
            utmp = pM->reg.r[rs] + SIGN_EXT16(imm);
            if(DEBUG) EXI_LOG_PRINTF("sh %s, 0x%x(%s) (=0x%x)\n", regStr[rt], imm, regStr[rs], utmp);
            storeHalfWord(pM, utmp, 0xffff & pM->reg.r[rt], &accError);
            if( accError ){
                prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
            }else{
                UPDATE_PC_NEXT(pM);
            }
            break;
        case MIPS32_OP_SW: // sw
            utmp = pM->reg.r[rs] + SIGN_EXT16(imm);
            if(DEBUG) EXI_LOG_PRINTF("sw %s, 0x%x(%s) (=0x%x)\n", regStr[rt], imm, regStr[rs], utmp);
            storeWord(pM, utmp, pM->reg.r[rt], &accError);
            if( accError ){
                prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
            }else{
                UPDATE_PC_NEXT(pM);
            }
            break;
        case MIPS32_OP_LWL: // lwl
            utmp = pM->reg.r[rs] + SIGN_EXT16(imm);
            if(DEBUG) EXI_LOG_PRINTF("lwl %s, 0x%x(%s) (=0x%x)\n", regStr[rt], imm, regStr[rs], utmp);
            loadData = loadWord(pM, utmp & (~((uintmx)0x3)), &accError);
            if( accError ){
                prepareException(pM, GET_EXCEPT_CODE(accError), utmp & (~((uintmx)0x3)));
            }else{
                switch( utmp & 3 ){
                    // TODO: 32-bit big-endian specific implementation
                    case 0: pM->reg.r[rt] = loadData; break;
                    case 1: pM->reg.r[rt] =((pM->reg.r[rt] & 0x000000ff) | (loadData<< 8)); break;
                    case 2: pM->reg.r[rt] =((pM->reg.r[rt] & 0x0000ffff) | (loadData<<16)); break;
                    case 3: pM->reg.r[rt] =((pM->reg.r[rt] & 0x00ffffff) | (loadData<<24)); break;
                }
                UPDATE_PC_NEXT(pM);
            }
            break;
        case MIPS32_OP_LWR: // lwr
            utmp = pM->reg.r[rs] + SIGN_EXT16(imm);
            if(DEBUG) EXI_LOG_PRINTF("lwr %s, 0x%x(%s) (=0x%x)\n", regStr[rt], imm, regStr[rs], utmp);
            loadData = loadWord(pM, utmp & (~((uintmx)0x3)), &accError);
            if( accError ){
                prepareException(pM, GET_EXCEPT_CODE(accError), utmp & (~((uintmx)0x3)));
            }else{
                switch( utmp & 3 ){
                    // TODO: 32-bit big-endian specific implementation
                    case 0: pM->reg.r[rt] =((pM->reg.r[rt] & 0xffffff00) | ((loadData>>24)&0x000000ff)); break;
                    case 1: pM->reg.r[rt] =((pM->reg.r[rt] & 0xffff0000) | ((loadData>>16)&0x0000ffff)); break;
                    case 2: pM->reg.r[rt] =((pM->reg.r[rt] & 0xff000000) | ((loadData>> 8)&0x00ffffff)); break;
                    case 3: pM->reg.r[rt] = loadData; break;
                }
                UPDATE_PC_NEXT(pM);
            }
            break;
        case MIPS32_OP_SWL: //swl
            utmp = pM->reg.r[rs] + SIGN_EXT16(imm);
            if(DEBUG) EXI_LOG_PRINTF("swl %s, 0x%x(%s) (=0x%x)\n", regStr[rt], imm, regStr[rs], utmp);
            loadData = loadWord(pM, utmp & (~((uintmx)0x3)), &accError);
            if( accError ){
                prepareException(pM, GET_EXCEPT_CODE(accError), utmp & (~((uintmx)0x3)));
                break;
            }
            switch( utmp & 3 ){
                // TODO: 32-bit big-endian specific implementation
                case 0: loadData = pM->reg.r[rt]; break;
                case 1: loadData =((loadData & 0xff000000) | ((pM->reg.r[rt]>> 8)&0x00ffffff)); break;
                case 2: loadData =((loadData & 0xffff0000) | ((pM->reg.r[rt]>>16)&0x0000ffff)); break;
                case 3: loadData =((loadData & 0xffffff00) | ((pM->reg.r[rt]>>24)&0x000000ff)); break;
            }
            storeWord(pM, utmp & (~((uintmx)0x3)), loadData, &accError);
            if( accError ){
                prepareException(pM, GET_EXCEPT_CODE(accError), utmp & (~((uintmx)0x3)));
                // TODO: error handling code is necessary
            }else{
                UPDATE_PC_NEXT(pM);
            }
            break;
        case MIPS32_OP_SWR: // swr
            utmp = pM->reg.r[rs] + SIGN_EXT16(imm);
            if(DEBUG) EXI_LOG_PRINTF("swr %s, 0x%x(%s) (=0x%x)\n", regStr[rt], imm, regStr[rs], utmp);
            loadData = loadWord(pM, utmp & (~((uintmx)0x3)), &accError);
            if( accError ){
                prepareException(pM, GET_EXCEPT_CODE(accError), utmp & (~((uintmx)0x3)));
                break;
            }
            switch( utmp & 3 ){
                // TODO: 32-bit big-endian specific implementation
                case 0: loadData =((loadData & 0x00ffffff) | (pM->reg.r[rt]<<24)); break;
                case 1: loadData =((loadData & 0x0000ffff) | (pM->reg.r[rt]<<16)); break;
                case 2: loadData =((loadData & 0x000000ff) | (pM->reg.r[rt]<< 8)); break;
                case 3: loadData = pM->reg.r[rt]; break;
            }
            storeWord(pM, utmp & (~((uintmx)0x3)), loadData, &accError);
            if( accError ){
                prepareException(pM, GET_EXCEPT_CODE(accError), utmp & (~((uintmx)0x3)));
            }else{
                UPDATE_PC_NEXT(pM);
            }
            break;
        case MIPS32_OP_SC: // sc
            utmp = pM->reg.r[rs] + SIGN_EXT16(imm);
            if(DEBUG) EXI_LOG_PRINTF("sc %s, 0x%x(%s) (=0x%x)\n", regStr[rt], imm, regStr[rs], utmp);
            if( pM->reg.ll_sc ){
                storeWord(pM, utmp, pM->reg.r[rt], &accError);
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                    // no write to rt for retrying this instruction after exception handling
                }else{
                    pM->reg.r[rt] = 1;
                    UPDATE_PC_NEXT(pM);
                }
            }else{
                pM->reg.r[rt] = 0;
                UPDATE_PC_NEXT(pM);
            }
            break;
        case MIPS32_OP_LL: // ll
            utmp = pM->reg.r[rs] + SIGN_EXT16(imm);
            if(DEBUG) EXI_LOG_PRINTF("ll %s, 0x%x(%s) (=0x%x)\n", regStr[rt], imm, regStr[rs], utmp);
            pM->reg.ll_sc = 1;
            loadData = loadWord(pM, utmp, &accError);
            if( accError ){
                prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
            }else{
                pM->reg.r[rt] = loadData;
                UPDATE_PC_NEXT(pM);
            }
            break;
        case MIPS32_OP_LUI: // lui
            if(DEBUG) EXI_LOG_PRINTF("lui %s, 0x%x\n", regStr[rt], imm);
            pM->reg.r[rt] = (ZERO_EXT16(imm) << 16);
            UPDATE_PC_NEXT(pM);
            break;
        case MIPS32_OP_ORI: // ori
            if(DEBUG) EXI_LOG_PRINTF("ori %s, %s, 0x%x\n", regStr[rt], regStr[rs], imm);
            pM->reg.r[rt] = (pM->reg.r[rs] | ZERO_EXT16(imm));
            UPDATE_PC_NEXT(pM);
            break;
        case MIPS32_OP_XORI: // xori
            if(DEBUG) EXI_LOG_PRINTF("xori %s, %s, 0x%x\n", regStr[rt], regStr[rs], imm);
            pM->reg.r[rt] = (pM->reg.r[rs] ^ ZERO_EXT16(imm));
            UPDATE_PC_NEXT(pM);
            break;
        case MIPS32_OP_SLTI: // slti
            if(DEBUG) EXI_LOG_PRINTF("slti %s, %s, 0x%x\n", regStr[rt], regStr[rs], imm);
            pM->reg.r[rt] = (((intmx)pM->reg.r[rs]) < ((intmx)SIGN_EXT16(imm))) ? 1 : 0;
            UPDATE_PC_NEXT(pM);
            break;
        case MIPS32_OP_SLTIU: // sltiu
            if(DEBUG) EXI_LOG_PRINTF("sltiu %s, %s, 0x%x\n", regStr[rt], regStr[rs], imm);
            pM->reg.r[rt] = (((uintmx)pM->reg.r[rs]) < ((uintmx)SIGN_EXT16(imm))) ? 1 : 0;
            UPDATE_PC_NEXT(pM);
            break;
        case MIPS32_OP_SPECIAL2: // SPECIAL2
            switch( imm & 0x7ff ){
                case 0x02:
                    if(DEBUG) EXI_LOG_PRINTF("mul %s, %s, %s\n", regStr[rd], regStr[rs], regStr[rt]);
                    pM->reg.r[rd] = (((intmx)(pM->reg.r[rs])) * ((intmx)(pM->reg.r[rt])));
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x00:
                {
                    if(rd!=0){ UNKNOWN_INSTRUCTION("op=0x1c"); }
                    if(DEBUG) EXI_LOG_PRINTF("madd %s, %s\n", regStr[rs], regStr[rt]);
                    int64_t hilo = ((((uint64_t)pM->reg.hi)<<16)<<16) + ((uint64_t)pM->reg.lo);
                    int64_t tmul = (((int64_t)((intmx)pM->reg.r[rs])) * ((int64_t)((intmx)pM->reg.r[rt])));
                    tmul = hilo + tmul;
                    pM->reg.hi = ((tmul>>16)>>16);
                    pM->reg.lo = tmul;
                    UPDATE_PC_NEXT(pM);
                    break;
                }
                case 0x01:
                {
                    if(rd!=0){ UNKNOWN_INSTRUCTION("op=0x1c"); }
                    if(DEBUG) EXI_LOG_PRINTF("maddu %s, %s\n", regStr[rs], regStr[rt]);
                    uint64_t hilo = ((((uint64_t)pM->reg.hi)<<16)<<16) + ((uint64_t)pM->reg.lo);
                    uint64_t tmul = (((uint64_t)(pM->reg.r[rs])) * ((uint64_t)(pM->reg.r[rt])));
                    tmul = hilo + tmul;
                    pM->reg.hi = ((tmul>>16)>>16);
                    pM->reg.lo = tmul;
                    UPDATE_PC_NEXT(pM);
                    break;
                }
                case 0x04:
                {
                    if(rd!=0){ UNKNOWN_INSTRUCTION("op=0x1c"); }
                    if(DEBUG) EXI_LOG_PRINTF("msub %s, %s\n", regStr[rs], regStr[rt]);
                    int64_t hilo = ((((uint64_t)pM->reg.hi)<<16)<<16) + ((uint64_t)pM->reg.lo);
                    int64_t tmul = (((int64_t)((intmx)pM->reg.r[rs])) * ((int64_t)((intmx)pM->reg.r[rt])));
                    tmul = hilo - tmul;
                    pM->reg.hi = ((tmul>>16)>>16);
                    pM->reg.lo = tmul;
                    UPDATE_PC_NEXT(pM);
                    break;
                }
                case 0x05:
                {
                    if(rd!=0){ UNKNOWN_INSTRUCTION("op=0x1c"); }
                    if(DEBUG) EXI_LOG_PRINTF("msubu %s, %s\n", regStr[rs], regStr[rt]);
                    uint64_t hilo = ((((uint64_t)pM->reg.hi)<<16)<<16) + ((uint64_t)pM->reg.lo);
                    uint64_t tmul = (((uint64_t)(pM->reg.r[rs])) * ((uint64_t)(pM->reg.r[rt])));
                    tmul = hilo - tmul;
                    pM->reg.hi = ((tmul>>16)>>16);
                    pM->reg.lo = tmul;
                    UPDATE_PC_NEXT(pM);
                    break;
                }
                case 0x21:
                {
                    if(DEBUG) EXI_LOG_PRINTF("clo %s, %s\n", regStr[rd], regStr[rs]);
                    int temp = 32;
                    for(int cnt=31; cnt>=0; cnt--){
                        if( 0 == ((pM->reg.r[rs]) & (1<<cnt)) ){
                            temp = 31 - cnt;
                            break;
                        }
                    }
                    pM->reg.r[rd] = temp;
                    UPDATE_PC_NEXT(pM);
                    break;
                }
                case 0x20:
                {
                    if(DEBUG) EXI_LOG_PRINTF("clz %s, %s\n", regStr[rd], regStr[rs]);
                    int temp = 32;
                    for(int cnt=31; cnt>=0; cnt--){
                        if( (pM->reg.r[rs]) & (1<<cnt) ){
                            temp = 31 - cnt;
                            break;
                        }
                    }
                    pM->reg.r[rd] = temp;
                    UPDATE_PC_NEXT(pM);
                    break;
                }
                default:
                    UNKNOWN_INSTRUCTION("op=0x1c");
            }
            break;
        case MIPS32_OP_SPECIAL3:
            switch (funct){
                case 0x3b:
                    if( rs == 0 ){
                        if(DEBUG) EXI_LOG_PRINTF("rdhwr %s, 0x%x, 0x%x\n", regStr[rt], rd, shamt&0x7);

                        switch( rd ){
                            case  0: /* read CPU ID num */
                                if( MODE_IS_USER(C0_VAL(pM,C0_STATUS)) && !(C0_VAL(pM,C0_HWRENA) & (1<<C0_HWRENA_BIT_CPUNUM)) ){
                                    prepareException(pM, EXCEPT_CODE_RESERVED_INSTRUCTION, 0);
                                }else{
                                    pM->reg.r[rt] = 0;
                                    UPDATE_PC_NEXT(pM);
                                }
                                break;
                            case  1: /* Address step size to be used with the SYNCI instruction, or zero if no caches need be synchronized. */
                                if( MODE_IS_USER(C0_VAL(pM,C0_STATUS)) && !(C0_VAL(pM,C0_HWRENA) & (1<<C0_HWRENA_BIT_SYNCISTEP)) ){
                                    prepareException(pM, EXCEPT_CODE_RESERVED_INSTRUCTION, 0);
                                }else{
                                    pM->reg.r[rt] = 16; /* TODO: check this value */
                                    UPDATE_PC_NEXT(pM);
                                }
                                break;
                            case  2: /* High-resolution cycle counter. This register provides read access to the coprocessor 0 Count Register. */
                                if( MODE_IS_USER(C0_VAL(pM,C0_STATUS)) && !(C0_VAL(pM,C0_HWRENA) & (1<<C0_HWRENA_BIT_CC)) ){
                                    prepareException(pM, EXCEPT_CODE_RESERVED_INSTRUCTION, 0);
                                }else{
                                    pM->reg.r[rt] = loadC0CounterPrecise(pM);
                                    UPDATE_PC_NEXT(pM);
                                }
                                break;
                            case  3: /* Resolution of the CC register (CC register increments every second CPU cycle) */
                                if( MODE_IS_USER(C0_VAL(pM,C0_STATUS)) && !(C0_VAL(pM,C0_HWRENA) & (1<<C0_HWRENA_BIT_CCRES)) ){
                                    prepareException(pM, EXCEPT_CODE_RESERVED_INSTRUCTION, 0);
                                }else{
                                    pM->reg.r[rt] = CPU_FREQ_COUNT_RESOLUTION;
                                    UPDATE_PC_NEXT(pM);
                                }
                                break;
                            case 29: /* read C0_USERLOCAL Register */
                                if( MODE_IS_USER(C0_VAL(pM,C0_STATUS)) && !(C0_VAL(pM,C0_HWRENA) & (1<<C0_HWRENA_BIT_UL)) ){
                                    prepareException(pM, EXCEPT_CODE_RESERVED_INSTRUCTION, 0);
                                }else{
                                    pM->reg.r[rt] = C0_VAL(pM, C0_USERLOCAL);
                                    UPDATE_PC_NEXT(pM);
                                }
                                break;
                            case  4: pM->reg.r[rt] = 0; UPDATE_PC_NEXT(pM); break; /* Performance Counter Pair */
                            case  5: pM->reg.r[rt] = 0; UPDATE_PC_NEXT(pM); break; /* support for the Release 6 Paired LL/SC family of instructions */
                            default: prepareException(pM, EXCEPT_CODE_RESERVED_INSTRUCTION, 0); break;
                        }
                    }else{
                        UNKNOWN_INSTRUCTION("op=0x1f");
                    }
                    break;
                case 0x00:
                    if(DEBUG) EXI_LOG_PRINTF("ext %s, %s, 0x%x, 0x%x\n", regStr[rt], regStr[rs], shamt, rd+1);
                    pM->reg.r[rt] = (pM->reg.r[rs] >> shamt);
                    if( rd < 31 ){
                        pM->reg.r[rt] &= ((2<<rd)-1);
                    }
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x04:
                    if(DEBUG) EXI_LOG_PRINTF("ins %s, %s, 0x%x, 0x%x\n", regStr[rt], regStr[rs], shamt, rd+1-shamt);
                    utmp = (1<<((rd+1)-shamt)) -1;
                    utmp = (utmp << shamt);
                    if( shamt==0 && rd==31 ){
                        pM->reg.r[rt] = pM->reg.r[rs];
                    }else{
                        pM->reg.r[rt] = ( (pM->reg.r[rt] & (~utmp)) | ((pM->reg.r[rs] << shamt) & utmp) );
                    }
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x20:
                    if( shamt == 0x10 ){ // seb
                        if(DEBUG) EXI_LOG_PRINTF("seb %s, %s\n", regStr[rd], regStr[rt]);
                        pM->reg.r[rd] = (intmx)((int8_t)((uint8_t)(pM->reg.r[rt])));
                        UPDATE_PC_NEXT(pM);
                        break;
                    }else if( shamt == 0x18 ){ // seh
                        if(DEBUG) EXI_LOG_PRINTF("seh %s, %s\n", regStr[rd], regStr[rt]);
                        pM->reg.r[rd] = (intmx)((int16_t)((uint16_t)(pM->reg.r[rt])));
                        UPDATE_PC_NEXT(pM);
                        break;
                    }else if( shamt == 0x02 ){ // wsbh
                        if(DEBUG) EXI_LOG_PRINTF("wsbh %s, %s\n", regStr[rd], regStr[rt]);
                        pM->reg.r[rd] = (((pM->reg.r[rt] & 0xff00ff00)>>8) | ((pM->reg.r[rt] & 0x00ff00ff)<<8));
                        UPDATE_PC_NEXT(pM);
                        break;
                    }
                    // walk through
                default:
                    UNKNOWN_INSTRUCTION("op=0x1f");
            }
            break;
        case MIPS32_OP_COP0: // COP0
            // Availability of CP0 is checked.
            if( MODE_IS_USER( C0_VAL(pM, C0_STATUS) ) && (!(C0_VAL(pM, C0_STATUS)&(1<<C0_STATUS_BIT_CU0))) ){
                prepareException(pM, EXCEPT_CODE_COPROCESSOR_UNAVAIL, 0);
                break;
            }

            switch( rs ){
                case 0x04:
                    if(DEBUG) EXI_LOG_PRINTF("mtc0 %s, %d, %d\n", regStr[rt], rd, imm & ((1<<CP_SEL_BITS)-1) );
                    storeCP0(pM, rd, (imm & ((1<<CP_SEL_BITS)-1)), pM->reg.r[rt]);
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x00:
                    if(DEBUG) EXI_LOG_PRINTF("mfc0 %s, %d, %d\n", regStr[rt], rd, imm & ((1<<CP_SEL_BITS)-1) );
                    pM->reg.r[rt] = loadCP0(pM, rd, imm & ((1<<CP_SEL_BITS)-1));
                    UPDATE_PC_NEXT(pM);
                    break;
                case 0x0b:
                    if( imm == 0x6000 ){ // di
                        if(DEBUG) EXI_LOG_PRINTF("di %s\n", regStr[rt]);
                        utmp = loadCP0(pM, C0_STATUS_REG, C0_STATUS_SEL);
                        pM->reg.r[rt] = utmp; // NOTE THAT: RT could be zero if old value is not necessary
                        utmp &= (~(1<<C0_STATUS_BIT_IE));
                        storeCP0(pM, C0_STATUS_REG, C0_STATUS_SEL, utmp);
                        UPDATE_PC_NEXT(pM);
                    }else if( imm == 0x6020 ){ // ei
                        if(DEBUG) EXI_LOG_PRINTF("ei %s\n", regStr[rt]);
                        utmp = loadCP0(pM, C0_STATUS_REG, C0_STATUS_SEL);
                        pM->reg.r[rt] = utmp; // NOTE THAT: RT could be zero if old value is not necessary
                        utmp |= (1<<C0_STATUS_BIT_IE);
                        storeCP0(pM, C0_STATUS_REG, C0_STATUS_SEL, utmp);
                        UPDATE_PC_NEXT(pM);
                    }
                    break;
                case 0x10:
                    if( inst == 0x42000002 ){ // tlbwi
                        if(DEBUG) EXI_LOG_PRINTF("tlbwi\n");
                        TLBWriteIndex(pM);
                        UPDATE_PC_NEXT(pM);
                    }else if( inst == 0x42000006 ){  // tlbwr
                        if(DEBUG) EXI_LOG_PRINTF("tlbwr\n");
                        TLBWriteRandom(pM);
                        UPDATE_PC_NEXT(pM);
                    }else if( inst == 0x42000008 ){  // tlbp
                        if(DEBUG) EXI_LOG_PRINTF("tlbp\n");
                        TLBProbe(pM);
                        UPDATE_PC_NEXT(pM);
                    }else if( inst == 0x42000018 ){ // eret
                        if(DEBUG) EXI_LOG_PRINTF("eret\n");
                        if( C0_VAL(pM,C0_STATUS) & (1<<C0_STATUS_BIT_ERL) ){
                            REG_PC(pM) = C0_VAL(pM,C0_ERROREPC);
                            C0_VAL(pM,C0_STATUS) &= ~(1<<C0_STATUS_BIT_ERL);
                        }else{
                            REG_PC(pM) = C0_VAL(pM,C0_EPC);
                            C0_VAL(pM,C0_STATUS) &= ~(1<<C0_STATUS_BIT_EXL);
                        }
                        pM->reg.ll_sc = 0;
                        // ERET is a hazard barrier instruction. 
                        // However, it seems to be limited to hazards created by CP0 state changes.
                        // This inplementation requests no treatment for changing CP0 values. I think.

                        // TODO: processing of SRSCTL etc
                    }else if( inst == 0x42000020 ){
                        if(DEBUG) EXI_LOG_PRINTF("wait\n");

                        int64_t diff = pM->reg.c0_compare_long - loadC0CounterLong(pM);
                        if( diff > 0 ){
                            delayUSec(pM, SYSTEM_TIMER_INTERVAL_IN_USEC / 4 );
                        }
                        UPDATE_PC_NEXT(pM);
                    }else{
                        UNKNOWN_INSTRUCTION("op=0x10, coprocessor instruction");
                    }
                    break;
                default:
                    UNKNOWN_INSTRUCTION("op=0x10, coprocessor instruction");
            }
            break;
        case MIPS32_OP_CACHE: // cache
            if(DEBUG) EXI_LOG_PRINTF("cache 0x%x, 0x%x(%s)\n", rt, imm, regStr[rs]);
/*
            #if USE_RUNTIME_COMPILATION
                if( (rt&3) == 0 ){
                    if( rt == 0 ){
                        utmp = pM->reg.r[rs]+SIGN_EXT16(imm);
                        rc_invalidate( utmp );
                    }else{
                        if( imm == 0 ){
                            rc_clear();
                        }
                    }
                }
            #endif
*/
            UPDATE_PC_NEXT(pM);
            break;
        case MIPS32_OP_PREF: // pref
            if(DEBUG) EXI_LOG_PRINTF("pref 0x%x, 0x%x(%s)\n", rt, imm, regStr[rs]);
            // This function performs prefetch (overhead is large; it is removed)
            //getPhysicalAddress(pM, pM->reg.r[rs]+SIGN_EXT16(imm), 0, &accError);
            UPDATE_PC_NEXT(pM);
            break;
        default:
            UNKNOWN_INSTRUCTION("unhandled op");
    }

    return MIPS32R2_EXEC_OK;
}