#ifndef __CP0DEF_H__
#define __CP0DEF_H__

#include "config.h"

// Definitions for C0_CONFIG
#define C0_CONFIG_MASK_R     ((1<<19 /*Write control*/) | (1<<18 /*Writable*/) | (3 /*Kseg0 coherency attribute*/)) /* 1 for variable bits */
#define C0_CONFIG_MASK_W     ((1<<19 /*Write control*/) | (1<<18 /*Writable*/) | (3 /*Kseg0 coherency attribute*/)) /* 1 for writable bits */
#define C0_CONFIG_INIT_VAL   ((1<<18 /*Writable*/) | (2<<0 /* Kseg0 is uncached */))
#define C0_CONFIG_CONST      ((1<<31 /*CONFIG1 is available*/) | (1<<10 /*MIPS32R2*/) | (1<<7 /*MMU-type:TLB*/))

// Definitions for C0_CONFIG1
#define C0_CONFIG1_INIT_VAL (\
    (1<<31 /*CONFIG2 is available*/) | \
    ((NUM_TLB_ENTRY-1)<<25) | \
    (2<<22 /*#sets per way*/) | (4<<19 /*line size*/) | (3<<16 /*#assoc*/) /*L1 I-cache*/ | \
    (2<<13 /*#sets per way*/) | (4<<10 /*line size*/) | (3<< 7 /*#assoc*/) /*L1 D-cache*/ | \
    (0<<6) /*existence of CP2*/ | \
    (0<<5) /*MDMX ASE is not implemented*/ | \
    (1<<4) /*#performance counter*/ | \
    (0<<3) /*#watchpoint registers*/ | \
    (1<<2) /*MIPS16e is available*/ | \
    (0<<1) /*EJTAG is not available*/ | \
    (0<<0) /*floating point unit is not available*/ | \
    0)

// Definitions for C0_CONFIG2
#define C0_CONFIG2_MASK_R    (1<<12 /*L2 bypass*/)  /* 1 for variable bits */
#define C0_CONFIG2_MASK_W    (1<<12 /*L2 bypass*/)  /* 1 for writable bits */
#define C0_CONFIG2_INIT_VAL  (1<<31 /*Config3 is available*/)
#define C0_CONFIG2_CONST     (1<<31 /*Config3 is available*/)

// Definitions for C0_CONFIG3
#define C0_CONFIG3_MASK_R    0
#define C0_CONFIG3_MASK_W    0
#define C0_CONFIG3_INIT_VAL  ((1<<13 /*USERLOCAL is implemented*/))
#define C0_CONFIG3_CONST     ((1<<13 /*USERLOCAL is implemented*/))

// Definitions for C0_EBASE
#define C0_EBASE_MASK_R      (0x3ffff<<12 /*base address of exception vectors*/) /* 1 for variable bits */
#define C0_EBASE_MASK_W      (0x3ffff<<12 /*base address of exception vectors*/) /* 1 for writable bits */
#define C0_EBASE_INIT_VAL    (1<<31)
#define C0_EBASE_CONST       (1<<31)

// Definitions for C0_PRID
#define MIPS_PRID_COMPANY_ID_MIPS_TECHNOLOGY 0x01
#define MIPS_PRID_PROCESSOR_ID_74K_CORE      0x97
#define C0_PRID_INIT_VAL     ((MIPS_PRID_COMPANY_ID_MIPS_TECHNOLOGY<<16) | (MIPS_PRID_PROCESSOR_ID_74K_CORE<<8))

// Definitions for C0_INTCTL

#define C0_INTCTL_TIMER_INT_IPNUM  7   /* IP num for Timer int.*/

