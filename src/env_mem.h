#ifndef __ENV_MEM_H__
#define __ENV_MEM_H__

#include <stdint.h>
#include "config.h"

struct stMemRegion {
    uint8_t *mem0;
#if !USE_SPI_EMULATION
    uint8_t *mem1;
#endif
};

#define MEM_READ0( pM, addr)      ((pM)->mem.stMem.mem0[(addr)])
#define MEM_WRITE0(pM, addr,val)  ((pM)->mem.stMem.mem0[(addr)] = (val))

#if !USE_SPI_EMULATION
#define MEM_READ1( pM, addr)      ((pM)->mem.stMem.mem1[(addr)])
#define MEM_WRITE1(pM, addr,val)  ((pM)->mem.stMem.mem1[(addr)] = (val))
#endif

#endif
