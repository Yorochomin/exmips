#include <stdio.h>
#include <stdint.h>
#include <signal.h>


#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>


#include "mips.h"
//#include "instmap32.h"
#include "ExInst_common.h"

#include "misc.h"
#include "dev_UART.h"
#include "logfile.h"
#include "mem.h"
#include "cp0.h"

#define STORE_MASKED_VAL(val,regStr)  (pM->reg.cp0[ C0_REG_IDX2(regStr##_REG, regStr##_SEL) ] = (((val) & regStr##_MASK_W) | regStr##_CONST))
#define LOAD_MASKED_VAL(regStr)      ((pM->reg.cp0[ C0_REG_IDX2(regStr##_REG, regStr##_SEL) ] & regStr##_MASK_R) | regStr##_CONST)

#define REGS_SEL2NUM(reg,sel)        ((reg) << CP_SEL_BITS) + ((sel)&((1<<CP_SEL_BITS)-1))
#define CP0NUM(regStr)               REGS_SEL2NUM(regStr##_REG, regStr##_SEL)


void storeC0Counter(struct stMachineState *pM, uintmx val){
    uint64_t diff = ((uint64_t)val) / ((uint64_t) (FREQ_CPU/(CPU_FREQ_COUNT_RESOLUTION*1000*1000)));

    pM->reg.c0_count_basetime = pM->reg.c0_count_currenttime - diff;
}

uintmx loadC0CounterPrecise(struct stMachineState *pM){
    uint64_t counter;

    counter  = pM->reg.c0_count_currenttime - pM->reg.c0_count_basetime;
    counter *= ((uint64_t) (FREQ_CPU/(CPU_FREQ_COUNT_RESOLUTION*1000*1000)));
    counter += (pM->emu.nExecInsts - pM->reg.c0_count_ninst_in_ctime) / CPU_FREQ_COUNT_RESOLUTION;
    return counter;
}

uintmx loadC0Counter(struct stMachineState *pM){
    uint64_t counter;

    counter  = pM->reg.c0_count_currenttime - pM->reg.c0_count_basetime;
    counter *= ((uint64_t) (FREQ_CPU/(CPU_FREQ_COUNT_RESOLUTION*1000*1000)));
    counter += (pM->emu.nExecInsts - pM->reg.c0_count_ninst_in_ctime) / CPU_FREQ_COUNT_RESOLUTION;
    return counter;
}

uint64_t loadC0CounterLong(struct stMachineState *pM){
    uint64_t counter;

    counter  = pM->reg.c0_count_currenttime - pM->reg.c0_count_basetime;
    counter *= ((uint64_t) (FREQ_CPU/(CPU_FREQ_COUNT_RESOLUTION*1000*1000)));
    counter += (pM->emu.nExecInsts - pM->reg.c0_count_ninst_in_ctime) * 2;
    return counter;
}