#define C0_INTCTL_MASK_R      (0xf<<5 /* spacing between vectored interrupts */) /* 1 for variable bits */
#define C0_INTCTL_MASK_W      (0xf<<5 /* spacing between vectored interrupts */) /* 1 for writable bits */
#define C0_INTCTL_INIT_VAL    ((C0_INTCTL_TIMER_INT_IPNUM<<29) | (5<<26 /**/) | (4<<23 /**/))
#define C0_INTCTL_CONST       ((C0_INTCTL_TIMER_INT_IPNUM<<29) | (5<<26 /**/) | (4<<23 /**/))


// Definitions for C0_ENTRYHI
#define C0_ENTRYHI_MASK_R    (~0x00001f00)
#define C0_ENTRYHI_MASK_W    (~0x00001f00)
#define C0_ENTRYHI_INIT_VAL  0
#define C0_ENTRYHI_CONST     0

#define C0_ENTRYHI_BIT_ASID   0
#define C0_ENTRYHI_BIT_VPN2   13
#define C0_ENTRYHI_ASID_MASK  0xff
#define C0_ENTRYHI_VPN2_MASK  (~0x1fff)

// Definitions for C0_ENTRYLO0,1
#define C0_ENTRYLO0_MASK_R   (~0xfc000000)
#define C0_ENTRYLO0_MASK_W   (~0xfc000000)
#define C0_ENTRYLO0_INIT_VAL 0
#define C0_ENTRYLO0_CONST    0

#define C0_ENTRYLO1_MASK_R   (~0xfc000000)
#define C0_ENTRYLO1_MASK_W   (~0xfc000000)
#define C0_ENTRYLO1_INIT_VAL 0
#define C0_ENTRYLO1_CONST    0

// Definitions for C0_INDEX
#define C0_INDEX_MASK_R      (0x8000003f)
#define C0_INDEX_MASK_W      (0x8000003f)
#define C0_INDEX_INIT_VAL    0
#define C0_INDEX_CONST       0

#define C0_INDEX_BIT_P       31 /* Probe Failure */
#define C0_INDEX_INDEX_MASK  (0x3f) /* mask for TLB index */


// Definitions for C0_RANDOM
#define C0_RANDOM_MASK_R      (0x0000003f)
#define C0_RANDOM_MASK_W      (0x00000000)
#define C0_RANDOM_INIT_VAL    (NUM_TLB_ENTRY-1)
#define C0_RANDOM_CONST       0

// Definitions for C0_PAGEMASK
#define C0_PAGEMASK_MASK_R   (0xffff<<13)
#define C0_PAGEMASK_MASK_W   (0xffff<<13)
#define C0_PAGEMASK_INIT_VAL 0
#define C0_PAGEMASK_CONST    0

// Definitions for C0_WIRED
#define C0_WIRED_MASK_R      (0x3f)
#define C0_WIRED_MASK_W      (0x3f)
#define C0_WIRED_INIT_VAL    0
#define C0_WIRED_CONST       0

// Definitions for C0_HWRENA
#define C0_HWRENA_MASK_R      ( (1<<C0_HWRENA_BIT_UL) | (1<<C0_HWRENA_BIT_CCRES) | (1<<C0_HWRENA_BIT_CC) | (1<<C0_HWRENA_BIT_SYNCISTEP) | (1<<C0_HWRENA_BIT_CPUNUM) )
#define C0_HWRENA_MASK_W      ( (1<<C0_HWRENA_BIT_UL) | (1<<C0_HWRENA_BIT_CCRES) | (1<<C0_HWRENA_BIT_CC) | (1<<C0_HWRENA_BIT_SYNCISTEP) | (1<<C0_HWRENA_BIT_CPUNUM) )
#define C0_HWRENA_INIT_VAL    0
#define C0_HWRENA_CONST       0

#define C0_HWRENA_BIT_UL          29 /* enable "rdhwr 29" to read C0_USERLOCAL register in user mode */
#define C0_HWRENA_BIT_CCRES       3  /* enable "rdhwr 3" to read resolution of the CC register in user mode */
#define C0_HWRENA_BIT_CC          2  /* enable "rdhwr 2" to read high-resolution cycle counter, C0_COUNT, in user mode */
#define C0_HWRENA_BIT_SYNCISTEP   1  /* enable "rdhwr 1" to read address step size to be used with the SYNCI instruction in user mode */
#define C0_HWRENA_BIT_CPUNUM      0  /* enable "rdhwr 0" to read CPU ID number in user mode */


