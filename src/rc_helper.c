#include <stdio.h>

#include "mips.h"
#include "ExInst_common.h"
#include "exception.h"
#include "mem.h"


int lb_helper(struct stMachineState *pM, uintmx addr, int reg){
    int accError;

    uintmx loadData = SIGN_EXT8( loadByte(pM, addr, &accError) );
    if( accError ){
        prepareException(pM, GET_EXCEPT_CODE(accError), addr);
        return 1;
    }else{
        pM->reg.r[reg] = loadData;
    }
    return 0;
}

int lbu_helper(struct stMachineState *pM, uintmx addr, int reg){
    int accError;

    uintmx loadData = loadByte(pM, addr, &accError);
    if( accError ){
        prepareException(pM, GET_EXCEPT_CODE(accError), addr);
        return 1;
    }else{
        pM->reg.r[reg] = loadData;
    }
    return 0;
}

int lh_helper(struct stMachineState *pM, uintmx addr, int reg){
    int accError;

    uintmx loadData = SIGN_EXT16( loadHalfWord(pM, addr, &accError) );
    if( accError ){
        prepareException(pM, GET_EXCEPT_CODE(accError), addr);
        return 1;
    }else{
        pM->reg.r[reg] = loadData;
    }
    return 0;
}

int lhu_helper(struct stMachineState *pM, uintmx addr, int reg){
    int accError;

    uintmx loadData = loadHalfWord(pM, addr, &accError);
    if( accError ){
        prepareException(pM, GET_EXCEPT_CODE(accError), addr);
        return 1;
    }else{
        pM->reg.r[reg] = loadData;
    }
    return 0;
}

int lw_helper(struct stMachineState *pM, uintmx addr, int reg){
    int accError;

    uintmx loadData = loadWord(pM, addr, &accError);
    if( accError ){
        prepareException(pM, GET_EXCEPT_CODE(accError), addr);
        return 1;
    }else{
        pM->reg.r[reg] = loadData;
    }
    return 0;
}

int sc_helper(struct stMachineState *pM, uintmx addr, uint32_t data){
    int accError;

    if( pM->reg.ll_sc ){
        storeWord(pM, addr, data, &accError);
        if( accError ){
            prepareException(pM, GET_EXCEPT_CODE(accError), addr);
            return 1;
        }
    }
    return 0;
}

int sw_helper(struct stMachineState *pM, uintmx addr, uint32_t data){
    int accError;

    storeWord(pM, addr, data, &accError);
    if( accError ){
        prepareException(pM, GET_EXCEPT_CODE(accError), addr);
        return 1;
    }
    return 0;
}

int sh_helper(struct stMachineState *pM, uintmx addr, uint32_t data){
    int accError;

    storeHalfWord(pM, addr, data, &accError);
    if( accError ){
        prepareException(pM, GET_EXCEPT_CODE(accError), addr);
        return 1;
    }
    return 0;
}

int sb_helper(struct stMachineState *pM, uintmx addr, uint32_t data){
    int accError;

    storeByte(pM, addr, data, &accError);
    if( accError ){
        prepareException(pM, GET_EXCEPT_CODE(accError), addr);
        return 1;
    }
    return 0;
}