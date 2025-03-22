#ifndef __ENV_UNIX_H__
#define __ENV_UNIX_H__

#define PRINTF(...)  printf(__VA_ARGS__)
#define FPRINTF(...) fprintf(__VA_ARGS__)
#define FLUSH_STDOUT() fflush(stdout)

#include <signal.h>
extern volatile sig_atomic_t intflag;
extern volatile sig_atomic_t timerflag;

#define getIntFlag(pM)    (intflag)
#define getTimerFlag(pM)  (timerflag)

int initEmulator(struct stMachineState *pM, int argc, char *argv[]);
void shutdownEmulator(struct stMachineState *pM);


void initTerminalSetting(struct stMachineState *pM);
void saveTerminalSetting(struct stMachineState *pM);
void restoreTerminalSetting(struct stMachineState *pM);

void resetTimerFlag(struct stMachineState *pM);

void delayUSec(struct stMachineState *pM, uint32_t delay_in_Usec);
void setTimerInUSec(struct stMachineState *pM, uint32_t interval_in_Usec);
uint64_t getTimeInUSec(struct stMachineState *pM);


#endif