/*
Definitions for C0_STATUS

Values are from
"MIPS32 74K Processor Core Family Software User’s Manual," Revision 01.05, March 30, 2011, page 164-166.
*/
#define C0_STATUS_BIT_IE    0  /* Interrupt Enable */
#define C0_STATUS_BIT_EXL   1  /* Exception Level */
#define C0_STATUS_BIT_ERL   2  /* Error Level */
#define C0_STATUS_BIT_SM    3  /* Supervisor Mode */
#define C0_STATUS_BIT_UM    4  /* User Mode */
#define C0_STATUS_BIT_IM    8  /* Interrupt Mask */
#define C0_STATUS_IM_MASK   (0xff<<C0_STATUS_BIT_IM)
#define C0_STATUS_BIT_CEE   17 /* CorExtend Enable. Enable/disable CorExtend User Defined Instructions */
#define C0_STATUS_BIT_NMI   19 /* Indicates that the entry through the reset exception vector was due to an NMI */
#define C0_STATUS_BIT_SR    20 /* Soft Reset */
#define C0_STATUS_BIT_TS    21 /* TLB Shutdown */
#define C0_STATUS_BIT_BEV   22 /* Boot Exception Vector. Controls the location of exception vectors */
#define C0_STATUS_BIT_MX    24 /* MIPS Extension. Enables access to DSP ASE resources */
#define C0_STATUS_BIT_RE    25 /* Reverse Endian */
#define C0_STATUS_BIT_FR    26 /* Floating Register */
#define C0_STATUS_BIT_RP    27 /* Reduced Power */
#define C0_STATUS_BIT_CU0   28 /* Coprocessor 0 Usable (1: access allowed in user mode) */
#define C0_STATUS_BIT_CU1   29 /* Coprocessor 1 Usable */
#define C0_STATUS_BIT_CU2   30 /* Coprocessor 2 Usable */
#define C0_STATUS_BIT_CU3   31 /* Coprocessor 3 Usable */

//  UM SM
//   0  0 : Kernel
//   0  1 : Supervisor
//   1  0 : User
#define C0_STATUS_KSU_MASK   (0x3<<C0_STATUS_BIT_SM)
#define C0_STATUS_BIT_KSU    C0_STATUS_BIT_SM

// Debug mode is ignored in this emulator.
#define MODE_IS_IN_ERROR(status)     ((status) & (1<<C0_STATUS_BIT_ERL))
#define MODE_IS_IN_EXCEPTION(status) ((status) & (1<<C0_STATUS_BIT_EXL))

#define MODE_IS_EXCEPTION(status)     ((status) & ((1<<C0_STATUS_BIT_EXL) | (1<<C0_STATUS_BIT_ERL)))
#define MODE_IS_KERNEL(status)        ( (!MODE_IS_EXCEPTION(status)) && (((status) & C0_STATUS_KSU_MASK) == (0<<C0_STATUS_BIT_KSU)) )
#define MODE_IS_SUPERVISOR(status)    ( (!MODE_IS_EXCEPTION(status)) && (((status) & C0_STATUS_KSU_MASK) == (1<<C0_STATUS_BIT_KSU)) )
#define MODE_IS_USER(status)          ( (!MODE_IS_EXCEPTION(status)) && (((status) & C0_STATUS_KSU_MASK) == (2<<C0_STATUS_BIT_KSU)) )


