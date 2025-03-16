#include <stdio.h>
#include <stdint.h>
#include <signal.h>

#include "mips.h"
#include "cp0.h"
#include "tlb.h"
#include "mem.h"

#define TLB_CACHE_BITS 8
#define TLB_CACHE_SIZE (1<<TLB_CACHE_BITS)
int8_t tlb_cache[TLB_CACHE_SIZE];

static int TLBWrite(struct stMachineState *pM, int idx){
    int entryhi  = C0_VAL(pM, C0_ENTRYHI);
    int entrylo0 = C0_VAL(pM, C0_ENTRYLO0);
    int entrylo1 = C0_VAL(pM, C0_ENTRYLO1);

    clearAddrCache(&pM->reg.pc_cache);
    clearAddrCache(&pM->reg.dr_cache);
    clearAddrCache(&pM->reg.dw_cache);

    idx &= C0_INDEX_INDEX_MASK;

    if(idx >= NUM_TLB_ENTRY ){
        idx = idx % NUM_TLB_ENTRY;
    }

    tlb_cache[ (pM->reg.tlb[idx].entryhi >> 12) & (TLB_CACHE_SIZE-1) ] = 0;
    tlb_cache[ (entryhi                  >> 12) & (TLB_CACHE_SIZE-1) ] = idx + 1;

    pM->reg.tlb[idx].entryhi  = entryhi;
    pM->reg.tlb[idx].entrylo0 = entrylo0;
    pM->reg.tlb[idx].entrylo1 = entrylo1;

    pM->reg.tlb[idx].field_vpn2 = (entryhi & C0_ENTRYHI_VPN2_MASK);
    pM->reg.tlb[idx].field_asid = (entryhi & C0_ENTRYHI_ASID_MASK);
    pM->reg.tlb[idx].field_g    = (entrylo0 & entrylo1 & 1);
    pM->reg.tlb[idx].field_pmask= C0_VAL(pM, C0_PAGEMASK);

    pM->reg.tlb[idx].lo[0].field_valid  = (entrylo0 & 2);
    pM->reg.tlb[idx].lo[0].field_dirty  = (entrylo0 & 4);
    pM->reg.tlb[idx].lo[0].field_pfn    =((entrylo0<<6) & 0xfffff000);

    pM->reg.tlb[idx].lo[1].field_valid  = (entrylo1 & 2);
    pM->reg.tlb[idx].lo[1].field_dirty  = (entrylo1 & 4);
    pM->reg.tlb[idx].lo[1].field_pfn    =((entrylo1<<6) & 0xfffff000);

    return 0;
}

int TLBWriteIndex(struct stMachineState *pM){
    int idx = C0_VAL(pM, C0_INDEX);

    return TLBWrite(pM, idx);
}

int TLBWriteRandom(struct stMachineState *pM){
    int idx = C0_VAL(pM, C0_RANDOM);

    // Relation between C0_WIRED and C0_RANDOM
    // 0 <= C0_WIRED <= C0_RANDOM < NUM_TLB_ENTRY

    if( idx - 1 < (intmx)(C0_VAL(pM,C0_WIRED) & C0_INDEX_INDEX_MASK) ){
        C0_VAL(pM, C0_RANDOM) = NUM_TLB_ENTRY -1;
    }else if( idx - 1 >= NUM_TLB_ENTRY  ){
        C0_VAL(pM, C0_RANDOM) = NUM_TLB_ENTRY -1;
    }else{
        C0_VAL(pM, C0_RANDOM) = idx - 1;
    }

    return TLBWrite(pM, idx);
}

int TLBProbe(struct stMachineState *pM){
    uint32_t addr = (C0_VAL(pM,C0_ENTRYHI) & C0_ENTRYHI_VPN2_MASK);
    uint32_t asid = (C0_VAL(pM,C0_ENTRYHI) & C0_ENTRYHI_ASID_MASK);

    for(int i=0; i<NUM_TLB_ENTRY; i++){
        uint32_t addrMask   = (0xfff | pM->reg.tlb[i].field_pmask );
        uint32_t addrMask2  = ((addrMask<<1) | 1);
        uint32_t maskedAddr = (addr                     & (~addrMask2));
        uint32_t maskedVPN2 = (pM->reg.tlb[i].field_vpn2& (~addrMask2));

        if( (maskedVPN2 == maskedAddr) &&
            ((pM->reg.tlb[i].field_g) || (asid == pM->reg.tlb[i].field_asid)) 
        ){
            C0_VAL(pM, C0_INDEX) = i;
            return 0;
        }
    }

    C0_VAL(pM, C0_INDEX) |= (1<<C0_INDEX_BIT_P);
    return 0;
}