void  storeCP0(struct stMachineState *pM, unsigned int reg, unsigned sel, uintmx val){

    int num = REGS_SEL2NUM(reg,sel);
    uint64_t long_count;

    switch(num){
        case CP0NUM(C0_STATUS):   STORE_MASKED_VAL(val, C0_STATUS  ); break;
        case CP0NUM(C0_CAUSE):    STORE_MASKED_VAL(val, C0_CAUSE   ); break;

        case CP0NUM(C0_ENTRYHI):  STORE_MASKED_VAL(val, C0_ENTRYHI ); break;
        case CP0NUM(C0_ENTRYLO0): STORE_MASKED_VAL(val, C0_ENTRYLO0); break;
        case CP0NUM(C0_ENTRYLO1): STORE_MASKED_VAL(val, C0_ENTRYLO1); break;
        case CP0NUM(C0_RANDOM):   /* Do nothing because read-only */  break;
        case CP0NUM(C0_INDEX):    STORE_MASKED_VAL(val, C0_INDEX   ); break;
        case CP0NUM(C0_PAGEMASK): STORE_MASKED_VAL(val, C0_PAGEMASK); break;
        case CP0NUM(C0_WIRED):    STORE_MASKED_VAL(val, C0_WIRED   ); break;
        case CP0NUM(C0_BADVADDR): /* Do nothing because read-only */  break;

        case CP0NUM(C0_HWRENA):   STORE_MASKED_VAL(val, C0_HWRENA  ); break;
        case CP0NUM(C0_EBASE):    STORE_MASKED_VAL(val, C0_EBASE   ); break;
        case CP0NUM(C0_CONFIG):   STORE_MASKED_VAL(val, C0_CONFIG  ); break;
        case CP0NUM(C0_CONFIG2):  STORE_MASKED_VAL(val, C0_CONFIG2 ); break;

        case CP0NUM(C0_INTCTL):   STORE_MASKED_VAL(val, C0_INTCTL  ); break;

        case CP0NUM(C0_COUNT):    storeC0Counter(pM, val);            break;
        case CP0NUM(C0_COMPARE):
            long_count = loadC0CounterLong(pM);
            if(val <= ((uintmx)long_count) ){
                pM->reg.c0_compare_long = ((long_count + 0x100000000ULL) & 0xffffffff00000000ULL);
                if( pM->reg.c0_compare_long < long_count ){
                    printf("Wrap around occurs ---------------------------------\n");
                }
            }else{
                pM->reg.c0_compare_long = (long_count & 0xffffffff00000000ULL);
            }
            pM->reg.c0_compare_long += val;
            C0_VAL(pM, C0_COMPARE) = val;
            C0_VAL(pM,C0_CAUSE) &=~((1<<C0_INTCTL_TIMER_INT_IPNUM)<<C0_CAUSE_BIT_IP);
            C0_VAL(pM,C0_CAUSE) &=~ (1<<C0_CAUSE_BIT_TI);
            break;

        default: {
//            printf("Write CP0(pc: 0x%x, reg: %d, sel: %d, val: 0x%x)\n", REG_PC(pM), reg, sel, val);
        }
    }

    pM->reg.cp0[ C0_REG_IDX2(reg, sel) ] = val;
}

uintmx loadCP0(struct stMachineState *pM, unsigned int reg, unsigned sel){

    int num = REGS_SEL2NUM(reg,sel);

    switch(num){
        case CP0NUM(C0_STATUS):    return LOAD_MASKED_VAL(C0_STATUS);
        case CP0NUM(C0_CAUSE):     return LOAD_MASKED_VAL(C0_CAUSE);

        case CP0NUM(C0_ENTRYHI):   return LOAD_MASKED_VAL(C0_ENTRYHI);
        case CP0NUM(C0_ENTRYLO0):  return LOAD_MASKED_VAL(C0_ENTRYLO0);
        case CP0NUM(C0_ENTRYLO1):  return LOAD_MASKED_VAL(C0_ENTRYLO1);
        case CP0NUM(C0_INDEX):     return LOAD_MASKED_VAL(C0_INDEX);
        case CP0NUM(C0_PAGEMASK):  return LOAD_MASKED_VAL(C0_PAGEMASK);
        case CP0NUM(C0_WIRED):     return LOAD_MASKED_VAL(C0_WIRED);
        case CP0NUM(C0_RANDOM):    return LOAD_MASKED_VAL(C0_RANDOM);

        case CP0NUM(C0_HWRENA):    return LOAD_MASKED_VAL(C0_HWRENA);
        case CP0NUM(C0_EBASE):     return LOAD_MASKED_VAL(C0_EBASE);
        case CP0NUM(C0_CONFIG):    return LOAD_MASKED_VAL(C0_CONFIG);
        case CP0NUM(C0_CONFIG1):   return C0_CONFIG1_INIT_VAL;
        case CP0NUM(C0_CONFIG2):   return LOAD_MASKED_VAL(C0_CONFIG2);
        case CP0NUM(C0_CONFIG3):   return C0_CONFIG3_INIT_VAL;
        case CP0NUM(C0_PRID):      return C0_PRID_INIT_VAL;

        case CP0NUM(C0_INTCTL):    return LOAD_MASKED_VAL(C0_INTCTL);

        case CP0NUM(C0_COUNT):     return loadC0Counter(pM);
        case CP0NUM(C0_COMPARE):   return C0_VAL(pM, C0_COMPARE);

        default: {
//            printf("Read CP0(pc 0x%x, reg: %d, sel: %d, val: 0x%x)\n", REG_PC(pM), reg, sel, pM->reg.cp0[ C0_REG_IDX2(reg, sel) ]);
        }
    }

    return pM->reg.cp0[ C0_REG_IDX2(reg, sel) ];
}
