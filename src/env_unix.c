#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mips.h"
#include "misc.h"
#include "logfile.h"
#include "terminal.h"
#include "mem.h"
#include "time.h"
#include "dev_gmac.h"
#include "dev_SPIFlash.h"

#include "env_unix.h"

//-----------------------------------------------------
//   signal handler                      
//-----------------------------------------------------
volatile sig_atomic_t intflag = 0;
volatile sig_atomic_t timerflag;

void sig_handler_SIGINT(int signum) {
    intflag = intflag + 1;
}

void timer_handler(int signum){
  switch (signum) {
  case SIGALRM:
    timerflag = 1;
    break;

  default:
    break;
  }
}

//-----------------------------------------------------
// Terminal setting
//-----------------------------------------------------
struct termios termOrigSettings;
//-----------------------------------------------------

void resetTimerFlag(struct stMachineState *pM){
    timerflag = 0;
}

void delayUSec(struct stMachineState *pM, uint32_t delay_in_Usec){
    usleep(delay_in_Usec);
}

uint64_t getTimeInUSec(struct stMachineState *pM){
    struct timeval TimevalTime;

    gettimeofday(&TimevalTime, NULL);
    return (((uint64_t)TimevalTime.tv_sec) * 1000UL*1000UL) + (((uint64_t)TimevalTime.tv_usec));
}

void setTimerInUSec(struct stMachineState *pM, uint32_t interval_in_Usec){
    struct sigaction action, old_action;
    struct itimerval timer, old_timer;

    memset(&    action, 0, sizeof(    action));
    memset(&old_action, 0, sizeof(old_action));

    action.sa_handler = timer_handler;
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGALRM, &action, &old_action) == -1) {
        logfile_printf(LOGCAT_IO_TIMER | LOGLV_ERROR, "Timer: timer setting error (failed to set sigaction)\n");
        logfile_close();
        shutdownEmulator(pM);
    }

    timer.it_value.tv_sec     = 0;
    timer.it_value.tv_usec    = interval_in_Usec;
    timer.it_interval.tv_sec  = 0;
    timer.it_interval.tv_usec = interval_in_Usec;
    if (setitimer(ITIMER_REAL, &timer, &old_timer) == -1) {
        logfile_printf(LOGCAT_IO_TIMER | LOGLV_ERROR, "Timer: timer setting error\n");
        logfile_close();
        shutdownEmulator(pM);
    }
}


