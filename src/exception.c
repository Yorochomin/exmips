#include <stdio.h>
#include <stdint.h>
#include <signal.h>

#include <stdlib.h>
/*
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
*/
#include <sys/time.h>
#include <sys/stat.h>


#include "mips.h"
#include "ExInst_common.h"

#include "misc.h"
#include "dev_UART.h"
#include "logfile.h"
#include "mem.h"
#include "cp0.h"

/*
Preparation for entering exception excepting software and hardware interrupts and syscalls.

This function updates PC, C0_EPC, C0_STATUS, and C0_CAUSE.
It also updates related coprocessor registers.

* Arguments
exceptCode:
 exception code for C0_CAUSE_EXC such as EXCEPT_CODE_TLB_LOAD.

option:
 coprocessor number when exceptCode is EXCEPT_CODE_COPROCESSOR_UNAVAIL
 BADVADDR           when exceptCode is EXCEPT_CODE_TLB_LOAD
 BADVADDR           when exceptCode is EXCEPT_CODE_TLB_STORE
 BADVADDR           when exceptCode is EXCEPT_CODE_ADDR_ERR_LOAD
 BADVADDR           when exceptCode is EXCEPT_CODE_ADDR_ERR_STORE
*/
void prepareException(struct stMachineState *pM, int exceptCode, uintmx option){
    uint32_t prevModeIsEXL;

    prevModeIsEXL = MODE_IS_IN_EXCEPTION(C0_VAL(pM,C0_STATUS));

    C0_VAL(pM,C0_CAUSE) &= ~(C0_CAUSE_EXCCODE_MASK);
    C0_VAL(pM,C0_CAUSE) |= ((exceptCode << C0_CAUSE_BIT_EXCCODE) & C0_CAUSE_EXCCODE_MASK);
    C0_VAL(pM,C0_CAUSE) &= ~(C0_CAUSE_CE_MASK);

    if( exceptCode == EXCEPT_CODE_COPROCESSOR_UNAVAIL ){
        C0_VAL(pM,C0_CAUSE) |= ((option << C0_CAUSE_BIT_CE) & C0_CAUSE_CE_MASK);
    }

    if( prevModeIsEXL ){
        if( pM->reg.delay_en ){
            C0_VAL(pM, C0_ERROREPC)= pM->reg.pc_prev_jump;
        }else{
            C0_VAL(pM, C0_ERROREPC)= REG_PC(pM);
        }
    }else{
        if( pM->reg.delay_en ){
            C0_VAL(pM, C0_EPC)     = pM->reg.pc_prev_jump;
            C0_VAL(pM, C0_CAUSE)  |=  (1<<C0_CAUSE_BIT_BD);
        }else{
            C0_VAL(pM, C0_EPC)     =  REG_PC(pM);
            C0_VAL(pM, C0_CAUSE)  &= ~(1<<C0_CAUSE_BIT_BD);
        }
    }

    pM->reg.delay_en = 0;

    switch( exceptCode ){
        case EXCEPT_CODE_CACHE_ERROR:
            C0_VAL(pM,C0_STATUS) |= (1<<C0_STATUS_BIT_ERL); // error level
            REG_PC(pM) = EXCEPT_VECT_CacheErr( C0_VAL(pM,C0_EBASE), C0_VAL(pM,C0_STATUS) & (1<<C0_STATUS_BIT_BEV) );
            break;
        case EXCEPT_CODE_TLB_REFILL_LOAD:
        case EXCEPT_CODE_TLB_REFILL_STORE:
            /*
            See:
            Figure 6.6 in page 141 of
            "MIPS32 74K Processor Core Family Software User’s Manual," Revision 01.05, March 30, 2011.
            */
            C0_VAL(pM,C0_CAUSE) &= ~(C0_CAUSE_EXCCODE_MASK);
            C0_VAL(pM,C0_CAUSE) |= (((exceptCode == EXCEPT_CODE_TLB_REFILL_LOAD) ? EXCEPT_CODE_TLB_LOAD:EXCEPT_CODE_TLB_STORE) << C0_CAUSE_BIT_EXCCODE);

            C0_VAL(pM,C0_STATUS)  |= (1<<C0_STATUS_BIT_EXL); // exception level
            C0_VAL(pM,C0_ENTRYHI) &= (~C0_ENTRYHI_VPN2_MASK);
            C0_VAL(pM,C0_ENTRYHI) |= (option & C0_ENTRYHI_VPN2_MASK);
            C0_VAL(pM,C0_CONTEXT) &= 0xff80000f; /* TODO: position is variable. check is necessary */
            C0_VAL(pM,C0_CONTEXT) |= ((option & C0_ENTRYHI_VPN2_MASK) >> 9); /* TODO: position is variable. check is necessary */
            C0_VAL(pM,C0_BADVADDR) = option;
            REG_PC(pM) = EXCEPT_VECT_TLBRefill( C0_VAL(pM,C0_EBASE), C0_VAL(pM,C0_STATUS) & (1<<C0_STATUS_BIT_BEV), prevModeIsEXL );
            break;
        case EXCEPT_CODE_TLB_LOAD:
        case EXCEPT_CODE_TLB_STORE:
        case EXCEPT_CODE_MOD:
            /*
            See:
            Figure 6.6 in page 141 of
            "MIPS32 74K Processor Core Family Software User’s Manual," Revision 01.05, March 30, 2011, page 175-176.
            */
            C0_VAL(pM,C0_STATUS)  |= (1<<C0_STATUS_BIT_EXL); // exception level
            C0_VAL(pM,C0_ENTRYHI) &= (~C0_ENTRYHI_VPN2_MASK);
            C0_VAL(pM,C0_ENTRYHI) |= (option & C0_ENTRYHI_VPN2_MASK);
            C0_VAL(pM,C0_CONTEXT) &= 0xff80000f; /* TODO: position is variable. check is necessary */
            C0_VAL(pM,C0_CONTEXT) |= ((option & C0_ENTRYHI_VPN2_MASK) >> 9); /* TODO: position is variable. check is necessary */
            C0_VAL(pM,C0_BADVADDR) = option;
            REG_PC(pM) = EXCEPT_VECT_AllOther( C0_VAL(pM,C0_EBASE), C0_VAL(pM,C0_STATUS) & (1<<C0_STATUS_BIT_BEV) );
            break;
        case EXCEPT_CODE_ADDR_ERR_LOAD:
        case EXCEPT_CODE_ADDR_ERR_STORE:
            C0_VAL(pM,C0_BADVADDR) = option;
            // go through
        default:
            C0_VAL(pM,C0_STATUS) |= (1<<C0_STATUS_BIT_EXL); // exception level
            REG_PC(pM) = EXCEPT_VECT_AllOther( C0_VAL(pM,C0_EBASE), C0_VAL(pM,C0_STATUS) & (1<<C0_STATUS_BIT_BEV) );
    }
}