/* 1: writable, 0: read-only */
#define C0_STATUS_MASK_R   (~((1<<C0_STATUS_BIT_CU3) | (1<<C0_STATUS_BIT_CU2) | (1<<C0_STATUS_BIT_RE) | (1<<C0_STATUS_BIT_SR) | (1<<23 /*reserved*/) | (7<<5 /*reserved*/) ))
#define C0_STATUS_MASK_W   (~((1<<C0_STATUS_BIT_CU3) | (1<<C0_STATUS_BIT_CU2) | (1<<C0_STATUS_BIT_RE) | (1<<C0_STATUS_BIT_SR) | (1<<23 /*reserved*/) | (7<<5 /*reserved*/) ))
#define C0_STATUS_INIT_VAL ((1<<C0_STATUS_BIT_BEV) | (1<<C0_STATUS_BIT_ERL))
#define C0_STATUS_CONST    (0)


/*
Exception Code values in ExcCode Field of Cause Register
from "MIPS32 74K Processor Core Family Software User’s Manual," Revision 01.05, March 30, 2011, page 175-176.
*/
#define C0_CAUSE_BIT_BD     31    /* Indicates whether the last exception taken occurred in a branch delay slot */
#define C0_CAUSE_BIT_TI     30    /* Timer Interrupt (1 when pending) */
#define C0_CAUSE_BIT_CE     28    /* Coprocessor unit number referenced when a Coprocessor Unusable exception */
#define C0_CAUSE_BIT_DC     27    /* Disable Count register */
#define C0_CAUSE_BIT_PCI    26    /* Performance Counter Interrupt */
#define C0_CAUSE_BIT_IV     23    /* Indicates whether an interrupt exception uses the general exception vector or a special interrupt vector */
#define C0_CAUSE_BIT_WP     22    /* a watch exception was deferred because StatusEXL or StatusERL was a one at the time the watch exception was detected */
#define C0_CAUSE_BIT_FDCI   21    /* Fast Debug Channel Interrupt */
#define C0_CAUSE_BIT_IP      8    /* pending interrupt or request for software interrupt */
#define C0_CAUSE_BIT_EXCCODE 2    /* pending interrupt or request for software interrupt */

#define C0_CAUSE_CE_MASK       (   3 << C0_CAUSE_BIT_CE)
#define C0_CAUSE_IP_MASK       (0xff << C0_CAUSE_BIT_IP)
#define C0_CAUSE_EXCCODE_MASK  (0x1f << C0_CAUSE_BIT_EXCCODE)

#define C0_CAUSE_MASK_R   (C0_CAUSE_CE_MASK | C0_CAUSE_IP_MASK | C0_CAUSE_EXCCODE_MASK | \
                            (1<<C0_CAUSE_BIT_BD) | \
                            (1<<C0_CAUSE_BIT_TI) | \
                            (1<<C0_CAUSE_BIT_DC) | \
                            (1<<C0_CAUSE_BIT_IV) | \
                            (1<<C0_CAUSE_BIT_WP) ) /* PCI, FDCI are ignored */
#define C0_CAUSE_MASK_W   ((1<<C0_CAUSE_BIT_DC) | (1<<C0_CAUSE_BIT_IV) | (1<<C0_CAUSE_BIT_WP) | (3<<C0_CAUSE_BIT_IP))
#define C0_CAUSE_INIT_VAL (0)
#define C0_CAUSE_CONST    (0)


#define REPORT_EXCEPT(x)     ((x) | 0x80 )
#define GET_EXCEPT_CODE(x)   ((x) & 0x7f )

