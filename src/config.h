#ifndef __CONFIG_H__
#define __CONFIG_H__

#define FREQ_CPU (400*1000*1000)

/*
 * Optional feature
 * 1: enables the option
 * 0: disables the option
 */
#define USE_GMAC_EMULATION      1
#define USE_SPI_EMULATION       1
#define USE_RUNTIME_COMPILATION 1
#define USE_DEBUG               0

#if USE_RUNTIME_COMPILATION
    #if USE_DEBUG
        #error "DEBUG should be disabled when runtime compilation is enabled."
    #endif
    #define USE_DEBUG 0
#else
    #if !defined(USE_DEBUG)
        #define USE_DEBUG 1
    #endif
#endif

// DEBUG variable is defined in mainloop32.c if USE_DEBUG != 0
#if defined(USE_DEBUG) && USE_DEBUG == 0
    #define DEBUG 0
#endif

#define CPU_FREQ_COUNT_RESOLUTION 2  /* CCRes value of RDHWR 3 */

/* main memory size (2^26 = 64MB) */
#define EMU_MEM0_AWIDTH   27
/* flash memory size (2^23 = 8MB) */
#define EMU_MEM1_AWIDTH   23

#define NUM_TLB_ENTRY 32
//#define NUM_TLB_WIRED 8

#define SYSTEM_TIMER_INTERVAL_IN_USEC 1000

#endif
