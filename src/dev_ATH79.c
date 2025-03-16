#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>

#include "mips.h"
#include "dev_UART.h"
#include "dev_ATH79.h"
#include "logfile.h"
#include "ExInst_common.h"

#define LOGLEVEL_EMU_INFO   (LOGCAT_EMU | LOGLV_INFO)
#define LOGLEVEL_EMU_INFO3  (LOGCAT_EMU | LOGLV_INFO3)
#define LOGLEVEL_EMU_NOTICE (LOGCAT_EMU | LOGLV_NOTICE)
#define LOGLEVEL_EMU_ERROR  (LOGCAT_EMU | LOGLV_ERROR)

uint32_t readMiscIntStatusReg(struct stMachineState *pM){
    int uart_int=0;

    if( pM->mem.ioUART0.intEnable ){
        if( pM->mem.ioUART0.intIdent != UART_REG_INTID_NO_INT ){
            uart_int = 1;
        }
    }

    return (uart_int ? (1<<3) : 0);
}

