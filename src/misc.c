#include <stdio.h>
#include <stdint.h>

#include "mips.h"
#include "logfile.h"
#include "cp0.h"

const char *regStr[32] = {
    //   0      1      2      3      4      5      6      7
    "$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
    //   8      9     10     11     12     13     14     15
      "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
    //  16     17     18     19     20     21     22     23
      "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
    //  24     25     26     27     28     29     30     31
      "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"
};


void log_printReg32(int loglevel, struct stReg *preg){
    logfile_printf(loglevel, "PC = %08x C0_STATUS = %x\n", preg->pc,  preg->cp0[ C0_REG_IDX(C0_STATUS) ]);
    logfile_printf(loglevel, "r[ 0.. 7]=%08x %08x %08x %08x %08x %08x %08x %08x\n", preg->r[ 0], preg->r[ 1], preg->r[ 2], preg->r[ 3], preg->r[ 4], preg->r[ 5], preg->r[ 6], preg->r[ 7]);
    logfile_printf(loglevel, "r[ 8..15]=%08x %08x %08x %08x %08x %08x %08x %08x\n", preg->r[ 8], preg->r[ 9], preg->r[10], preg->r[11], preg->r[12], preg->r[13], preg->r[14], preg->r[15]);
    logfile_printf(loglevel, "r[16..23]=%08x %08x %08x %08x %08x %08x %08x %08x\n", preg->r[16], preg->r[17], preg->r[18], preg->r[19], preg->r[20], preg->r[21], preg->r[22], preg->r[23]);
    logfile_printf(loglevel, "r[24..31]=%08x %08x %08x %08x %08x %08x %08x %08x\n", preg->r[24], preg->r[25], preg->r[26], preg->r[27], preg->r[28], preg->r[29], preg->r[30], preg->r[31]);
}

uint32_t parseHex(char *str){
    uint32_t addr = 0, nd;
    int count = 0;
    if( str[0] == '0' && (str[1] == 'x' || str[1] == 'X') ){
        str = str + 2;
    }

    for(count=0; count<8 && str[count] != '\0'; count++ ){
        nd = 0x10;
        if( str[count] >= '0' && str[count] <= '9' ) nd = str[count] - '0';
        if( str[count] >= 'a' && str[count] <= 'f' ) nd = str[count] - 'a' + 0xa;
        if( str[count] >= 'A' && str[count] <= 'F' ) nd = str[count] - 'A' + 0xa;

        if(nd >= 0x10) break;

        addr = (addr<<4) + nd;
    }
    return addr;
}

uint32_t parseDec(char *str){
    uint32_t res = 0, nd;
    int count = 0;

    if( str[0] == '0' && (str[1] == 'x' || str[1] == 'X') ){
        return parseHex(str);
    }

    for(count=0; str[count] != '\0'; count++ ){
        nd = 10;
        if( str[count] >= '0' && str[count] <= '9' ) nd = str[count] - '0';

        if(nd >= 0x10) break;

        res = res * 10 + nd;
    }
    return res;
}

