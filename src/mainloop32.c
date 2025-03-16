#include <stdio.h>
#include <stdint.h>
#include <signal.h>

#include "mips.h"
//#include "instmap32.h"
#include "ExInst_common.h"
#include "mips16e.h"
#include "mips32r2.h"

#include "misc.h"
#include "dev_UART.h"
#include "dev_ATH79.h"
#include "logfile.h"
#include "mem.h"
#include "cp0.h"
#include "tlb.h"
#include "exception.h"
#include "env_unix.h"
#include "monitor.h"

#if USE_GMAC_EMULATION
#include "dev_gmac.h"
#endif

#define LOGLEVEL_EMU_INFO   (LOGCAT_EMU | LOGLV_INFO)
#define LOGLEVEL_EMU_INFO3  (LOGCAT_EMU | LOGLV_INFO3)
#define LOGLEVEL_EMU_NOTICE (LOGCAT_EMU | LOGLV_NOTICE)
#define LOGLEVEL_EMU_ERROR  (LOGCAT_EMU | LOGLV_ERROR)

extern volatile sig_atomic_t eflag;
extern volatile sig_atomic_t tflag;

#if !USE_RUNTIME_COMPILATION
#define POINTER_LOG_SIZE_BIT 5
#define POINTER_LOG_SIZE (1<<POINTER_LOG_SIZE_BIT)

static uint32_t pointerLog[POINTER_LOG_SIZE];
static int pointerLog_start=0, pointerLog_end=0;
inline static void saveInstPointer(uint32_t pointer){
    pointerLog_end = (pointerLog_end+1) & (POINTER_LOG_SIZE-1);
    if( pointerLog_end == pointerLog_start ){
        pointerLog_start = (pointerLog_start+1) & (POINTER_LOG_SIZE-1);
    }
    pointerLog[pointerLog_end] = pointer;
}
#endif

#if USE_DEBUG && !defined(DEBUG)
// Define debug flag if the debug feature is enabled and "DEBUG" is not defined as a macro.
int DEBUG = 0;
#endif

