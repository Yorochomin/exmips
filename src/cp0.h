#ifndef __CP0_H__
#define __CP0_H__


void    storeC0Counter(struct stMachineState *pM, uintmx val);
uintmx   loadC0Counter(struct stMachineState *pM);
uint64_t loadC0CounterLong(struct stMachineState *pM);
uintmx   loadC0CounterPrecise(struct stMachineState *pM);

void  storeCP0(struct stMachineState *pM, unsigned int reg, unsigned sel, uintmx val);
uintmx loadCP0(struct stMachineState *pM, unsigned int reg, unsigned sel);

#endif