/*
Exception Code values in ExcCode Field of Cause Register
from "MIPS32 74K Processor Core Family Software User’s Manual," Revision 01.05, March 30, 2011, page 175-176.
*/
#define EXCEPT_CODE_INTERRUPT              0  /* Interrupt */  
#define EXCEPT_CODE_MOD                    1  /* Store, but page marked as read-only in the TLB */
#define EXCEPT_CODE_TLB_LOAD               2  /* Load or fetch, but page marked as invalid in the TLB */
#define EXCEPT_CODE_TLB_STORE              3  /* Store, but page marked as invalid in the TLB */
#define EXCEPT_CODE_ADDR_ERR_LOAD          4  /* Address error on load/fetch. Address is either wrongly aligned, or a privilege violation. */
#define EXCEPT_CODE_ADDR_ERR_STORE         5  /* Address error on store. Address is either wrongly aligned, or a privilege violation. */
#define EXCEPT_CODE_BUS_ERR_IFETCH         6  /* Bus error signaled on instruction fetch */
#define EXCEPT_CODE_BUS_ERR_DATA           7  /* Bus error signaled on load/store */
#define EXCEPT_CODE_SYSCALL                8  /* System call, i.e. syscall instruction executed */
#define EXCEPT_CODE_BREAKPOINT             9  /* Breakpoint, i.e. break instruction executed */
#define EXCEPT_CODE_RESERVED_INSTRUCTION  10  /* Instruction code not recognized (or not legal) */
#define EXCEPT_CODE_COPROCESSOR_UNAVAIL   11  /* Instruction code was for a co-processor which is not enabled in StatusCU3-0 */
#define EXCEPT_CODE_INTEGER_OVERFLOW      12  /* Overflow from a trapping variant of integer arithmetic instructions */
#define EXCEPT_CODE_TRAP                  13  /* Condition met on one of the conditional trap instructions teq etc */
#define EXCEPT_CODE_FP_EXCEPTION          15  /* Floating point unit exception — more details in the FPU control/status registers */
#define EXCEPT_CODE_WATCH                 23  /* Instruction or data reference matched a watchpoint */
#define EXCEPT_CODE_MCHECK                24  /* "Machine check" */
#define EXCEPT_CODE_THREAD                25  /* Thread-related exception, only for CPUs supporting the MIPS MT ASE */
#define EXCEPT_CODE_DSP                   26  /* Tried to run an instruction from the MIPS DSP ASE, but it’s either not enabled or not available */
#define EXCEPT_CODE_CACHE_ERROR           30  /* Parity/ECC error somewhere in the core, on either instruction fetch, load or cache refill */

#define EXCEPT_CODE_TLB_REFILL_LOAD       32  /* virtual exception code for TLB Refill not used in hardware */
#define EXCEPT_CODE_TLB_REFILL_STORE      33  /* virtual exception code for TLB Refill not used in hardware */

#define EXCEPT_VECT_RESET                      0xbfc00000
#define EXCEPT_VECT_CacheErr(ebase, bev)       ((bev)?                      0xbfc00300  : (                  0xA0000100 |((ebase)&0x1ffff000)))
#define EXCEPT_VECT_TLBRefill(ebase, bev, exl) ((bev)? ((exl)? 0xbfc00380 : 0xbfc00200) : (((exl)?0x80000180:0x80000000)|((ebase)&0x3ffff000)))
#define EXCEPT_VECT_Int(ebase, bev, iv)        ((bev)? ((iv) ? 0xbfc00400 : 0xbfc00380) : (((iv )?0x80000200:0x80000180)|((ebase)&0x3ffff000)))
#define EXCEPT_VECT_AllOther(ebase, bev)       ((bev)?                      0xbfc00380  : (                  0x80000180 |((ebase)&0x3ffff000)))


/*
Definitions of pairs of a register number and a selector number for CP0 registers
and supporting macros.
*/
#define C0_REG_IDX2(reg,sel)         ((((reg) & ((1<<CP_REG_BITS)-1))<<CP_SEL_BITS) | ((sel) & ((1<<CP_SEL_BITS)-1)))

#define _C0_REG_NUM_HELPER(reg,sel)  C0_REG_IDX2(reg, sel)
#define C0_REG_IDX(regStr)           _C0_REG_NUM_HELPER(regStr##_REG, regStr##_SEL)

