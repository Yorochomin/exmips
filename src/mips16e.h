#include <stdio.h>
#include <stdint.h>
#include <signal.h>

#include "mips.h"
#include "ExInst_common.h"

#include "misc.h"
#include "logfile.h"
#include "mem.h"
#include "cp0.h"
#include "exception.h"
#include "mips_op.h"

#define MIPS16E_EXEC_FAIL -1
#define MIPS16E_EXEC_OK   0

#define LOGLEVEL_EMU_ERROR_MIPS16E  (LOGCAT_EMU | LOGLV_ERROR)


#define WITH_PREXIX(i)   ((i) & 0xffff0000)


#define UNKNOWN_INSTRUCTION16(additional_msg)   \
do{                                           \
    fprintf(stderr, "Unknown MIPS16 instruction (%s)\n", additional_msg);                                         \
    fprintf(stderr, "inst=0x%x op5=0x%x rx=0x%x ry=0x%x rz=0x%x sa3=0x%x funct5=0x%x\n", inst32, op5,ry,rx,rz,sa3, funct5);  \
    logfile_printf(LOGLEVEL_EMU_ERROR_MIPS16E, "Unknown instruction (%s)\n", additional_msg);                      \
    return MIPS16E_EXEC_FAIL;                                                                                      \
}while(0)


