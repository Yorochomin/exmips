#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

#include "mips.h"
#include "misc.h"
#include "logfile.h"
#include "terminal.h"
#include "mem.h"
#include "ExInst_common.h"
#include "cp0.h"
#include "env_unix.h"
#include "mainloop.h"

#if USE_RUNTIME_COMPILATION
#include "rc.h"
#endif

#ifndef USE_DEBUG
int DEBUG = 0;
#endif

#if USE_SPI_EMULATION 
#include "dev_SPI.h"
#endif
#if USE_GMAC_EMULATION 
#include "dev_gmac.h"
#endif


int main(int argc, char *argv[]){
    struct stMachineState ms;

    memset(&ms, 0, sizeof(ms));

    ms.emu.breakPoint    = EMU_MEM1_ADDR + EMU_MEM1_SIZE;
    ms.emu.breakMask     = ~0;
    ms.emu.runAfterBreak = 10;
    ms.emu.breakCounter  = 0;
    ms.emu.bWriteBackFlashData = 0;

    //-----------------------------------------------------
    // Reset the CPU register and set pointers for CPU registers
    //-----------------------------------------------------
    ms.reg.pc    = EXCEPT_VECT_RESET;

    memset(ms.reg.r, 0, sizeof(ms.reg.r));

    // Initializing register values of CoProcessor 0
    C0_VAL(&ms, C0_STATUS)  = C0_STATUS_INIT_VAL;
    C0_VAL(&ms, C0_CONFIG)  = C0_CONFIG_INIT_VAL;
    C0_VAL(&ms, C0_CONFIG1) = C0_CONFIG1_INIT_VAL;
    C0_VAL(&ms, C0_CONFIG2) = C0_CONFIG2_INIT_VAL;
    C0_VAL(&ms, C0_EBASE)   = C0_EBASE_INIT_VAL;
    C0_VAL(&ms, C0_PRID)    = C0_PRID_INIT_VAL;
    C0_VAL(&ms, C0_ENTRYHI) = C0_ENTRYHI_INIT_VAL;
    C0_VAL(&ms, C0_ENTRYLO0)= C0_ENTRYLO0_INIT_VAL;
    C0_VAL(&ms, C0_ENTRYLO1)= C0_ENTRYLO1_INIT_VAL;
    C0_VAL(&ms, C0_RANDOM)  = NUM_TLB_ENTRY - 1;
    C0_VAL(&ms, C0_INDEX)   = C0_INDEX_INIT_VAL;
    C0_VAL(&ms, C0_PAGEMASK)= C0_PAGEMASK_INIT_VAL;
    C0_VAL(&ms, C0_WIRED)   = C0_WIRED_INIT_VAL;

    ms.reg.c0_count_basetime    = getTimeInUSec(&ms);
    ms.reg.c0_count_currenttime = getTimeInUSec(&ms);
    storeCP0(&ms, C0_COUNT_REG  , C0_STATUS_SEL , 0); // reset the counter
    storeCP0(&ms, C0_COMPARE_REG, C0_COMPARE_SEL, 0); // reset the compare

    //-----------------------------------------------------
    // initializing the memory system and debugging feature
    //-----------------------------------------------------
    ms.mem.watchAddr = EMU_MEM1_ADDR + EMU_MEM1_SIZE; // watch is disabled

    ms.emu.log_enabled_cat = (LOGCAT_EMU | LOGCAT_CPU_EXE);
    ms.emu.log_level       = LOGLV_NOTICE;

#if USE_SPI_EMULATION
    ms.emu.spiflash_param = &spiflashparam_s25fl164k;
#endif

    initEmulator(&ms, argc, argv);

#if USE_RUNTIME_COMPILATION
    rc_init();
#endif

#if USE_SPI_EMULATION
    if( initSPI(&ms, &ms.mem.ioSPI) != SPI_WORKER_RESULT_OK ){
        exit(1);
    }
#endif
#if USE_GMAC_EMULATION
    initGMACX(&ms, &ms.mem.ioGMACX0);
#endif

    //-----------------------------------------------------
    // Reset the terminal color for serial console programs such as monitors
    //-----------------------------------------------------
    termResetColor();
    termResetBlink();
    //-----------------------------------------------------


    logfile_init( ms.emu.log_enabled_cat, ms.emu.log_level);
    logfile_printf(LOGCAT_EMU | LOGLV_NOTICE, "------------------------\n");
    logfile_printf(LOGCAT_EMU | LOGLV_NOTICE, "Starting the emulator...\n");

    if(DEBUG){
        logfile_printf(LOGCAT_EMU | LOGLV_NOTICE, "Debug mode\n");
    }

    if( (ms.emu.breakPoint >= EMU_MEM0_ADDR && ms.emu.breakPoint < EMU_MEM0_ADDR + EMU_MEM0_SIZE) ||
        (ms.emu.breakPoint >= EMU_MEM1_ADDR && ms.emu.breakPoint < EMU_MEM1_ADDR + EMU_MEM1_SIZE) ){
        logfile_printf(LOGCAT_EMU | LOGLV_NOTICE, "Breakpoint        : 0x%x\n", ms.emu.breakPoint);
        logfile_printf(LOGCAT_EMU | LOGLV_NOTICE, "#insts after break: %d\n",   ms.emu.runAfterBreak);
    }

    saveTerminalSetting(&ms);
    initTerminalSetting(&ms);

    /* Host Timer */
    setTimerInUSec(&ms, SYSTEM_TIMER_INTERVAL_IN_USEC);

    /* Main routine */
    mainloop32(&ms);

    /* remove devices */
#if USE_SPI_EMULATION
    removeSPI(&ms, &ms.mem.ioSPI);
#endif
#if USE_GMAC_EMULATION
    removeGMACX(&ms, &ms.mem.ioGMACX0);
#endif

    restoreTerminalSetting(&ms);
    termResetSettingForExit();

    logfile_close();

    return 0;
}