#define _C0_VAL_HELPER(pM, idx)      ((pM)->reg.cp0[ idx ])
#define C0_VAL(pM, regStr)           _C0_VAL_HELPER( (pM), C0_REG_IDX(regStr) )


#define C0_BADVADDR_REG        8
#define C0_BADVADDR_SEL        0

#define C0_CACHEERR_REG        27
#define C0_CACHEERR_SEL        0

#define C0_CAUSE_REG           13
#define C0_CAUSE_SEL           0

#define C0_CDMMBASE_REG        15
#define C0_CDMMBASE_SEL        2

#define C0_CMGCRBASE_REG       15
#define C0_CMGCRBASE_SEL       3

#define C0_COMPARE_REG         11
#define C0_COMPARE_SEL         0

#define C0_CONFIG_REG          16
#define C0_CONFIG_SEL          0

#define C0_CONFIG1_REG         16
#define C0_CONFIG1_SEL         1

#define C0_CONFIG2_REG         16
#define C0_CONFIG2_SEL         2

#define C0_CONFIG3_REG         16
#define C0_CONFIG3_SEL         3

#define C0_CONFIG7_REG         16
#define C0_CONFIG7_SEL         7

#define C0_CONTEXT_REG         4
#define C0_CONTEXT_SEL         0

#define C0_COUNT_REG           9
#define C0_COUNT_SEL           0

#define C0_DEBUG_REG           23
#define C0_DEBUG_SEL           0

#define C0_DEPC_REG            24
#define C0_DEPC_SEL            0

#define C0_DESAVE_REG          31
#define C0_DESAVE_SEL          0

#define C0_DDATALO_REG         28
#define C0_DDATALO_SEL         3

#define C0_DTAGLO_REG          28
#define C0_DTAGLO_SEL          2

#define C0_EBASE_REG           15
#define C0_EBASE_SEL           1

#define C0_ENTRYHI_REG         10
#define C0_ENTRYHI_SEL         0

#define C0_ENTRYLO0_REG        2
#define C0_ENTRYLO0_SEL        0

#define C0_ENTRYLO1_REG        3
#define C0_ENTRYLO1_SEL        0

#define C0_EPC_REG             14
#define C0_EPC_SEL             0

#define C0_ERRCTL_REG          26
#define C0_ERRCTL_SEL          0

#define C0_ERROREPC_REG        30
#define C0_ERROREPC_SEL        0

#define C0_HWRENA_REG          7
#define C0_HWRENA_SEL          0

#define C0_INDEX_REG           0
#define C0_INDEX_SEL           0

#define C0_INTCTL_REG          12
#define C0_INTCTL_SEL          1

#define C0_IDATAHI_REG         29
#define C0_IDATAHI_SEL         1

#define C0_IDATALO_REG         28
#define C0_IDATALO_SEL         1

#define C0_ITAGLO_REG          28
#define C0_ITAGLO_SEL          0

#define C0_L23DATAHI_REG       29
#define C0_L23DATAHI_SEL       5

#define C0_L23DATALO_REG       28
#define C0_L23DATALO_SEL       5

#define C0_L23TAGLO_REG        28
#define C0_L23TAGLO_SEL        4

#define C0_LLADDR_REG          17
#define C0_LLADDR_SEL          0

#define C0_MVPCONF0_REG        0
#define C0_MVPCONF0_SEL        2

#define C0_MVPCONF1_REG        0
#define C0_MVPCONF1_SEL        3

#define C0_MVPCONTROL_REG      0
#define C0_MVPCONTROL_SEL      1

#define C0_PAGEMASK_REG        5
#define C0_PAGEMASK_SEL        0

#define C0_PERFCNT0_REG        25
#define C0_PERFCNT0_SEL        1

#define C0_PERFCNT1_REG        25
#define C0_PERFCNT1_SEL        3

#define C0_PERFCNT2_REG        25
#define C0_PERFCNT2_SEL        5

#define C0_PERFCNT3_REG        25
#define C0_PERFCNT3_SEL        7