/*
TLB lookup:

See Section 5.4 (p.100-104) of
"MIPS32 74K Processor Core Family Software User’s Manual," Revision 01.05, March 30, 2011, page 175-176.
*/
int TLBLookUp(struct stMachineState *pM, unsigned int asid, int isWrite, uintmx addr, int *perror){

    *perror = REPORT_EXCEPT(isWrite ? EXCEPT_CODE_TLB_REFILL_STORE : EXCEPT_CODE_TLB_REFILL_LOAD);

    int idx = tlb_cache[ (addr >> 12) & (TLB_CACHE_SIZE-1) ] - 1;

    if(idx >= 0){
        uint32_t addrMask   = (0xfff | pM->reg.tlb[idx].field_pmask );
        uint32_t addrMask2  = ((addrMask<<1) | 1);
        uint32_t vpnlsb     = (addrMask2 ^ addrMask);

        uint32_t maskedAddr = (addr                       & (~addrMask2));
        uint32_t maskedVPN2 = (pM->reg.tlb[idx].field_vpn2& (~addrMask2));
        int odd             =((addr & vpnlsb      ) ? 1 : 0);

        if( (maskedVPN2 == maskedAddr) && ( (pM->reg.tlb[idx].field_g) || (asid == pM->reg.tlb[idx].field_asid) ) ){
            if( ! pM->reg.tlb[idx].lo[odd].field_valid ){
                *perror = REPORT_EXCEPT( isWrite ? EXCEPT_CODE_TLB_STORE : EXCEPT_CODE_TLB_LOAD );
                return 0;
            }
            if( isWrite && (!pM->reg.tlb[idx].lo[odd].field_dirty) ){
                *perror = REPORT_EXCEPT( EXCEPT_CODE_MOD );
                return 0;
            }

            *perror = 0;
            uintmx result_addr = ((pM->reg.tlb[idx].lo[odd].field_pfn) | (addr & addrMask));
            PREFETCH_VM_MEM(pM, result_addr, 0, 0);
            return result_addr;
        }
    }

    for(int i=0; i<NUM_TLB_ENTRY; i++){
        uint32_t addrMask   = (0xfff | pM->reg.tlb[i].field_pmask );
        uint32_t addrMask2  = ((addrMask<<1) | 1);
        uint32_t vpnlsb     = (addrMask2 ^ addrMask);

        uint32_t maskedAddr = (addr                     & (~addrMask2));
        uint32_t maskedVPN2 = (pM->reg.tlb[i].field_vpn2& (~addrMask2));
        int odd             =((addr & vpnlsb      ) ? 1 : 0);

        if( maskedVPN2 != maskedAddr ) continue;

        if( (!pM->reg.tlb[i].field_g) && (asid != pM->reg.tlb[i].field_asid)){
            //*perror = EXCEPT_CODE_TLB_REFILL;
            continue;
        }

        if( ! pM->reg.tlb[i].lo[odd].field_valid ){
            *perror = REPORT_EXCEPT( isWrite ? EXCEPT_CODE_TLB_STORE : EXCEPT_CODE_TLB_LOAD );
            return 0;
        }

        if( isWrite && (!pM->reg.tlb[i].lo[odd].field_dirty) ){
            *perror = REPORT_EXCEPT( EXCEPT_CODE_MOD );
            return 0;
        }

        *perror = 0;
        uintmx result_addr = ((pM->reg.tlb[i].lo[odd].field_pfn) | (addr & addrMask));
        PREFETCH_VM_MEM(pM, result_addr, 0, 0);
        tlb_cache[ (addr >> 12) & (TLB_CACHE_SIZE-1) ] = i+1;

        return result_addr;
    }

    return 0;
}