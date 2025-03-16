#ifndef __MEM_H__ 
#define __MEM_H__ 

#define PREFETCH_VM_MEM(pM, addr, isWrite, locality) __builtin_prefetch(&((pM)->mem.stMem.mem0[(addr)]), (isWrite), (locality));
#define PREFETCH(addr, isWrite, locality)            __builtin_prefetch(                        addr   , (isWrite), (locality));


uintmx getPhysicalAddress(struct  stMachineState *pM, uintmx addr, int isWrite, int *perror);
uint32_t fetchCodeWord(struct stMachineState *pM);

uint8_t loadByte(struct stMachineState *pM, uintmx addr, int *perror);
uintmx loadHalfWord(struct stMachineState *pM, uintmx addr, int *perror);
uintmx loadWord(struct stMachineState *pM, uintmx addr, int *perror);

void storeByte(struct stMachineState *pM, uintmx addr, uint8_t  data, int *perror);
void storeHalfWord(struct stMachineState *pM, uintmx addr, uintmx data, int *perror);
void storeWord(struct stMachineState *pM, uintmx addr, uintmx data, int *perror);

uint32_t loadPhysMemWord(struct stMachineState *pM, uintmx addr);
void storePhysMemWord(struct stMachineState *pM, uintmx addr, uintmx data);

uint32_t loadPhysMemHalfWord(struct stMachineState *pM, uintmx addr);

uint8_t loadPhysMemByte(struct stMachineState *pM, uintmx addr);
void storePhysMemByte(struct stMachineState *pM, uintmx addr, uintmx data);

#endif
