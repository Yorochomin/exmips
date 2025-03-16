#include <stdio.h>
#include <stdint.h>
#include <signal.h>

#include "mips.h"
//#include "instmap32.h"
#include "ExInst_common.h"

#include "misc.h"
#include "dev_UART.h"
#include "dev_ATH79.h"
#include "logfile.h"
#include "mem.h"
#include "cp0.h"
#include "tlb.h"
#include "exception.h"
#include "env_unix.h"
#include <ctype.h>
#include "terminal.h"
#include <string.h>

static void emuMonitor_help(void){
    PRINTF(" d [addr] :  dump memory. \n");
    PRINTF(" perf     :  show performance values. \n");
    PRINTF(" reg      :  show register values. \n");
    PRINTF(" halt     :  halt the emulator and exit. \n");
    PRINTF(" exit     :  exit this monitor and return to emulation. \n");
    PRINTF(" help     :  show this message. \n\n");
}

static int emuMonitor_splitCmd(char *cmd, int *pargc, char **argv, int maxArg){
    int i, argc = 0;
    char *p = cmd;

    for(i = 0; i < maxArg; i++){
        while (*p == ' ' || *p == '\t' || *p == '\b' || *p == '\r' || *p == '\n')
            p++;
        argv[argc] = p;
        while (!(*p == ' ' || *p == '\t' || *p == '\b' || *p == '\r' || *p == '\n' || *p == '\0'))
            p++;
        if( *p == '\0' ){
            if( argv[argc] != p ){
                argc++;
            }
            break;
        }else{
            *p++ = '\0';
            argc++;
        }
    }
    *pargc = argc;

    return argc;
}

static int emuMonitor_execCmd(struct stMachineState *pM, char *cmd){
    char *argv[20];
    int argc;
    //    static int start_address = 0;

    emuMonitor_splitCmd(cmd, &argc, argv, sizeof(argv) / sizeof(char *));

    if( (!strcmp(argv[0], "d") && argc == 2) || (*argv[0] == 'd' && argc == 1) ){
        static unsigned int addr = 0;
        char memstr[17];
        unsigned char membyte;
        unsigned int i;

        memstr[16] = '\0';

        if( argc == 1 ){
            if( *(argv[0] + 1) != '\0' ){
                sscanf(argv[0] + 1, "%x", &addr);
            }
        }else{
            sscanf(argv[1], "%x", &addr);
        }
        for(i = addr; i <= ((addr + 0x7f) | 0xf) && !(i==0 && addr>0); i++){
            if( (i & 0xf) == 0 ){
                PRINTF("%08x : ", i);
            }
            if( i < addr ){
                PRINTF("   ");
                memstr[i & 0xf] = ' ';
            }else{
                int accessError;
                membyte = loadByte(pM, i, &accessError);
                if( accessError ){
                    PRINTF("?? ");
                    membyte = 0; // non-printable
                }else{
                    PRINTF("%02x ", membyte);
                }
                memstr[i & 0xf] = isprint(membyte) ? membyte : '.';
            }
            if( (i & 0xf) == 0xf ){
                PRINTF(": %s \r\n", memstr);
            }
        }
        addr += 0x80;
    }else if( !strcmp(argv[0], "perf") ){
        PRINTF("Clock frequency: %9ld Hz\n", (long)FREQ_CPU);
        PRINTF("Execution rate : %9ld inst./sec\n", pM->emu.execRate);
    }else if( !strcmp(argv[0], "tlb") ){
        PRINTF("Index ASID    VPN2    PageMask  G     Entry0      Entry1\n");
        for(int i=0; i < NUM_TLB_ENTRY; i++){
            PRINTF("  %3x   %2x  %08x  %08x  %c  %08x%c%c  %08x%c%c\n", 
            i, pM->reg.tlb[i].field_asid, pM->reg.tlb[i].field_vpn2, pM->reg.tlb[i].field_pmask, pM->reg.tlb[i].field_g ? 'g':'-',
            pM->reg.tlb[i].lo[0].field_pfn, pM->reg.tlb[i].lo[0].field_valid ? 'V' : '-', pM->reg.tlb[i].lo[0].field_dirty ? 'D' : '-',
            pM->reg.tlb[i].lo[1].field_pfn, pM->reg.tlb[i].lo[1].field_valid ? 'V' : '-', pM->reg.tlb[i].lo[1].field_dirty ? 'D' : '-'
            );
        }
    }else if( !strcmp(argv[0], "reg") ){
        PRINTF("PC = %08x C0_STATUS = %x\n", pM->reg.pc,  pM->reg.cp0[ C0_REG_IDX(C0_STATUS) ]);
        PRINTF("r[ 0.. 7]=%08x %08x %08x %08x %08x %08x %08x %08x\n", pM->reg.r[ 0], pM->reg.r[ 1], pM->reg.r[ 2], pM->reg.r[ 3], pM->reg.r[ 4], pM->reg.r[ 5], pM->reg.r[ 6], pM->reg.r[ 7]);
        PRINTF("r[ 8..15]=%08x %08x %08x %08x %08x %08x %08x %08x\n", pM->reg.r[ 8], pM->reg.r[ 9], pM->reg.r[10], pM->reg.r[11], pM->reg.r[12], pM->reg.r[13], pM->reg.r[14], pM->reg.r[15]);
        PRINTF("r[16..23]=%08x %08x %08x %08x %08x %08x %08x %08x\n", pM->reg.r[16], pM->reg.r[17], pM->reg.r[18], pM->reg.r[19], pM->reg.r[20], pM->reg.r[21], pM->reg.r[22], pM->reg.r[23]);
        PRINTF("r[24..31]=%08x %08x %08x %08x %08x %08x %08x %08x\n", pM->reg.r[24], pM->reg.r[25], pM->reg.r[26], pM->reg.r[27], pM->reg.r[28], pM->reg.r[29], pM->reg.r[30], pM->reg.r[31]);
    }else if( !strcmp(argv[0], "exit") ){
        return 0;
    }else if( !strcmp(argv[0], "halt") ){
        return -1;
    }else if( !strcmp(argv[0], "help") ){
        emuMonitor_help();
    }else{
        char *p;
        for(p = argv[0]; *p != '\0'; p++){
            if (*p != '\n' && *p != '\r' && *p != '\t' && *p != ' ')
                break;
        }if( *p != '\0' ){
            PRINTF("Syntax error\r\n");
            emuMonitor_help();
        }
    }

    return 1;
}

extern struct termios term_original_settings;

/**
 * Emulator monitor for 16bit environment
 *
 * Return value is 0 or a negative value.
 * 0 requests the return to the emulation.
 * A negative value requests halting the emulator.
 */
int emuMonitor(struct stMachineState *pM){
    int result;
    char cmd[128];

    restoreTerminalSetting(pM);
    termResetSettingForExit();

    termSetCharColor(charColorBrightRed);
    PRINTF("\nEmulator monitor\n\n");
    PRINTF("Use \"halt\" command to halt the emulator\n");
    PRINTF("Use \"exit\" command to return the emulator\n");
    termSetCharColor(charColorReset);

    do{
        termSetCharColor(charColorBrightCyan);
        PRINTF("> ");
        termSetCharColor(charColorReset);
        FLUSH_STDOUT();

        if( NULL == fgets(cmd, sizeof(cmd), stdin) ){
            result = 0; break;
        }
        result = emuMonitor_execCmd(pM, cmd);
    }while( result > 0 );

    termResetSettingForExit();
//    termClear();
    initTerminalSetting(pM);

    return result;
}