#define C0_PERFCTL0_REG        25
#define C0_PERFCTL0_SEL        0

#define C0_PERFCTL1_REG        25
#define C0_PERFCTL1_SEL        2

#define C0_PERFCTL2_REG        25
#define C0_PERFCTL2_SEL        4

#define C0_PERFCTL3_REG        25
#define C0_PERFCTL3_SEL        6

#define C0_PRID_REG            15
#define C0_PRID_SEL            0

#define C0_RANDOM_REG          1
#define C0_RANDOM_SEL          0

#define C0_SRSCONF0_REG        6
#define C0_SRSCONF0_SEL        1

#define C0_SRSCONF1_REG        6
#define C0_SRSCONF1_SEL        2

#define C0_SRSCONF2_REG        6
#define C0_SRSCONF2_SEL        3

#define C0_SRSCONF3_REG        6
#define C0_SRSCONF3_SEL        4

#define C0_SRSCONF4_REG        6
#define C0_SRSCONF4_SEL        5

#define C0_SRSCTL_REG          12
#define C0_SRSCTL_SEL          2

#define C0_SRSMAP_REG          12
#define C0_SRSMAP_SEL          3

#define C0_STATUS_REG          12
#define C0_STATUS_SEL          0

#define C0_TCBIND_REG          2
#define C0_TCBIND_SEL          2

#define C0_TCCONTEXT_REG       2
#define C0_TCCONTEXT_SEL       5

#define C0_TCHALT_REG          2
#define C0_TCHALT_SEL          4

#define C0_TCRESTART_REG       2
#define C0_TCRESTART_SEL       3

#define C0_TCSCHEDULE_REG      2
#define C0_TCSCHEDULE_SEL      6

#define C0_VPESCHEDULE_REG     1
#define C0_VPESCHEDULE_SEL     5

#define C0_TCSCHEFBACK_REG     2
#define C0_TCSCHEFBACK_SEL     7

#define C0_TCSTATUS_REG        2
#define C0_TCSTATUS_SEL        1

#define C0_TRACECONTROL_REG    23
#define C0_TRACECONTROL_SEL    1

#define C0_TRACECONTROL2_REG   23
#define C0_TRACECONTROL2_SEL   2

#define C0_TRACEDBPC_REG       23
#define C0_TRACEDBPC_SEL       5

#define C0_TRACEIBPC_REG       23
#define C0_TRACEIBPC_SEL       4

#define C0_USERLOCAL_REG       4
#define C0_USERLOCAL_SEL       2

#define C0_USERTRACEDATA_REG   23
#define C0_USERTRACEDATA_SEL   3

#define C0_VPECONF0_REG        1
#define C0_VPECONF0_SEL        2

#define C0_VPECONF1_REG        1
#define C0_VPECONF1_SEL        3

#define C0_VPECONTROL_REG      1
#define C0_VPECONTROL_SEL      1

#define C0_VPEOPT_REG          1
#define C0_VPEOPT_SEL          7

#define C0_WATCHHI0_REG        19
#define C0_WATCHHI0_SEL        0

#define C0_WATCHHI1_REG        19
#define C0_WATCHHI1_SEL        1

#define C0_WATCHHI2_REG        19
#define C0_WATCHHI2_SEL        2

#define C0_WATCHHI3_REG        19
#define C0_WATCHHI3_SEL        3

#define C0_WATCHLO0_REG        18
#define C0_WATCHLO0_SEL        0

#define C0_WATCHLO1_REG        18
#define C0_WATCHLO1_SEL        1

#define C0_WATCHLO2_REG        18
#define C0_WATCHLO2_SEL        2

#define C0_WATCHLO3_REG        18
#define C0_WATCHLO3_SEL        3

#define C0_WIRED_REG           6
#define C0_WIRED_SEL           0

#define C0_YQMASK_REG          1
#define C0_YQMASK_SEL          4


#endif