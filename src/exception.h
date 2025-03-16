#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

void prepareException(struct stMachineState *pM, int exceptCode, uintmx option);
void prepareInterrupt(struct stMachineState *pM, int intCode);


#endif
