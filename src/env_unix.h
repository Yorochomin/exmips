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

//int getIntFlag(struct stMachineState *pM);
//int getTimerFlag(struct stMachineState *pM);
void resetTimerFlag(struct stMachineState *pM);

void delayUSec(struct stMachineState *pM, uint32_t delay_in_Usec);
void setTimerInUSec(struct stMachineState *pM, uint32_t interval_in_Usec);
uint64_t getTimeInUSec(struct stMachineState *pM);

uint32_t getDriveSize(struct stMachineState *pM, int driveNum);
size_t readDriveSector (struct stMachineState *pM, int driveNum, int sect, uint8_t *buf);
size_t writeDriveSector(struct stMachineState *pM, int driveNum, int sect, uint8_t *buf);


size_t readHostFile (struct stMachineState *pM, char *file, size_t offset, size_t len, uint8_t *buf);
size_t writeHostFile(struct stMachineState *pM, char *file, char *mode, size_t len, uint8_t *buf);

void setSystemTime(struct stMachineState *pM, struct tm *pT);

#endif
