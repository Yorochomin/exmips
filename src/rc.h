#ifndef __RC_H__ 
#define __RC_H__ 

#include "mips.h"

struct RCBin{
    uintmx vaddr;
    int acc_count;
    uint32_t last_access;
    uint8_t compiled;
    uint8_t asid;
    uint8_t mode; // [4:1] mode

    uint8_t (*compiled_bin)(struct stMachineState *pM);
};

#define RC_COMPARE_BITS 12
#define RC_ARRAY_SIZE (1<<RC_COMPARE_BITS)
#define RC_COMPILE_TH  5
#define RC_COMPILE_MAX_BIN_SIZE (1024*8)

#define RCBIN_MATCH(rc, addr, mode, asid) \
( ((rc)->vaddr == (addr)) && ((addr) != 0) && ((rc)->asid == (asid)) && ((rc)->mode == (mode)) )


void rc_init(void);
void rc_clear(void);
void rc_invalidate(uintmx addr);
void rc_check_and_run(struct stMachineState *pM);



int lb_helper (struct stMachineState *pM, uintmx addr, int reg);
int lbu_helper(struct stMachineState *pM, uintmx addr, int reg);
int lh_helper (struct stMachineState *pM, uintmx addr, int reg);
int lhu_helper(struct stMachineState *pM, uintmx addr, int reg);
int lw_helper (struct stMachineState *pM, uintmx addr, int reg);

int sc_helper(struct stMachineState *pM, uintmx addr, uint32_t data);
int sw_helper(struct stMachineState *pM, uintmx addr, uint32_t data);
int sh_helper(struct stMachineState *pM, uintmx addr, uint32_t data);
int sb_helper(struct stMachineState *pM, uintmx addr, uint32_t data);

#endif
