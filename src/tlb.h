#ifndef __TLB_H__
#define __TLB_H__

int TLBWriteIndex(struct stMachineState *pM);
int TLBWriteRandom(struct stMachineState *pM);
int TLBProbe(struct stMachineState *pM);

int TLBLookUp(struct stMachineState *pM, unsigned int asid, int isWrite, uintmx addr, int *perror);

#endif