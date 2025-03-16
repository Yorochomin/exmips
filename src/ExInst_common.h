#ifndef __EXINST_COMMON_H__ 
#define __EXINST_COMMON_H__ 

#include "config.h"

#if USE_RUNTIME_COMPILATION
    #include "rc.h"
#endif

#define EXI_LOGLEVEL             (LOGCAT_CPU_EXE | LOGLV_VERBOSE)
#define EXI_LOG_PRINTF(...)      (logfile_printf_without_header(EXI_LOGLEVEL, __VA_ARGS__))
#define EXI_LOG_ERR_PRINTF(...)  (logfile_printf(EXI_LOGLEVEL, __VA_ARGS__))

//------------------------------------------------------------
// Maintenance MACROs to update program counter
//------------------------------------------------------------

#define REG_PC(pM)         ((pM)->reg.pc)

#define UPDATE_PC_NEXT_WITH_DELAYED_IMM( pM, imm )                       \
do{                                                                      \
    (pM)->reg.pc_delay = (imm);                                          \
    (pM)->reg.pc_prev_jump = (pM)->reg.pc;                               \
    (pM)->reg.pc += 4;                                                   \
    (pM)->reg.delay_en = 1;                                              \
}while(0)

#define UPDATE_PC_NEXT32_WITH_DELAYED_IMM( pM, imm ) \
 UPDATE_PC_NEXT_WITH_DELAYED_IMM( pM, imm ) 

#define UPDATE_PC_NEXT16_WITH_DELAYED_IMM( pM, imm )                     \
do{                                                                      \
    (pM)->reg.pc_delay = (imm);                                          \
    (pM)->reg.pc_prev_jump = (pM)->reg.pc;                               \
    (pM)->reg.pc += 2;                                                   \
    (pM)->reg.delay_en = 1;                                              \
}while(0)

#if USE_RUNTIME_COMPILATION
    #define UPDATE_PC_IMM( pM, imm )                                     \
    do{                                                                  \
        (pM)->reg.pc_prev_jump = (pM)->reg.pc;                           \
        (pM)->reg.pc = (imm);                                            \
        (pM)->reg.delay_en = 0;                                          \
        rc_check_and_run( pM );                                          \
    }while(0)
#else
    #define UPDATE_PC_IMM( pM, imm )                                     \
    do{                                                                  \
        (pM)->reg.pc_prev_jump = (pM)->reg.pc;                           \
        (pM)->reg.pc = (imm);                                            \
        (pM)->reg.delay_en = 0;                                          \
    }while(0)
#endif

#if USE_RUNTIME_COMPILATION
    #define UPDATE_PC_NEXT( pM )                                         \
    do{                                                                  \
        if( (pM)->reg.delay_en ){                                        \
            (pM)->reg.pc = ((pM)->reg.pc_delay);                         \
            (pM)->reg.delay_en = 0;                                      \
            rc_check_and_run( pM );                                      \
        }else{                                                           \
            (pM)->reg.pc += 4;                                           \
        }                                                                \
    }while(0)
#else
    #define UPDATE_PC_NEXT( pM )                                         \
    do{                                                                  \
        if( (pM)->reg.delay_en ){                                        \
            (pM)->reg.pc = ((pM)->reg.pc_delay);                         \
            (pM)->reg.delay_en = 0;                                      \
        }else{                                                           \
            (pM)->reg.pc += 4;                                           \
        }                                                                \
    }while(0)
#endif

#if USE_RUNTIME_COMPILATION
    #define UPDATE_PC_NEXT_BRANCH_FAIL( pM )                             \
    do{                                                                  \
        (pM)->reg.pc += 4;                                               \
        (pM)->reg.delay_en = 0;                                          \
        rc_check_and_run( pM );                                          \
    }while(0)
#else
    #define UPDATE_PC_NEXT_BRANCH_FAIL(pM) UPDATE_PC_NEXT( pM )          
#endif

#define UPDATE_PC_NEXT32( pM ) UPDATE_PC_NEXT( pM )
#define UPDATE_PC_NEXT32_BRANCH_FAIL(pM) UPDATE_PC_NEXT_BRANCH_FAIL( pM )          

#if USE_RUNTIME_COMPILATION
    #define UPDATE_PC_NEXT16( pM )                                       \
    do{                                                                  \
        if( (pM)->reg.delay_en ){                                        \
            (pM)->reg.pc = ((pM)->reg.pc_delay);                         \
            (pM)->reg.delay_en = 0;                                      \
            rc_check_and_run( pM );                                      \
        }else{                                                           \
            (pM)->reg.pc += 2;                                           \
        }                                                                \
    }while(0)
#else
    #define UPDATE_PC_NEXT16( pM )                                       \
    do{                                                                  \
        if( (pM)->reg.delay_en ){                                        \
            (pM)->reg.pc = ((pM)->reg.pc_delay);                         \
            (pM)->reg.delay_en = 0;                                      \
        }else{                                                           \
            (pM)->reg.pc += 2;                                           \
        }                                                                \
    }while(0)
#endif

#if USE_RUNTIME_COMPILATION
    #define UPDATE_PC_NEXT16_BRANCH_FAIL( pM )                           \
    do{                                                                  \
        (pM)->reg.pc += 2;                                               \
        (pM)->reg.delay_en = 0;                                          \
        rc_check_and_run( pM );                                          \
    }while(0)
#else
    #define UPDATE_PC_NEXT16_BRANCH_FAIL( pM ) UPDATE_PC_NEXT16( pM )
#endif

#define SIGN_EXT16(x)    ((uintmx)((intmx)((int16_t)(x))))
#define ZERO_EXT16(x)    ((uintmx)((uint16_t)(x)))

#define SIGN_EXT8(x)     ((uintmx)((intmx)((int8_t)(x))))
#define ZERO_EXT8(x)     ((uintmx)((uint8_t)(x)))

#define SIGN_EXT15(x)    ((intmx)((int16_t)(((int32_t)((int16_t)((x)<<1)))>>1)))
#define SIGN_EXT11(x)    ((intmx)((int16_t)(((int32_t)((int16_t)((x)<<5)))>>5)))

#define SIGN_EXT4(x)     ((intmx)((int16_t)(((int32_t)((int8_t)((x)<<4)))>>4)))
#define ZERO_EXT4(x)     ((uintmx)((uint8_t)((x)&0xf)))

// Used in MIPS16e instructions
#define XLAT(x) ((x)<2 ? ((x)+16) : (x))

#define SIGN_EXT(num, bit) (((num) & (1<<(bit))) ? ((num) | (~((1ULL<<(bit)) - 1ULL))) : (num))
#define BT(i,b0,b1)  ( (((i)>>(b0))&1) << (b1) )

#endif