#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "rc_x64.h"
#include "rc.h"
#include "tlb.h"
#include "mem.h"
#include "mips_op.h"
#include "cp0.h"

#include "ExInst_common.h"

struct RCBin rc_array[RC_ARRAY_SIZE];
static uint32_t rc_counter;

void rc_init(void){
    memset(rc_array, 0, sizeof(rc_array));
    rc_counter = 0;

    long page_size = sysconf(_SC_PAGESIZE);
    for(int i=0; i<RC_ARRAY_SIZE; i++){
        if ((posix_memalign((void **)&(rc_array[i].compiled_bin), page_size, RC_COMPILE_MAX_BIN_SIZE))){
            FPRINTF(stderr, "posix_memalign failed\n");
            exit(EXIT_FAILURE);
        }
        if ((mprotect((void *)rc_array[i].compiled_bin, RC_COMPILE_MAX_BIN_SIZE, PROT_WRITE | PROT_EXEC)) == -1){
            FPRINTF(stderr, "mprotect failed (%d)\n", errno);
            exit(EXIT_FAILURE);
        }
    }
}

void rc_invalidate(uintmx addr){
    for(int i=0; i<RC_ARRAY_SIZE; i++){
        if( (addr & 0xffffffc0) == (rc_array[i].vaddr & 0xffffffc0) ){
            rc_array[i].vaddr = 0;
        }
    }
}

void rc_clear(void){
    rc_counter = 0;

    for(int i=0; i<RC_ARRAY_SIZE; i++){
        rc_array[i].vaddr     = 0;
    }
}

void rc_check_and_run(struct stMachineState *pM){
    uintmx pc;
    int run_count = 0;
    struct RCBin *rc;
    int count;

    // during restart loop process, CP0 values does not changed. If changed, restart loop ends.
    uint32_t c0_status     = C0_VAL(pM, C0_STATUS);
    uint32_t c0_entry_high = C0_VAL(pM, C0_ENTRYHI);
    uint32_t mode = (c0_status     & (C0_STATUS_KSU_MASK|(1<<C0_STATUS_BIT_ERL)|(1<<C0_STATUS_BIT_EXL)));
    uint32_t asid = (c0_entry_high & C0_ENTRYHI_ASID_MASK);

rc_check_and_run_restart:
    if( ++run_count > 100 ){
        return;
    }
    pc = pM->reg.pc;
    rc = &rc_array[ (pc>>2) & (RC_ARRAY_SIZE-1) ];
    rc_counter += 1;

    if( pc >= KSEG1 ) return ;

    if( RCBIN_MATCH(rc, pc, mode, asid) ){
        /*
        Some instructions may move a non-zero unnecessary result into $zero, such as "di" and "ei" instructions, 
        and no-checking is done in the C main loop.
        For every cycle, $zero is cleared to 0 in the loop.
        Thus, before beginning the execution, clearing the $zero is necessary.
        */
        pM->reg.r[0] = 0;
        rc->last_access = rc_counter;

        if( rc->compiled ){
            if( 0 != ((*rc->compiled_bin)(pM)) ){
                goto rc_check_and_run_restart;
            }
            return;
        }else{
            if( rc->acc_count >= 0 ){
                rc->acc_count ++;
            }
            if(rc->acc_count > RC_COMPILE_TH){

                count = ((pc&1) ? 
                    compile16(pM, (uint8_t *)rc->compiled_bin, pc) : 
                    compile32(pM, (uint8_t *)rc->compiled_bin, pc) );

                if( count > 0 ){
                    rc->compiled = 1;
                    if( 0 != ((*rc->compiled_bin)(pM)) ){
                        goto rc_check_and_run_restart;
                    }
                }else{
                    // unable to compile
                    rc->acc_count = -1000;
                }
                return;
            }else{
                return;
            }
        }
    }else{
        if( rc->acc_count < 2 || rc->vaddr == 0 || rc_counter - rc->last_access > 2000 ){
            rc->acc_count = 1;
            rc->asid      = asid;
            rc->mode      = mode;
            rc->vaddr     = pc;
            rc->compiled  = 0;
            rc->last_access = rc_counter;
        }
    }
}