static inline int execMIPS16eInst(struct stMachineState *pM, uintmx inst32){

    uint8_t  op5  = ((inst32>>11) & 0x1f); /* inst[15:11] */
    uint8_t  rx   = ((inst32>> 8) & 0x07); /* inst[10: 8] */
    uint8_t  ry   = ((inst32>> 5) & 0x07); /* inst[ 7: 5] */
    uint8_t  rz   = ((inst32>> 2) & 0x07); /* inst[ 4: 2] */
    uint8_t  sa3  = ((inst32>> 2) & 0x07); /* inst[ 4: 2] */
    uint8_t funct5= ( inst32      & 0x1f); /* inst[ 5: 0] */
    uint8_t  imm8 = ( inst32      & 0xff); /* inst[ 7: 0] */

    uint16_t imm16 = (((inst32>>5)&(0x1f<<11)) | ((inst32>>16)&(0x3f<<5)) | funct5);
    uint16_t imm15 = (((inst32>>5)&(0x0f<<11)) | ((inst32>>16)&(0x7f<<4)) | (inst32&0xf));


    uintmx jumpaddr, loadData;
    uintmx utmp, utmp2;
    int accError;

    if( (inst32>>27) == MIPS16E_OP_JAL ){
        if( inst32 & (1<<26) ){
            jumpaddr = (inst32 & 0xffff);         // [16: 0]
            jumpaddr|= ((inst32>>21) & 0x1f)<<16; // [20:16]
            jumpaddr|= ((inst32>>16) & 0x1f)<<21; // [25:21]
            pM->reg.r[31] = pM->reg.pc + 6;
            UPDATE_PC_NEXT32_WITH_DELAYED_IMM( pM, ((pM->reg.pc+4) & ((~((uintmx)0x03ffffff))<<2)) + (jumpaddr<<2) );
            if(DEBUG) EXI_LOG_PRINTF("jalx 0x%x(=%x)\n", jumpaddr, pM->reg.pc_delay);
        }else{
            // JAL (which preserves the current ISA)
            jumpaddr = (inst32 & 0xffff);         // [16: 0]
            jumpaddr|= ((inst32>>21) & 0x1f)<<16; // [20:16]
            jumpaddr|= ((inst32>>16) & 0x1f)<<21; // [25:21]
            pM->reg.r[31] = pM->reg.pc + 6;
            // +1 is to keep execution in MIPS16e mode
            UPDATE_PC_NEXT32_WITH_DELAYED_IMM( pM, ((pM->reg.pc+4) & ((~((uintmx)0x03ffffff))<<2)) + (jumpaddr<<2) +1);
            if(DEBUG) EXI_LOG_PRINTF("jal 0x%x(=%x)\n", jumpaddr, pM->reg.pc_delay);
        }
        goto execMIPS16_exit;
    }

    switch( op5 ){
        case MIPS16E_OP_ADDIUSP:
            if( WITH_PREXIX(inst32) ){
                if(DEBUG) EXI_LOG_PRINTF("addiu %s, sp, 0x%x\n", regStr[XLAT(rx)], imm16);
                pM->reg.r[XLAT(rx)] = pM->reg.r[29] + SIGN_EXT16(imm16);
                UPDATE_PC_NEXT32(pM);
            }else{
                if(DEBUG) EXI_LOG_PRINTF("addiu %s, sp, 0x%x\n", regStr[XLAT(rx)], ZERO_EXT8(imm8)<<2);
                pM->reg.r[XLAT(rx)] = pM->reg.r[29] + (ZERO_EXT8(imm8)<<2);
                UPDATE_PC_NEXT16(pM);
            }
            break;
        case MIPS16E_OP_ADDIUPC:
            if( WITH_PREXIX(inst32) ){
                if(DEBUG) EXI_LOG_PRINTF("addiu %s, pc, 0x%x\n", regStr[XLAT(rx)], imm16);
                pM->reg.r[XLAT(rx)] = (pM->reg.pc & ~((uintmx)3)) + SIGN_EXT16(imm16);
                UPDATE_PC_NEXT32(pM);
            }else{
                if(DEBUG) EXI_LOG_PRINTF("addiu %s, pc, 0x%x\n", regStr[XLAT(rx)], ZERO_EXT8(imm8)<<2);
                utmp = ((pM->reg.delay_en ? pM->reg.pc_prev_jump : pM->reg.pc) & ~((uintmx)3)) + (ZERO_EXT8(imm8)<<2);
                pM->reg.r[XLAT(rx)] = utmp;
                UPDATE_PC_NEXT16(pM);
            }
            break;
        case MIPS16E_OP_B:
            if( WITH_PREXIX(inst32) ){
                if(DEBUG) EXI_LOG_PRINTF("b 0x%x(=%x)\n", imm16, pM->reg.pc + (SIGN_EXT16(imm16) << 1) + 4);
                UPDATE_PC_IMM(pM, pM->reg.pc + (SIGN_EXT16(imm16) << 1) + 4);
            }else{
                utmp = (inst32 & 0x7ff);
                if(DEBUG) EXI_LOG_PRINTF("b 0x%x(=%x)\n", imm8, pM->reg.pc + (SIGN_EXT11(utmp) << 1) + 2);
                UPDATE_PC_IMM(pM, pM->reg.pc + (SIGN_EXT11(utmp) << 1) + 2);
            }
            break;
        case MIPS16E_OP_JAL:
            /* not processed here */
            UNKNOWN_INSTRUCTION16("unhandled MIPS16 op (JAL)");
        case MIPS16E_OP_BEQZ:
            if( WITH_PREXIX(inst32) ){
                if(DEBUG) EXI_LOG_PRINTF("beqz %s, 0x%x(=%x)\n", regStr[XLAT(rx)], imm16, pM->reg.pc + (SIGN_EXT16(imm16) << 1) + 4);
                if( pM->reg.r[XLAT(rx)] == 0 ){
                    UPDATE_PC_IMM(pM, pM->reg.pc + (SIGN_EXT16(imm16) << 1) + 4);
                }else{
                    UPDATE_PC_NEXT32_BRANCH_FAIL(pM);
                }
            }else{
                if(DEBUG) EXI_LOG_PRINTF("beqz %s, 0x%x(=%x)\n", regStr[XLAT(rx)], imm8, pM->reg.pc + (SIGN_EXT8(imm8) << 1) + 2);
                if( pM->reg.r[XLAT(rx)] == 0 ){
                    UPDATE_PC_IMM(pM, pM->reg.pc + (SIGN_EXT8(imm8) << 1) + 2);
                }else{
                    UPDATE_PC_NEXT16_BRANCH_FAIL(pM);
                }
            }
            break;
        case MIPS16E_OP_BNEZ:
            if( WITH_PREXIX(inst32) ){
                if(DEBUG) EXI_LOG_PRINTF("bnez %s, 0x%x(=%x)\n", regStr[XLAT(rx)], imm16, pM->reg.pc + (SIGN_EXT16(imm16) << 1) + 4);
                if( pM->reg.r[XLAT(rx)] != 0 ){
                    UPDATE_PC_IMM(pM, pM->reg.pc + (SIGN_EXT16(imm16) << 1) + 4);
                }else{
                    UPDATE_PC_NEXT32_BRANCH_FAIL(pM);
                }
            }else{
                if(DEBUG) EXI_LOG_PRINTF("bnez %s, 0x%x(=%x)\n", regStr[XLAT(rx)], imm8, pM->reg.pc + (SIGN_EXT8(imm8) << 1) + 2);
                if( pM->reg.r[XLAT(rx)] != 0 ){
                    UPDATE_PC_IMM(pM, pM->reg.pc + (SIGN_EXT8(imm8) << 1) + 2);
                }else{
                    UPDATE_PC_NEXT16_BRANCH_FAIL(pM);
                }
            }
            break;
        case MIPS16E_OP_SHIFT:
            switch( funct5 & 3 ){
                case 0:/* SLL */
                    if( WITH_PREXIX(inst32) ){
                        int32_t sa = ((inst32>>22) & 0x1f);
                        if(DEBUG) EXI_LOG_PRINTF("sll %s, %s, 0x%x\n", regStr[XLAT(rx)], regStr[XLAT(ry)], sa);
                        pM->reg.r[XLAT(rx)] = (pM->reg.r[XLAT(ry)] << sa);
                        UPDATE_PC_NEXT32(pM);
                    }else{
                        int32_t sa = (sa3==0) ? 8 : sa3;
                        if(DEBUG) EXI_LOG_PRINTF("sll %s, %s, 0x%x\n", regStr[XLAT(rx)], regStr[XLAT(ry)], sa);
                        pM->reg.r[XLAT(rx)] = (pM->reg.r[XLAT(ry)] << sa);
                        UPDATE_PC_NEXT16(pM);
                    }
                    break;
                case 2: /* SRL */
                    if( WITH_PREXIX(inst32) ){
                        int32_t sa = ((inst32>>22) & 0x1f);
                        if(DEBUG) EXI_LOG_PRINTF("srl %s, %s, 0x%x\n", regStr[XLAT(rx)], regStr[XLAT(ry)], sa);
                        pM->reg.r[XLAT(rx)] = (((uint64_t)((uintmx)pM->reg.r[XLAT(ry)])) >> sa);
                        UPDATE_PC_NEXT32(pM);
                    }else{
                        int32_t sa = (sa3==0) ? 8 : sa3;
                        if(DEBUG) EXI_LOG_PRINTF("srl %s, %s, 0x%x\n", regStr[XLAT(rx)], regStr[XLAT(ry)], sa);
                        pM->reg.r[XLAT(rx)] = (((uint64_t)((uintmx)pM->reg.r[XLAT(ry)])) >> sa);
                        UPDATE_PC_NEXT16(pM);
                    }
                    break;
                case 3: /* SRA */
                    if( WITH_PREXIX(inst32) ){
                        int32_t sa = ((inst32>>22) & 0x1f);
                        if(DEBUG) EXI_LOG_PRINTF("sra %s, %s, 0x%x\n", regStr[XLAT(rx)], regStr[XLAT(ry)], sa);
                        pM->reg.r[XLAT(rx)] = (((int64_t)((intmx)pM->reg.r[XLAT(ry)])) >> sa);
                        UPDATE_PC_NEXT32(pM);
                    }else{
                        int32_t sa = (sa3==0) ? 8 : sa3;
                        if(DEBUG) EXI_LOG_PRINTF("sra %s, %s, 0x%x\n", regStr[XLAT(rx)], regStr[XLAT(ry)], sa);
                        pM->reg.r[XLAT(rx)] = (((int64_t)((intmx)pM->reg.r[XLAT(ry)])) >> sa);
                        UPDATE_PC_NEXT16(pM);
                    }
                    break;
                default:
                    UNKNOWN_INSTRUCTION16("unhandled MIPS16 op (SHIFT)");
                    break;
            }
            break;
        case MIPS16E_OP_RRIA:
            if( inst32 & (1<<4) ){
                UNKNOWN_INSTRUCTION16("unhandled MIPS16 op (RRIA)");
            }else{
                if( WITH_PREXIX(inst32) ){
                    if(DEBUG) EXI_LOG_PRINTF("addiu %s, %s, 0x%x\n", regStr[XLAT(ry)], regStr[XLAT(rx)], imm15);
                    pM->reg.r[XLAT(ry)] = pM->reg.r[XLAT(rx)] + SIGN_EXT15(imm15);
                    UPDATE_PC_NEXT32(pM);
                }else{
                    if(DEBUG) EXI_LOG_PRINTF("addiu %s, %s, 0x%x\n", regStr[XLAT(ry)], regStr[XLAT(rx)], inst32&0xf);
                    pM->reg.r[XLAT(ry)] = pM->reg.r[XLAT(rx)] + SIGN_EXT4(inst32 & 0xf);
                    UPDATE_PC_NEXT16(pM);
                }
            }
            break;
        case MIPS16E_OP_ADDIU8:
            if( WITH_PREXIX(inst32) ){
                if(DEBUG) EXI_LOG_PRINTF("addiu %s, 0x%x\n", regStr[XLAT(rx)], imm16);
                pM->reg.r[XLAT(rx)] += SIGN_EXT16(imm16);
                UPDATE_PC_NEXT32(pM);
            }else{
                if(DEBUG) EXI_LOG_PRINTF("addiu %s, 0x%x\n", regStr[XLAT(rx)], imm8);
                pM->reg.r[XLAT(rx)] += SIGN_EXT8(imm8);
                UPDATE_PC_NEXT16(pM);
            }
            break;
        case MIPS16E_OP_SLTI:
            if( WITH_PREXIX(inst32) ){
                if(DEBUG) EXI_LOG_PRINTF("slti %s, 0x%x\n", regStr[XLAT(rx)], imm16);
                pM->reg.r[24] = (((intmx)pM->reg.r[XLAT(rx)]) < ((intmx)SIGN_EXT16(imm16))) ? 1 : 0;
                UPDATE_PC_NEXT32(pM);
            }else{
                if(DEBUG) EXI_LOG_PRINTF("slti %s, 0x%x\n", regStr[XLAT(rx)], imm8);
                pM->reg.r[24] = (((intmx)pM->reg.r[XLAT(rx)]) < ((intmx)ZERO_EXT8(imm8))) ? 1 : 0;
                UPDATE_PC_NEXT16(pM);
            }
            break;
        case MIPS16E_OP_SLTIU:
            if( WITH_PREXIX(inst32) ){
                if(DEBUG) EXI_LOG_PRINTF("sltiu %s, 0x%x\n", regStr[XLAT(rx)], imm16);
                pM->reg.r[24] = (pM->reg.r[XLAT(rx)] < ((uintmx)SIGN_EXT16(imm16))) ? 1 : 0;
                UPDATE_PC_NEXT32(pM);
            }else{
                if(DEBUG) EXI_LOG_PRINTF("sltiu %s, 0x%x\n", regStr[XLAT(rx)], imm8);
                pM->reg.r[24] = (pM->reg.r[XLAT(rx)] < ZERO_EXT8(imm8)) ? 1 : 0;
                UPDATE_PC_NEXT16(pM);
            }
            break;
        case MIPS16E_OP_I8:
            switch( rx ){
                case 0:
                    if( WITH_PREXIX(inst32) ){
                        if(DEBUG) EXI_LOG_PRINTF("bteqz 0x%x(=%x)\n", imm16, pM->reg.pc + (SIGN_EXT16(imm16) << 1) + 4);
                        if( pM->reg.r[24] == 0 ){
                            UPDATE_PC_IMM(pM, pM->reg.pc + (SIGN_EXT16(imm16) << 1) + 4);
                        }else{
                            UPDATE_PC_NEXT32_BRANCH_FAIL(pM);
                        }
                    }else{
                        if(DEBUG) EXI_LOG_PRINTF("bteqz 0x%x(=%x)\n", imm8, pM->reg.pc + (SIGN_EXT8(imm8) << 1) + 2);
                        if( pM->reg.r[24] == 0 ){
                            UPDATE_PC_IMM(pM, pM->reg.pc + (SIGN_EXT8(imm8) << 1) + 2);
                        }else{
                            UPDATE_PC_NEXT16_BRANCH_FAIL(pM);
                        }
                    }
                    break;
                case 1:
                    if( WITH_PREXIX(inst32) ){
                        if(DEBUG) EXI_LOG_PRINTF("btnez 0x%x(=%x)\n", imm16, pM->reg.pc + (SIGN_EXT16(imm16) << 1) + 4);
                        if( pM->reg.r[24] != 0 ){
                            UPDATE_PC_IMM(pM, pM->reg.pc + (SIGN_EXT16(imm16) << 1) + 4);
                        }else{
                            UPDATE_PC_NEXT32_BRANCH_FAIL(pM);
                        }
                    }else{
                        if(DEBUG) EXI_LOG_PRINTF("btnez 0x%x(=%x)\n", imm8, pM->reg.pc + (SIGN_EXT8(imm8) << 1) + 2);
                        if( pM->reg.r[24] != 0 ){
                            UPDATE_PC_IMM(pM, pM->reg.pc + (SIGN_EXT8(imm8) << 1) + 2);
                        }else{
                            UPDATE_PC_NEXT16_BRANCH_FAIL(pM);
                        }
                    }
                    break;
                case 2: /* SWRASP */
                    if( WITH_PREXIX(inst32) ){
                        utmp = pM->reg.r[29] + SIGN_EXT16(imm16);
                        if(DEBUG) EXI_LOG_PRINTF("sw ra, 0x%x(sp) (=0x%x)\n", imm16, utmp);
                        storeWord(pM, utmp, pM->reg.r[31], &accError);
                        if( accError ){
                            prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                        }else{
                            UPDATE_PC_NEXT32(pM);
                        }
                    }else{
                        utmp = pM->reg.r[29] + (ZERO_EXT8(imm8)<<2);
                        if(DEBUG) EXI_LOG_PRINTF("sw ra, 0x%x(sp) (=0x%x)\n", (ZERO_EXT8(imm8)<<2), utmp);
                        storeWord(pM, utmp, pM->reg.r[31], &accError);
                        if( accError ){
                            prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                        }else{
                            UPDATE_PC_NEXT16(pM);
                        }
                    }
                    break;
                case 3: /* ADJSP */
                    if( WITH_PREXIX(inst32) ){
                        if(DEBUG) EXI_LOG_PRINTF("addiu sp, 0x%x\n", imm16);
                        pM->reg.r[29] += SIGN_EXT16(imm16);
                        UPDATE_PC_NEXT32(pM);
                    }else{
                        if(DEBUG) EXI_LOG_PRINTF("addiu sp, 0x%x\n", SIGN_EXT8(imm8)<<3);
                        pM->reg.r[29] += (SIGN_EXT8(imm8)<<3);
                        UPDATE_PC_NEXT16(pM);
                    }
                    break;
                case 4:
                    if( inst32 & (1<<7) ){
                        // save function
                        uint16_t fs = (inst32 & 0x0f);  // framesize
                        uint8_t  s1 = ((inst32>>4) & 1);
                        uint8_t  s0 = ((inst32>>5) & 1);
                        uint8_t  ra = ((inst32>>6) & 1);

                        if( WITH_PREXIX(inst32) ){
                            // SAVE (Extended)
                            uint8_t aregs  = ((inst32>>16) & 0xf);
                            uint8_t xsregs = ((inst32>>24) & 0x7);
                            fs |= ((inst32>>16) & 0xf0);

                            if(DEBUG) EXI_LOG_PRINTF("save ra=%d, xsregs=%d, aregs=%d, s0=%d, s1=%d, framesize=%d\n", ra, xsregs, aregs, s0,s1,fs);

                            utmp = utmp2 = pM->reg.r[29];
                            uint32_t args;
                            switch(aregs){
                                case 0: case 1: case 2: case 3: case 11: args = 0; break;
                                case 4: case 5: case 6: case 7: args = 1; break;
                                case 8: case 9: case 10: args = 2; break;
                                case 12: case 13: args = 3; break;
                                case 14: args = 4; break;
                                default: args = 0; break;
                            }
                            for( int idx = 0;  idx < args; idx++ ){
                                storeWord(pM, utmp+(idx<<2), pM->reg.r[4+idx], &accError);
                                if( accError ){
                                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp+(idx<<2));
                                    goto execMIPS16_exit;
                                }
                            }
                            if( ra ){
                                utmp -= 4;
                                storeWord(pM, utmp, pM->reg.r[31], &accError);
                                if( accError ){
                                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                                    goto execMIPS16_exit;
                                }
                            }
                            for(int idx = xsregs;  idx > 0; idx-- ){
                                int regnum = (idx==7) ? 30 : 18 + (idx-1);
                                utmp -= 4;
                                storeWord(pM, utmp, pM->reg.r[regnum], &accError);
                                if( accError ){
                                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                                    goto execMIPS16_exit;
                                }
                            }
                            if( s1 ){
                                utmp -= 4;
                                storeWord(pM, utmp, pM->reg.r[17], &accError);
                                if( accError ){
                                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                                    goto execMIPS16_exit;
                                }
                            }
                            if( s0 ){
                                utmp -= 4;
                                storeWord(pM, utmp, pM->reg.r[16], &accError);
                                if( accError ){
                                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                                    goto execMIPS16_exit;
                                }
                            }
                            uint32_t astatic;
                            switch(aregs){
                                case 0: case 4: case 8: case 12: case 14: astatic = 0; break;
                                case 1: case 5: case 9: case 13: astatic = 1; break;
                                case 2: case 6: case 10: astatic = 2; break;
                                case 3: case 7: astatic = 3; break;
                                case 11: astatic = 4; break;
                                default: astatic = 0; break;
                            }
                            for( int idx = 0;  idx < astatic; idx++ ){
                                utmp -= 4;
                                storeWord(pM, utmp, pM->reg.r[7-idx], &accError);
                                if( accError ){
                                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                                    goto execMIPS16_exit;
                                }
                            }
                            utmp = utmp2 - (fs<<3);
                            pM->reg.r[29] = utmp;

                            UPDATE_PC_NEXT32_BRANCH_FAIL(pM);
                        }else{
                            // SAVE (non-extended)
                            if(DEBUG) EXI_LOG_PRINTF("save ra=%d, s0=%d, s1=%d, framesize=%d\n", ra,s0,s1,fs);

                            utmp = utmp2 = pM->reg.r[29];

                            if( ra ){
                                utmp -= 4;
                                storeWord(pM, utmp, pM->reg.r[31], &accError);
                                if( accError ){
                                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                                    goto execMIPS16_exit;
                                }
                            }
                            if( s1 ){
                                utmp -= 4;
                                storeWord(pM, utmp, pM->reg.r[17], &accError);
                                if( accError ){
                                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                                    goto execMIPS16_exit;
                                }
                            }
                            if( s0 ){
                                utmp -= 4;
                                storeWord(pM, utmp, pM->reg.r[16], &accError);
                                if( accError ){
                                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                                    goto execMIPS16_exit;
                                }
                            }
                            if( fs == 0 ){
                                utmp = utmp2 - 128;
                            }else{
                                utmp = utmp2 - (fs<<3);
                            }
                            pM->reg.r[29] = utmp;

                            UPDATE_PC_NEXT16_BRANCH_FAIL(pM);
                        }
                        break;
                    }else{
                        // restore function
                        uint16_t fs = (inst32 & 0x0f);  // framesize
                        uint8_t  s1 = ((inst32>>4) & 1);
                        uint8_t  s0 = ((inst32>>5) & 1);
                        uint8_t  ra = ((inst32>>6) & 1);

                        if( WITH_PREXIX(inst32) ){
                            // restore (Extended)

                            uint8_t aregs  = ((inst32>>16) & 0xf);
                            uint8_t xsregs = ((inst32>>24) & 0x7);
                            fs |= ((inst32>>16) & 0xf0);

                            if(DEBUG) EXI_LOG_PRINTF("restore ra=%d, xsregs=%d, aregs=%d, s0=%d, s1=%d, framesize=%d\n", ra, xsregs, aregs, s0,s1,fs);

                            utmp2 = utmp = pM->reg.r[29] + (fs <<3);

                            if( ra ){
                                utmp -= 4;
                                loadData = loadWord(pM, utmp, &accError);
                                if( accError ){
                                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                                    goto execMIPS16_exit;
                                }else{
                                    pM->reg.r[31] = loadData;
                                }
                            }
                            for(int idx = xsregs; idx > 0; idx-- ){
                                int regnum = (idx==7) ? 30 : 18 + (idx-1);
                                utmp -= 4;
                                loadData = loadWord(pM, utmp, &accError);
                                if( accError ){
                                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                                    goto execMIPS16_exit;
                                }else{
                                    pM->reg.r[regnum] = loadData;
                                }
                            }
                            if( s1 ){
                                utmp -= 4;
                                loadData = loadWord(pM, utmp, &accError);
                                if( accError ){
                                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                                    goto execMIPS16_exit;
                                }else{
                                    pM->reg.r[17] = loadData;
                                }
                            }
                            if( s0 ){
                                utmp -= 4;
                                loadData = loadWord(pM, utmp, &accError);
                                if( accError ){
                                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                                    goto execMIPS16_exit;
                                }else{
                                    pM->reg.r[16] = loadData;
                                }
                            }
                            uint32_t astatic;
                            switch(aregs){
                                case 0: case 4: case 8: case 12: case 14: astatic = 0; break;
                                case 1: case 5: case 9: case 13: astatic = 1; break;
                                case 2: case 6: case 10: astatic = 2; break;
                                case 3: case 7: astatic = 3; break;
                                case 11: astatic = 4; break;
                                default: astatic = 0; break;
                            }
                            for( int idx = 0;  idx < astatic; idx++ ){
                                utmp -= 4;
                                loadData = loadWord(pM, utmp, &accError);
                                if( accError ){
                                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                                    goto execMIPS16_exit;
                                }else{
                                    pM->reg.r[7-idx] = loadData;
                                }
                            }
                            pM->reg.r[29] = utmp2;

                            UPDATE_PC_NEXT32(pM);
                        }else{
                            // restore (non-extended)

                            if(DEBUG) EXI_LOG_PRINTF("restore ra=%d, s0=%d, s1=%d, framesize=%d\n", ra,s0,s1,fs);

                            if( fs == 0 ){
                                utmp = pM->reg.r[29] + 128;
                            }else{
                                utmp = pM->reg.r[29] + (fs <<3);
                            }
                            utmp2 = utmp;

                            if( ra ){
                                utmp -= 4;
                                loadData = loadWord(pM, utmp, &accError);
                                if( accError ){
                                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                                    goto execMIPS16_exit;
                                }else{
                                    pM->reg.r[31] = loadData;
                                }
                            }
                            if( s1 ){
                                utmp -= 4;
                                loadData = loadWord(pM, utmp, &accError);
                                if( accError ){
                                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                                    goto execMIPS16_exit;
                                }else{
                                    pM->reg.r[17] = loadData;
                                }
                            }
                            if( s0 ){
                                utmp -= 4;
                                loadData = loadWord(pM, utmp, &accError);
                                if( accError ){
                                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                                    goto execMIPS16_exit;
                                }else{
                                    pM->reg.r[16] = loadData;
                                }
                            }
                            pM->reg.r[29] = utmp2;

                            UPDATE_PC_NEXT16(pM);
                        }
                        break;
                    }
                case 5:
                    // This instruction has no extended version
                    utmp = (ry | (funct5&0x18));
                    utmp2= (funct5 & 0x7);
                    if(DEBUG) EXI_LOG_PRINTF("move %s, %s\n", regStr[utmp], regStr[XLAT(utmp2)]);
                    pM->reg.r[utmp] = pM->reg.r[XLAT(utmp2)];
                    UPDATE_PC_NEXT16(pM);
                    break;
                case 7:
                    // This instruction has no extended version
                    if(DEBUG) EXI_LOG_PRINTF("move %s, %s\n", regStr[XLAT(ry)], regStr[funct5]);
                    pM->reg.r[XLAT(ry)] = pM->reg.r[funct5];
                    UPDATE_PC_NEXT16(pM);
                    break;
                default:
                    UNKNOWN_INSTRUCTION16("unhandled MIPS16 op (I8)");
            }
            break;
        case MIPS16E_OP_LI:
            if( WITH_PREXIX(inst32) ){
                if(DEBUG) EXI_LOG_PRINTF("li %s, 0x%x\n", regStr[XLAT(rx)], imm16);
                pM->reg.r[XLAT(rx)] = ZERO_EXT16(imm16);
                UPDATE_PC_NEXT32(pM);
            }else{
                if(DEBUG) EXI_LOG_PRINTF("li %s, 0x%x\n", regStr[XLAT(rx)], imm8);
                pM->reg.r[XLAT(rx)] = ZERO_EXT8(imm8);
                UPDATE_PC_NEXT16(pM);
            }
            break;
        case MIPS16E_OP_CMPI:
            if( WITH_PREXIX(inst32) ){
                if(DEBUG) EXI_LOG_PRINTF("cmpi %s, 0x%x\n", regStr[XLAT(rx)], imm16);
                pM->reg.r[24] = (pM->reg.r[XLAT(rx)] ^ ZERO_EXT16(imm16));
                UPDATE_PC_NEXT32(pM);
            }else{
                if(DEBUG) EXI_LOG_PRINTF("cmpi %s, 0x%x\n", regStr[XLAT(rx)], imm8);
                pM->reg.r[24] = (pM->reg.r[XLAT(rx)] ^ ZERO_EXT8(imm8));
                UPDATE_PC_NEXT16(pM);
            }
            break;
        case MIPS16E_OP_LB:
            if( WITH_PREXIX(inst32) ){
                utmp = pM->reg.r[XLAT(rx)] + SIGN_EXT16(imm16);
                if(DEBUG) EXI_LOG_PRINTF("lb %s, 0x%x(%s) (=0x%x)\n", regStr[XLAT(ry)], imm16, regStr[XLAT(rx)], utmp);
                loadData = (intmx)((int8_t)loadByte(pM, utmp, &accError));
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    pM->reg.r[XLAT(ry)] = loadData;
                    UPDATE_PC_NEXT32(pM);
                }
            }else{
                utmp = pM->reg.r[XLAT(rx)] + funct5;
                if(DEBUG) EXI_LOG_PRINTF("lb %s, 0x%x(%s) (=0x%x)\n", regStr[XLAT(ry)], funct5, regStr[XLAT(rx)], utmp);
                loadData = (intmx)((int8_t)loadByte(pM, utmp, &accError));
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    pM->reg.r[XLAT(ry)] = loadData;
                    UPDATE_PC_NEXT16(pM);
                }
            }
            break;
        case MIPS16E_OP_LH:
            if( WITH_PREXIX(inst32) ){
                utmp = pM->reg.r[XLAT(rx)] + SIGN_EXT16(imm16);
                if(DEBUG) EXI_LOG_PRINTF("lh %s, 0x%x(%s) (=0x%x)\n", regStr[XLAT(ry)], imm16, regStr[XLAT(rx)], utmp);
                loadData = (intmx)((int16_t)loadHalfWord(pM, utmp, &accError));
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    pM->reg.r[XLAT(ry)] = loadData;
                    UPDATE_PC_NEXT32(pM);
                }
            }else{
                utmp = pM->reg.r[XLAT(rx)] + (funct5<<1);
                if(DEBUG) EXI_LOG_PRINTF("lh %s, 0x%x(%s) (=0x%x)\n", regStr[XLAT(ry)], funct5<<1, regStr[XLAT(rx)], utmp);
                loadData = (intmx)((int16_t)loadHalfWord(pM, utmp, &accError));
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    pM->reg.r[XLAT(ry)] = loadData;
                    UPDATE_PC_NEXT16(pM);
                }
            }
            break;
        case MIPS16E_OP_LWSP:
            if( WITH_PREXIX(inst32) ){
                utmp = pM->reg.r[29] + SIGN_EXT16(imm16);
                if(DEBUG) EXI_LOG_PRINTF("lw %s, 0x%x(sp) (=0x%x)\n", regStr[XLAT(rx)], imm16, utmp);
                loadData = loadWord(pM, utmp, &accError);
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    pM->reg.r[XLAT(rx)] = loadData;
                    UPDATE_PC_NEXT32(pM);
                }
            }else{
                utmp = pM->reg.r[29] + (ZERO_EXT8(imm8)<<2);
                if(DEBUG) EXI_LOG_PRINTF("lw %s, 0x%x(sp) (=0x%x)\n", regStr[XLAT(rx)], (ZERO_EXT8(imm8)<<2), utmp);
                loadData = loadWord(pM, utmp, &accError);
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    pM->reg.r[XLAT(rx)] = loadData;
                    UPDATE_PC_NEXT16(pM);
                }
            }
            break;
        case MIPS16E_OP_LW:
            if( WITH_PREXIX(inst32) ){
                utmp = pM->reg.r[XLAT(rx)] + SIGN_EXT16(imm16);
                if(DEBUG) EXI_LOG_PRINTF("lw %s, 0x%x(%s) (=0x%x)\n", regStr[XLAT(ry)], imm16, regStr[XLAT(rx)], utmp);
                loadData = loadWord(pM, utmp, &accError);
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    pM->reg.r[XLAT(ry)] = loadData;
                    UPDATE_PC_NEXT32(pM);
                }
            }else{
                utmp = pM->reg.r[XLAT(rx)] + (funct5<<2);
                if(DEBUG) EXI_LOG_PRINTF("lw %s, 0x%x(%s) (=0x%x)\n", regStr[XLAT(ry)], funct5<<2, regStr[XLAT(rx)], utmp);
                loadData = loadWord(pM, utmp, &accError);
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    pM->reg.r[XLAT(ry)] = loadData;
                    UPDATE_PC_NEXT16(pM);
                }
            }
            break;
        case MIPS16E_OP_LBU:
            if( WITH_PREXIX(inst32) ){
                utmp = pM->reg.r[XLAT(rx)] + SIGN_EXT16(imm16);
                if(DEBUG) EXI_LOG_PRINTF("lbu %s, 0x%x(%s) (=0x%x)\n", regStr[XLAT(ry)], imm16, regStr[XLAT(rx)], utmp);
                loadData = loadByte(pM, utmp, &accError);
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    pM->reg.r[XLAT(ry)] = loadData;
                    UPDATE_PC_NEXT32(pM);
                }
            }else{
                utmp = pM->reg.r[XLAT(rx)] + funct5;
                if(DEBUG) EXI_LOG_PRINTF("lbu %s, 0x%x(%s) (=0x%x)\n", regStr[XLAT(ry)], funct5, regStr[XLAT(rx)], utmp);
                loadData = loadByte(pM, utmp, &accError);
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    pM->reg.r[XLAT(ry)] = loadData;
                    UPDATE_PC_NEXT16(pM);
                }
            }
            break;
        case MIPS16E_OP_LHU:
            if( WITH_PREXIX(inst32) ){
                utmp = pM->reg.r[XLAT(rx)] + SIGN_EXT16(imm16);
                if(DEBUG) EXI_LOG_PRINTF("lhu %s, 0x%x(%s) (=0x%x)\n", regStr[XLAT(ry)], imm16, regStr[XLAT(rx)], utmp);
                loadData = (loadHalfWord(pM, utmp, &accError) & 0xffff);
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    pM->reg.r[XLAT(ry)] = loadData;
                    UPDATE_PC_NEXT32(pM);
                }
            }else{
                utmp = pM->reg.r[XLAT(rx)] + (funct5<<1);
                if(DEBUG) EXI_LOG_PRINTF("lhu %s, 0x%x(%s) (=0x%x)\n", regStr[XLAT(ry)], funct5<<1, regStr[XLAT(rx)], utmp);
                loadData = (loadHalfWord(pM, utmp, &accError) & 0xffff);
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    pM->reg.r[XLAT(ry)] = loadData;
                    UPDATE_PC_NEXT16(pM);
                }
            }
            break;
        case MIPS16E_OP_LWPC:
            if( WITH_PREXIX(inst32) ){
                utmp = (pM->reg.pc & ~((uintmx)3)) + SIGN_EXT16(imm16);
                if(DEBUG) EXI_LOG_PRINTF("lw %s, 0x%x(pc) (=0x%x)\n", regStr[XLAT(rx)], imm16, utmp);
                loadData = loadWord(pM, utmp, &accError);
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    pM->reg.r[XLAT(rx)] = loadData;
                    UPDATE_PC_NEXT32(pM);
                }
            }else{
                utmp = ((pM->reg.delay_en ? pM->reg.pc_prev_jump : pM->reg.pc) & ~((uintmx)3)) + (ZERO_EXT8(imm8)<<2);
                if(DEBUG) EXI_LOG_PRINTF("lw %s, 0x%x(pc) (=0x%x)\n", regStr[XLAT(rx)], (ZERO_EXT8(imm8)<<2), utmp);
                loadData = loadWord(pM, utmp, &accError);
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    pM->reg.r[XLAT(rx)] = loadData;
                    UPDATE_PC_NEXT16(pM);
                }
            }
            break;
        case MIPS16E_OP_SB:
            if( WITH_PREXIX(inst32) ){
                utmp = pM->reg.r[XLAT(rx)] + SIGN_EXT16(imm16);
                if(DEBUG) EXI_LOG_PRINTF("sb %s, 0x%x(%s) (=0x%x)\n", regStr[XLAT(ry)], imm16, regStr[XLAT(rx)], utmp);
                storeByte(pM, utmp, pM->reg.r[XLAT(ry)], &accError);
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    UPDATE_PC_NEXT32(pM);
                }
            }else{
                utmp = pM->reg.r[XLAT(rx)] + funct5;
                if(DEBUG) EXI_LOG_PRINTF("sb %s, 0x%x(%s) (=0x%x)\n", regStr[XLAT(ry)], funct5, regStr[XLAT(rx)], utmp);
                storeByte(pM, utmp, pM->reg.r[XLAT(ry)], &accError);
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    UPDATE_PC_NEXT16(pM);
                }
            }
            break;
        case MIPS16E_OP_SH:
            if( WITH_PREXIX(inst32) ){
                utmp = pM->reg.r[XLAT(rx)] + SIGN_EXT16(imm16);
                if(DEBUG) EXI_LOG_PRINTF("sh %s, 0x%x(%s) (=0x%x)\n", regStr[XLAT(ry)], imm16, regStr[XLAT(rx)], utmp);
                storeHalfWord(pM, utmp, pM->reg.r[XLAT(ry)], &accError);
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    UPDATE_PC_NEXT32(pM);
                }
            }else{
                utmp = pM->reg.r[XLAT(rx)] + (funct5<<1);
                if(DEBUG) EXI_LOG_PRINTF("sh %s, 0x%x(%s) (=0x%x)\n", regStr[XLAT(ry)], funct5<<1, regStr[XLAT(rx)], utmp);
                storeHalfWord(pM, utmp, pM->reg.r[XLAT(ry)], &accError);
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    UPDATE_PC_NEXT16(pM);
                }
            }
            break;
        case MIPS16E_OP_SWSP:
            if( WITH_PREXIX(inst32) ){
                utmp = pM->reg.r[29] + SIGN_EXT16(imm16);
                if(DEBUG) EXI_LOG_PRINTF("sw %s, 0x%x(sp) (=0x%x)\n", regStr[XLAT(rx)], imm16, utmp);
                storeWord(pM, utmp, pM->reg.r[XLAT(rx)], &accError);
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    UPDATE_PC_NEXT32(pM);
                }
            }else{
                utmp = pM->reg.r[29] + (ZERO_EXT8(imm8)<<2);
                if(DEBUG) EXI_LOG_PRINTF("sw %s, 0x%x(sp) (=0x%x)\n", regStr[XLAT(rx)], (ZERO_EXT8(imm8)<<2), utmp);
                storeWord(pM, utmp, pM->reg.r[XLAT(rx)], &accError);
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    UPDATE_PC_NEXT16(pM);
                }
            }
            break;
        case MIPS16E_OP_SW:
            if( WITH_PREXIX(inst32) ){
                utmp = pM->reg.r[XLAT(rx)] + SIGN_EXT16(imm16);
                if(DEBUG) EXI_LOG_PRINTF("sw %s, 0x%x(%s) (=0x%x)\n", regStr[XLAT(ry)], imm16, regStr[XLAT(rx)], utmp);
                storeWord(pM, utmp, pM->reg.r[XLAT(ry)], &accError);
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    UPDATE_PC_NEXT32(pM);
                }
            }else{
                utmp = pM->reg.r[XLAT(rx)] + (funct5<<2);
                if(DEBUG) EXI_LOG_PRINTF("sw %s, 0x%x(%s) (=0x%x)\n", regStr[XLAT(ry)], funct5<<2, regStr[XLAT(rx)], utmp);
                storeWord(pM, utmp, pM->reg.r[XLAT(ry)], &accError);
                if( accError ){
                    prepareException(pM, GET_EXCEPT_CODE(accError), utmp);
                }else{
                    UPDATE_PC_NEXT16(pM);
                }
            }
            break;
        case MIPS16E_OP_RRR:
            switch(inst32 & 3){
                case 1:
                    if(DEBUG) EXI_LOG_PRINTF("addu %s, %s, %s\n", regStr[XLAT(rz)], regStr[XLAT(rx)], regStr[XLAT(ry)]);
                    pM->reg.r[XLAT(rz)] = (pM->reg.r[XLAT(rx)] + pM->reg.r[XLAT(ry)]);
                    UPDATE_PC_NEXT16(pM);
                    break;
                case 3:
                    if(DEBUG) EXI_LOG_PRINTF("subu %s, %s, %s\n", regStr[XLAT(rz)], regStr[XLAT(rx)], regStr[XLAT(ry)]);
                    pM->reg.r[XLAT(rz)] = (pM->reg.r[XLAT(rx)] - pM->reg.r[XLAT(ry)]);
                    UPDATE_PC_NEXT16(pM);
                    break;
                default:
                    UNKNOWN_INSTRUCTION16("unhandled MIPS16 op (RRR)");
                    break;
            }
            break;
        case MIPS16E_OP_RR:
            switch(funct5){
                case MIPS16E_RRFUNCT_JR:
                    switch( ry ){
                        case 2:
                            jumpaddr = pM->reg.r[XLAT(rx)];
                            pM->reg.r[31] = pM->reg.pc + 4;
                            UPDATE_PC_NEXT16_WITH_DELAYED_IMM(pM, jumpaddr);
                            if(DEBUG) EXI_LOG_PRINTF("jalr %s(=0x%x)\n", regStr[XLAT(rx)], jumpaddr);
                            break;
                        case 6:
                            jumpaddr = pM->reg.r[XLAT(rx)];
                            pM->reg.r[31] = pM->reg.pc + 2;
                            UPDATE_PC_IMM(pM, jumpaddr);
                            if(DEBUG) EXI_LOG_PRINTF("jalrc %s(=0x%x)\n", regStr[XLAT(rx)], jumpaddr);
                            break;
                        case 4:
                            UPDATE_PC_IMM(pM, pM->reg.r[XLAT(rx)]);
                            if(DEBUG) EXI_LOG_PRINTF("jrc %s(=0x%x)\n", regStr[XLAT(rx)], pM->reg.r[XLAT(rx)]);
                            break;
                        case 5:
                            UPDATE_PC_IMM(pM, pM->reg.r[31]);
                            if(DEBUG) EXI_LOG_PRINTF("jrc ra(=0x%x)\n", pM->reg.r[31]);
                            break;
                        case 0:
                            UPDATE_PC_NEXT16_WITH_DELAYED_IMM(pM, pM->reg.r[XLAT(rx)]);
                            if(DEBUG) EXI_LOG_PRINTF("jr %s(=0x%x)\n", regStr[XLAT(rx)], pM->reg.r[XLAT(rx)]);
                            break;
                        case 1:
                            UPDATE_PC_NEXT16_WITH_DELAYED_IMM(pM, pM->reg.r[31]);
                            if(DEBUG) EXI_LOG_PRINTF("jr ra(=0x%x)\n", pM->reg.r[31]);
                            break;
                        default:
                            UNKNOWN_INSTRUCTION16("unhandled MIPS16 op (JR)");
                            break;
                    }
                    break;
                case MIPS16E_RRFUNCT_SDBBP:
                    /* Software Debug Breakpoint */
                    UNKNOWN_INSTRUCTION16("unhandled MIPS16 op (SDBBP)");
                case MIPS16E_RRFUNCT_SLT:
                    if(DEBUG) EXI_LOG_PRINTF("slt %s, %s\n", regStr[XLAT(rx)], regStr[XLAT(ry)]);
                    pM->reg.r[24] = (((intmx)pM->reg.r[XLAT(rx)]) < ((intmx)pM->reg.r[XLAT(ry)])) ? 1 : 0;
                    UPDATE_PC_NEXT16(pM);
                    break;
                case MIPS16E_RRFUNCT_SLTU:
                    if(DEBUG) EXI_LOG_PRINTF("sltu %s, %s\n", regStr[XLAT(rx)], regStr[XLAT(ry)]);
                    pM->reg.r[24] = (pM->reg.r[XLAT(rx)] < pM->reg.r[XLAT(ry)]) ? 1 : 0;
                    UPDATE_PC_NEXT16(pM);
                    break;
                case MIPS16E_RRFUNCT_SLLV:
                    if(DEBUG) EXI_LOG_PRINTF("sllv %s, %s\n", regStr[XLAT(ry)], regStr[XLAT(rx)]);
                    pM->reg.r[XLAT(ry)] = (pM->reg.r[XLAT(ry)] << ((pM->reg.r[XLAT(rx)]) & 0x1f));
                    UPDATE_PC_NEXT16(pM);
                    break;
                case MIPS16E_RRFUNCT_BREAK:
                    UNKNOWN_INSTRUCTION16("unhandled MIPS16 op (BREAK)");
                case MIPS16E_RRFUNCT_SRLV:
                    if(DEBUG) EXI_LOG_PRINTF("srlv %s, %s\n", regStr[XLAT(ry)], regStr[XLAT(rx)]);
                    pM->reg.r[XLAT(ry)] = (((uint64_t)((uintmx)pM->reg.r[XLAT(ry)])) >> ((pM->reg.r[XLAT(rx)]) & 0x1f));
                    UPDATE_PC_NEXT16(pM);
                    break;
                case MIPS16E_RRFUNCT_SRAV:
                    if(DEBUG) EXI_LOG_PRINTF("srav %s, %s\n", regStr[XLAT(ry)], regStr[XLAT(rx)]);
                    pM->reg.r[XLAT(ry)] = (((int64_t)((intmx)pM->reg.r[XLAT(ry)])) >> ((pM->reg.r[XLAT(rx)]) & 0x1f));
                    UPDATE_PC_NEXT16(pM);
                    break;
                case MIPS16E_RRFUNCT_CMP:
                    if(DEBUG) EXI_LOG_PRINTF("cmp %s, %s\n", regStr[XLAT(rx)], regStr[XLAT(ry)]);
                    pM->reg.r[24] = (pM->reg.r[XLAT(rx)] ^ pM->reg.r[XLAT(ry)]);
                    UPDATE_PC_NEXT16(pM);
                    break;
                case MIPS16E_RRFUNCT_NEG:
                    if(DEBUG) EXI_LOG_PRINTF("neg %s, %s\n", regStr[XLAT(rx)], regStr[XLAT(ry)]);
                    pM->reg.r[XLAT(rx)] = 0 - pM->reg.r[XLAT(ry)];
                    UPDATE_PC_NEXT16(pM);
                    break;
                case MIPS16E_RRFUNCT_AND:
                    if(DEBUG) EXI_LOG_PRINTF("and %s, %s\n", regStr[XLAT(rx)], regStr[XLAT(ry)]);
                    pM->reg.r[XLAT(rx)] = (pM->reg.r[XLAT(rx)] & pM->reg.r[XLAT(ry)]);
                    UPDATE_PC_NEXT16(pM);
                    break;
                case MIPS16E_RRFUNCT_OR:
                    if(DEBUG) EXI_LOG_PRINTF("or %s, %s\n", regStr[XLAT(rx)], regStr[XLAT(ry)]);
                    pM->reg.r[XLAT(rx)] = (pM->reg.r[XLAT(rx)] | pM->reg.r[XLAT(ry)]);
                    UPDATE_PC_NEXT16(pM);
                    break;
                case MIPS16E_RRFUNCT_XOR:
                    if(DEBUG) EXI_LOG_PRINTF("xor %s, %s\n", regStr[XLAT(rx)], regStr[XLAT(ry)]);
                    pM->reg.r[XLAT(rx)] = (pM->reg.r[XLAT(rx)] ^ pM->reg.r[XLAT(ry)]);
                    UPDATE_PC_NEXT16(pM);
                    break;
                case MIPS16E_RRFUNCT_NOT:
                    if(DEBUG) EXI_LOG_PRINTF("not %s, %s\n", regStr[XLAT(rx)], regStr[XLAT(ry)]);
                    pM->reg.r[XLAT(rx)] = ~pM->reg.r[XLAT(ry)];
                    UPDATE_PC_NEXT16(pM);
                    break;
                case MIPS16E_RRFUNCT_MFHI:
                    if(DEBUG) EXI_LOG_PRINTF("mfhi %s\n", regStr[XLAT(rx)]);
                    pM->reg.r[XLAT(rx)] = pM->reg.hi;
                    UPDATE_PC_NEXT16(pM);
                    break;
                case MIPS16E_RRFUNCT_CNVT:
                    switch(ry){
                        case 0:
                            if(DEBUG) EXI_LOG_PRINTF("zeb %s\n", regStr[XLAT(rx)]);
                            pM->reg.r[XLAT(rx)] = (uintmx)((uint8_t)pM->reg.r[XLAT(rx)]);
                            UPDATE_PC_NEXT16(pM);
                            break;
                        case 1:
                            if(DEBUG) EXI_LOG_PRINTF("zeh %s\n", regStr[XLAT(rx)]);
                            pM->reg.r[XLAT(rx)] = (uintmx)((uint16_t)pM->reg.r[XLAT(rx)]);
                            UPDATE_PC_NEXT16(pM);
                            break;
                        case 4:
                            if(DEBUG) EXI_LOG_PRINTF("seb %s\n", regStr[XLAT(rx)]);
                            pM->reg.r[XLAT(rx)] = (intmx)((int8_t)pM->reg.r[XLAT(rx)]);
                            UPDATE_PC_NEXT16(pM);
                            break;
                        case 5:
                            if(DEBUG) EXI_LOG_PRINTF("seh %s\n", regStr[XLAT(rx)]);
                            pM->reg.r[XLAT(rx)] = (intmx)((int16_t)pM->reg.r[XLAT(rx)]);
                            UPDATE_PC_NEXT16(pM);
                            break;
                        default:
                            UNKNOWN_INSTRUCTION16("unhandled MIPS16 op (CNVT)");
                            break;
                    }
                    break;
                case MIPS16E_RRFUNCT_MFLO:
                    if(DEBUG) EXI_LOG_PRINTF("mflo %s\n", regStr[XLAT(rx)]);
                    pM->reg.r[XLAT(rx)] = pM->reg.lo;
                    UPDATE_PC_NEXT16(pM);
                    break;
                case MIPS16E_RRFUNCT_MULT:
                    { // to define a variable
                        if(DEBUG) EXI_LOG_PRINTF("mult %s, %s\n", regStr[XLAT(rx)], regStr[XLAT(ry)]);
                        int64_t mul_tmp = ((int64_t)((intmx)(pM->reg.r[XLAT(rx)]))) * ((int64_t)((intmx)(pM->reg.r[XLAT(ry)])));
                        pM->reg.hi = (uintmx)((mul_tmp>>16)>>16);
                        pM->reg.lo = (uintmx)mul_tmp;
                        UPDATE_PC_NEXT16(pM);
                    }
                    break;
                case MIPS16E_RRFUNCT_MULTU:
                    { // to define a variable
                        if(DEBUG) EXI_LOG_PRINTF("multu %s, %s\n", regStr[XLAT(rx)], regStr[XLAT(ry)]);
                        uint64_t mul_tmp = ((uint64_t)((uintmx)(pM->reg.r[XLAT(rx)]))) * ((uint64_t)((uintmx)(pM->reg.r[XLAT(ry)])));
                        pM->reg.hi = (uintmx)((mul_tmp>>16)>>16);
                        pM->reg.lo = (uintmx)mul_tmp;
                        UPDATE_PC_NEXT16(pM);
                    }
                    break;
                case MIPS16E_RRFUNCT_DIV:
                    if(DEBUG) EXI_LOG_PRINTF("div %s, %s\n", regStr[XLAT(rx)], regStr[XLAT(ry)]);
                    if( pM->reg.r[XLAT(ry)] == 0 ){
                        // zero division
                        pM->reg.lo = 0; // q
                        pM->reg.hi = 0; // r
                    }else{
                        pM->reg.lo = ((((intmx)(pM->reg.r[XLAT(rx)])) / ((intmx)(pM->reg.r[XLAT(ry)])))); // q
                        pM->reg.hi = ((((intmx)(pM->reg.r[XLAT(rx)])) % ((intmx)(pM->reg.r[XLAT(ry)])))); // r
                    }
                    UPDATE_PC_NEXT16(pM);
                    break;
                case MIPS16E_RRFUNCT_DIVU:
                    if(DEBUG) EXI_LOG_PRINTF("divu %s, %s\n", regStr[XLAT(rx)], regStr[XLAT(ry)]);
                    if( pM->reg.r[XLAT(ry)] == 0 ){
                        // zero division
                        pM->reg.lo = 0; // q
                        pM->reg.hi = 0; // r
                    }else{
                        pM->reg.lo = ((uintmx)(pM->reg.r[XLAT(rx)])) / ((uintmx)(pM->reg.r[XLAT(ry)])); // q
                        pM->reg.hi = ((uintmx)(pM->reg.r[XLAT(rx)])) % ((uintmx)(pM->reg.r[XLAT(ry)])); // r
                    }
                    UPDATE_PC_NEXT16(pM);
                    break;
                default:
                    UNKNOWN_INSTRUCTION16("unhandled MIPS16 op (RR)");
                    break;
            }
            break;
        case MIPS16E_OP_EXTEND:
            /* not processed here */
            UNKNOWN_INSTRUCTION16("unhandled MIPS16 op (EXTEND)");
        default:
            UNKNOWN_INSTRUCTION16("unhandled MIPS16 op");
    }

execMIPS16_exit:

    return MIPS16E_EXEC_OK;
}

