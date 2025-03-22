#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>

#include "mips.h"
#include "dev_UART.h"
#include "logfile.h"
#include "ExInst_common.h"


#define ASCII_LF  0x0a
#define ASCII_CR  0x0d
#define ASCII_DEL 0x7f
#define ASCII_BS  0x08

#define LOGLEVEL_UART_INFO   (LOGCAT_IO_UART | LOGLV_INFO)
#define LOGLEVEL_UART_NOTICE (LOGCAT_IO_UART | LOGLV_NOTICE)


void requestUARTSendBreak(struct stMachineState *pM, struct stIO_UART *pUART){
    pUART->breakRequest = 1;
}

uint8_t readUARTReg(struct stMachineState *pM, struct stIO_UART *pUART, uint32_t addr){

    switch( (addr&IOADDR_UART_MASK) ){

        case UART_REG_RXBUF:
            if( pUART->lineControl & (1<<UART_REG_LINECTL_BIT_DLAB) ){
                return pUART->divisor[0];
            }else{
                if( pUART->buffered ){
                    pUART->buffered = 0;
                }

                if(pUART->buf[0] == ASCII_LF ) pUART->buf[0] = ASCII_CR;
                if(pUART->buf[0] == ASCII_DEL) pUART->buf[0] = ASCII_BS;

                return pUART->buf[0];
            }
            break;

        case UART_REG_INTEN: // interrupt enable register
            if( pUART->lineControl & (1<<UART_REG_LINECTL_BIT_DLAB) ){
                return pUART->divisor[1];
            }else{
                return pUART->intEnable;
            }
            break;

        case UART_REG_INTID: // interrupt ident. register
        {
            int prevID = pUART->intIdent;
            pUART->intIdent = UART_REG_INTID_NO_INT;
            return prevID;
        }
        case UART_REG_LINECTL: // line control register
            return pUART->lineControl;

        case UART_REG_MODEMCTL: // modem control register
            return pUART->modemControl;

        case UART_REG_LINESTAT: // line status register
            // status reg
            // bit0: receive buffer empty if this bit is 0
            // bit1: transmitter idle if this bit is 0

            if( ! pUART->buffered){
                if( pUART->breakRequest ){
                    // Send a Ctrl+C
                    pUART->buffered = 1;
                    pUART->buf[0]   = 3;
                    pUART->breakRequest = 0; // Clear the request
                }else{
                    pUART->buffered = read(0, pUART->buf, 1);
                    if( pUART->buffered < 0){
                        pUART->buffered = 0;
                    }
                }
            }

            if( pUART->buffered ){
                if( pUART->intEnable & (1<<UART_REG_INTEN_BIT_RX_DATA_AVAIL) ){
                    pUART->intIdent = UART_REG_INTID_RX_DATA_AVAIL;
                }
                return (UART_TX_BUF_EMPTY|UART_TX_EMPTY|UART_RX_BUF_FULL);
            }else{
                return (UART_TX_BUF_EMPTY|UART_TX_EMPTY);
            }
            break;

        case UART_REG_MODEMSTAT: // modem status register
            return ((1<<7)|(1<<4)|(1<<5)); // CTS (Clear To Send) and DSR (Data Set Ready) bits are set

        case UART_REG_SCRATCH: // cratch register
            logfile_printf(LOGLEVEL_UART_INFO, "UART: scratch register was read\n");
            return pUART->scratch;

        default:
            break;
    }

    logfile_printf(LOGLEVEL_UART_NOTICE, "UART: unknown register was read (addr: %x)\n", addr);

    return 0;
}

void writeUARTReg  (struct stMachineState *pM, struct stIO_UART *pUART, uint32_t addr, uint8_t data){

    switch( (addr&IOADDR_UART_MASK) ){
        
        case UART_REG_TXBUF:
            if( pUART->lineControl & (1<<UART_REG_LINECTL_BIT_DLAB) ){
                pUART->divisor[0] = data;
            }else{
                if(data == '\n'){
                    PRINTF("\033[39m");
                    PRINTF("\033[49m");
                }
                PRINTF("%c", data);
                FLUSH_STDOUT();
                if( pUART->intEnable & (1<<UART_REG_INTEN_BIT_TX_DATA_EMPTY) ){
                    if( pUART->intIdent == UART_REG_INTID_NO_INT ){
                        pUART->intIdent = UART_REG_INTID_TX_DATA_EMPTY;
                    }
                }
            }
            break;

        case UART_REG_INTEN: // interrupt enable register
            if( pUART->lineControl & (1<<UART_REG_LINECTL_BIT_DLAB) ){
                pUART->divisor[1] = data;
            }else{
                pUART->intEnable = data;
                readUARTReg(pM, pUART, UART_REG_LINESTAT); // to update internal state
            }
            break;

        case UART_REG_INTID: // interrupt ident. register
            /* read only */
            break;

        case UART_REG_LINECTL: // line control register
            pUART->lineControl = data;
            break;

        case UART_REG_MODEMCTL: // modem control register
            pUART->modemControl = data;
            break;

        case UART_REG_LINESTAT: // line status register
            break;

        case UART_REG_MODEMSTAT: // modem status register
            break;

        case UART_REG_SCRATCH: // cratch register
            pUART->scratch = data;
            break;

        default:
            logfile_printf(LOGLEVEL_UART_NOTICE, "UART: write for unknown register (addr: %x, data %x)\n", addr, data);
            break;
    }
}