void printUsage(void){
    fprintf(stderr, "MIPS emulator\n\n");
    fprintf(stderr, "Usage: \n");
    fprintf(stderr, " exmips [Options...] [Memory image file]\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, " -b [addr]     : enables breakpoint \n");
    fprintf(stderr, " -bm [mask]    : enables breakpoint mask \n");
    fprintf(stderr, " -c [count]    : enables break after executing the specified number of instructions\n");
    fprintf(stderr, " -r [#insts]   : specifies number of instruction execusions after break (in hexadecimal)\n");
    fprintf(stderr, " -w [addr]     : enables address write watch \n");
    fprintf(stderr, " -f [MByte]    : specifies the capacity of SPI flash memory\n");
    fprintf(stderr, "               : (default: 8MByte, 8 and 256 MBytes are possible) \n");
    fprintf(stderr, " -s            : enables write-back the flash image into the file when exiting the emulator \n");
    fprintf(stderr, " -dm           : dump memory image to dump.txt during exiting\n");
    fprintf(stderr, " -tap [name]   : specifies the TAP interface name\n");
    fprintf(stderr, " -h            : prints this message\n");
    fprintf(stderr, " -debug        : enables debug mode\n");
    fprintf(stderr, "\n");
}


int initEmulator(struct stMachineState *pM, int argc, char *argv[]){
    int image_specified = 0;

    if( argc < 2 ){
        printUsage();
        exit(1);
    }

    pM->mem.stMem.mem0 = malloc(EMU_MEM0_SIZE);
    if( pM->mem.stMem.mem0 == NULL ){
        fprintf(stderr, "Memory allocation for a virtual machine RAM was failed.\n");
        exit(1);
    }
    memset(pM->mem.stMem.mem0, 0xff, EMU_MEM0_SIZE);

#if !USE_SPI_EMULATION
    pM->mem.stMem.mem1 = malloc(EMU_MEM1_SIZE);
    if( pM->mem.stMem.mem1 == NULL ){
        fprintf(stderr, "Memory allocation for a virtual machine ROM was failed.\n");
        exit(1);
    }
    memset(pM->mem.stMem.mem1, 0, EMU_MEM1_SIZE);
#endif

    for(int i=1; i<argc; i++){
        if(!strcmp("-debug", argv[i])){
#ifndef DEBUG
            DEBUG = 1;
#endif
        }else if(!strcmp("-b", argv[i])){
            pM->emu.breakPoint = (i+1 < argc) ? parseHex( argv[i+1] ) : 0;
            i++;
        }else if(!strcmp("-bm", argv[i])){
            pM->emu.breakMask = (i+1 < argc) ? parseHex( argv[i+1] ) : 0;
            pM->emu.breakMask = ~pM->emu.breakMask;
            i++;
        }else if(!strcmp("-c", argv[i])){
            pM->emu.breakCounter = (i+1 < argc) ? parseDec( argv[i+1] ) : 0;
            i++;
        }else if(!strcmp("-f", argv[i])){
            int flash_capacity = (i+1 < argc) ? parseDec( argv[i+1] ) : 8;
            switch (flash_capacity)
            {
                case 256: pM->emu.spiflash_param = &spiflashparam_mx66u2g45g; break;
                default:  pM->emu.spiflash_param = &spiflashparam_s25fl164k;  break;
            }
            i++;
        }else if(!strcmp("-r", argv[i])){
            pM->emu.runAfterBreak = (i+1 < argc) ? parseDec( argv[i+1] ) : 0;
            i++;
        }else if(!strcmp("-w", argv[i])){
            pM->mem.watchAddr = (i+1 < argc) ? parseHex( argv[i+1] ) : 0;
            i++;
        }else if(!strcmp("-s", argv[i])){
            pM->emu.bWriteBackFlashData = 1;
        }else if(!strcmp("-h", argv[i])){
            printUsage();
            return 0;
        }else if(!strcmp("-tap", argv[i])){
            setGMACXTAPIFName(pM, &(pM->mem.ioGMACX0), argv[i+1]);
            i++;
        }else{
            image_specified = 1;
            setFlashImageName(pM, 0, argv[i]);

#if !USE_SPI_EMULATION
            FILE    *fp         = NULL;
            char    *filename   = NULL;
            filename = argv[i];
            fp = fopen(filename, "rb");
            if( fp == NULL ){
                fprintf(stderr, " Failed to open \"%s\" \n", filename);
                exit(1);
            }

            printf("read: up to %d bytes\n", EMU_MEM1_SIZE);
            size_t nread; 
            if( (nread = fread(pM->mem.stMem.mem1, 1, EMU_MEM1_SIZE, fp)) < 1 ){
                fprintf(stderr, " Read error \"%s\" \n", filename);
                exit(1);
            }
            printf("File \"%s\" is loading (%d bytes)\n", filename, (unsigned int)nread);
            fclose(fp);
#endif
        }
    }

    if( image_specified == 0 ){
        fprintf(stderr, "no image file is specified ...\n\n");
        printUsage();
        exit(1);
    }


    //-----------------------------------------------------
    // Register a signal handler for handling Ctrl+C
    //-----------------------------------------------------
    if ( signal(SIGINT, sig_handler_SIGINT) == SIG_ERR ) {
        logfile_printf(LOGCAT_EMU | LOGLV_ERROR, "failed to register a signal hander.\n");
        logfile_close();
        shutdownEmulator(pM);
    }
    //-----------------------------------------------------


    return 0;
}



void shutdownEmulator( struct stMachineState *pM ){
    restoreTerminalSetting(pM);
    termResetSettingForExit();

    exit(1);
}

void initTerminalSetting(struct stMachineState *pM){
    struct termios settings;

    settings = termOrigSettings;
    settings.c_lflag &= ~(ECHO|ICANON);  /* without input echo, and unbuffered */
    settings.c_cc[VTIME] = 0;
    settings.c_cc[VMIN] = 1;
    tcsetattr(0,TCSANOW,&settings);
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
}

void saveTerminalSetting(struct stMachineState *pM){
    tcgetattr(0, &termOrigSettings);
}

void restoreTerminalSetting(struct stMachineState *pM){
    tcsetattr(0, TCSANOW, &termOrigSettings);
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) & (~O_NONBLOCK));
}