void prepareInterrupt(struct stMachineState *pM, int intCode){
    uint32_t prevModeIsEXL;

    prevModeIsEXL = MODE_IS_IN_EXCEPTION(C0_VAL(pM,C0_STATUS));

    C0_VAL(pM,C0_CAUSE) &= ~(C0_CAUSE_EXCCODE_MASK);
    C0_VAL(pM,C0_CAUSE) |= (EXCEPT_CODE_INTERRUPT << C0_CAUSE_BIT_EXCCODE);

    C0_VAL(pM,C0_CAUSE) &= ~(C0_CAUSE_IP_MASK);
    C0_VAL(pM,C0_CAUSE) |= ((intCode << C0_CAUSE_BIT_IP) & C0_CAUSE_IP_MASK);

    if( ! prevModeIsEXL ){
        if( pM->reg.delay_en ){
            C0_VAL(pM, C0_EPC)    =  pM->reg.pc_prev_jump;
            C0_VAL(pM, C0_CAUSE) |=  (1<<C0_CAUSE_BIT_BD);
        }else{
            C0_VAL(pM, C0_EPC)    =  REG_PC(pM);
            C0_VAL(pM, C0_CAUSE) &= ~(1<<C0_CAUSE_BIT_BD);
        }
    }

    pM->reg.delay_en = 0;

    C0_VAL(pM,C0_STATUS) |= (1<<C0_STATUS_BIT_EXL); // exception level
    REG_PC(pM) = EXCEPT_VECT_Int( C0_VAL(pM,C0_EBASE), C0_VAL(pM,C0_STATUS) & (1<<C0_STATUS_BIT_BEV), C0_VAL(pM,C0_CAUSE) & (1<<C0_CAUSE_BIT_IV) );
}