void mainloop32(struct stMachineState *pM){
    uintmx pointer = REG_PC(pM);
    uint32_t inst = 0xffffffff;

    int      prev_eflag           = 0;
    uint64_t prevIntEventTrigTime = 0;

    uint64_t prevExecInsts = 0;
    pM->emu.nExecInsts = 0;
    pM->emu.stop       = 0;

    while(1){
        #if !USE_RUNTIME_COMPILATION
            if( pM->emu.stop > 0 && pM->emu.stop < pM->emu.nExecInsts ) break;
        #endif

        pM->reg.r[0] = 0;

        inst = fetchCodeWord(pM);
        pointer = REG_PC(pM);

#if !USE_RUNTIME_COMPILATION && USE_DEBUG
        saveInstPointer(pointer);

        if( ((pointer&pM->emu.breakMask) == (pM->emu.breakPoint&pM->emu.breakMask) && pM->emu.stop==0) || (pM->emu.breakCounter != 0 && pM->emu.breakCounter == pM->emu.nExecInsts) ){
            logfile_printf(LOGLEVEL_EMU_NOTICE, "Breakpoint\n");
            logfile_printf(LOGLEVEL_EMU_NOTICE, "================================== \n");
            logfile_printf(LOGLEVEL_EMU_NOTICE, "pointer: %05x  insts = %08x \n", pointer, inst);
            log_printReg32(LOGLEVEL_EMU_NOTICE, &(pM->reg));

            DEBUG=1;
            pM->emu.stop = pM->emu.nExecInsts + pM->emu.runAfterBreak;
        }
        if(DEBUG){
            logfile_printf(LOGLEVEL_EMU_ERROR, "cnt:%06ld PC:%x  ", pM->emu.nExecInsts, REG_PC(pM));
            logfile_printf(LOGLEVEL_EMU_NOTICE, "================================== \n");
            logfile_printf(LOGLEVEL_EMU_NOTICE, "pointer: %05x  insts = %08x \n", pointer, inst );
            log_printReg32(LOGLEVEL_EMU_NOTICE, &(pM->reg));
        }
#endif

        if( pointer & 1 ){
            // MIPS16e mode
            if( execMIPS16eInst(pM, inst) == MIPS16E_EXEC_FAIL ){
                goto mainloop32_exit;
            }
        }else{
            // MIPS32 mode
            if( execMIPS32eInst(pM, inst) == MIPS32R2_EXEC_FAIL ){
                goto mainloop32_exit;
            }
        }

        if( getTimerFlag(pM) ){
            resetTimerFlag(pM);
            uint64_t currentTime = getTimeInUSec(pM);

#if USE_GMAC_EMULATION
            // GMAC update process
            updateGMACX(pM, &(pM->mem.ioGMACX0));
#endif

            // Calculating the instruction execution rate
            pM->emu.execRate = (pM->emu.nExecInsts - prevExecInsts)*(1000*1000 / SYSTEM_TIMER_INTERVAL_IN_USEC);
            prevExecInsts = pM->emu.nExecInsts;

            // Updating the CoProcessor0 Counter
            pM->reg.c0_count_currenttime    = currentTime;
            pM->reg.c0_count_ninst_in_ctime = pM->emu.nExecInsts;
            readUARTReg(pM, &(pM->mem.ioUART0), IOADDR_UART0_BASE + UART_REG_LINESTAT); // to update internal state

            // Checking the SIGINT status, treatment of Ctrl+C.
            if( getIntFlag(pM) != prev_eflag ){
                // time between two Ctrl+C keyins is shorter than 1000ms, then enter the monitor
                if( currentTime - prevIntEventTrigTime < 1000*1000 ){
                    if( emuMonitor(pM) < 0 ){
                        // halt the system
                        pM->mem.ioMISC.reset_request = 1;
                    }
                    prevIntEventTrigTime = 0;
                }else{
                    prevIntEventTrigTime = currentTime;
                }
                prev_eflag = getIntFlag(pM);
            }
            if( prevIntEventTrigTime != 0 &&  currentTime - prevIntEventTrigTime >= 1000*1000 ){
                requestUARTSendBreak(pM, &(pM->mem.ioUART0));
                prevIntEventTrigTime = 0;
            }


            if( pM->mem.ioMISC.reset_request ){
                PRINTF("System reset ...\n");
                logfile_printf(LOGLEVEL_EMU_NOTICE, "System reset ...\n");
                goto mainloop32_exit;
            }
        }
        if( pM->reg.c0_compare_long <= loadC0CounterLong(pM) ){
            C0_VAL(pM,C0_CAUSE) |= ((1<<C0_INTCTL_TIMER_INT_IPNUM)<<C0_CAUSE_BIT_IP);
            C0_VAL(pM,C0_CAUSE) |= (1<<C0_CAUSE_BIT_TI);
        }

        if( pM->mem.ioUART0.intEnable && ((1<<3) & pM->mem.ioMISC.int_mask & readMiscIntStatusReg(pM)) ){
            C0_VAL(pM,C0_CAUSE) |=  ((1<<6)<<C0_CAUSE_BIT_IP);
        }else{
            C0_VAL(pM,C0_CAUSE) &= ~((1<<6)<<C0_CAUSE_BIT_IP);
        }

#if USE_GMAC_EMULATION
        // GMAC0 interrupt
        if( pM->mem.ioGMACX0.int_enable & pM->mem.ioGMACX0.int_status ){
            C0_VAL(pM,C0_CAUSE) |=  ((1<<4)<<C0_CAUSE_BIT_IP);
        }else{
            C0_VAL(pM,C0_CAUSE) &= ~((1<<4)<<C0_CAUSE_BIT_IP);
        }
#endif

        uint32_t utmp = C0_VAL(pM,C0_STATUS);
        if( (utmp & (1<<C0_STATUS_BIT_IE)) && (!MODE_IS_EXCEPTION(utmp)) ){
            if( (C0_VAL(pM,C0_CAUSE) & utmp & C0_CAUSE_IP_MASK) ){
                prepareInterrupt(pM, (C0_VAL(pM,C0_CAUSE) & C0_CAUSE_IP_MASK) >> C0_CAUSE_BIT_IP );
            }
        }

        pM->emu.nExecInsts++;
    }

mainloop32_exit:

    PRINTF("\n\n");
    PRINTF("pointer %x\n", pointer);

    if( getIntFlag(pM) ){
        logfile_printf(LOGLEVEL_EMU_NOTICE, "Interrupt...\n");
    }

#if !USE_RUNTIME_COMPILATION
    logfile_printf(LOGLEVEL_EMU_NOTICE, "================================== \n");
    logfile_printf(LOGLEVEL_EMU_NOTICE, "history:\n");
    for(int loop=pointerLog_start; loop != pointerLog_end; loop = (loop+1)%POINTER_LOG_SIZE){
        logfile_printf(LOGLEVEL_EMU_NOTICE, "pointer: 0x%08x\n", pointerLog[loop]);
    }
#endif
    logfile_printf(LOGLEVEL_EMU_NOTICE, "================================== \n");
    logfile_printf(LOGLEVEL_EMU_NOTICE, "pointer: 0x%08x, instruction = %08x \n", pointer, inst);

    log_printReg32(LOGLEVEL_EMU_NOTICE, &(pM->reg));

    logfile_printf(LOGLEVEL_EMU_NOTICE, "CP0:\n");
    logfile_printf(LOGLEVEL_EMU_NOTICE, " C0_STATUS : 0x%x\n", C0_VAL(pM, C0_STATUS));
    logfile_printf(LOGLEVEL_EMU_NOTICE, " C0_CAUSE  : 0x%x\n", C0_VAL(pM, C0_CAUSE));
    logfile_printf(LOGLEVEL_EMU_NOTICE, " C0_COMPARE: 0x%x\n", C0_VAL(pM, C0_COMPARE));
    logfile_printf(LOGLEVEL_EMU_NOTICE, " C0_COUNT  : 0x%x\n", loadC0Counter(pM));
}
