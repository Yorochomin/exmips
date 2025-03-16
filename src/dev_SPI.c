#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>

#include "mips.h"
#include "dev_SPI.h"
#include "dev_SPIFlash.h"
#include "logfile.h"
#include "ExInst_common.h"


int dummy_init(struct stMachineState *pM, int cs){ 
    return SPI_WORKER_RESULT_OK;
}
int dummy_remote(struct stMachineState *pM, int cs){ 
    return SPI_WORKER_RESULT_OK;
}
int dummy_select(struct stMachineState *pM, int cs){ 
    PRINTF("SPI select %d\n", cs);
    return SPI_WORKER_RESULT_OK;
}
int dummy_deselect(struct stMachineState *pM, int cs){
    PRINTF("SPI deselect %d\n", cs);
    return SPI_WORKER_RESULT_OK;
}
uint8_t dummy_write(struct stMachineState *pM, int cs, uint8_t val){
    PRINTF("SPI write %d 0x%x\n", cs, val);
    return 0xff;
}

struct stSPIWorker workers[]={
    {
        .init     = initFlash,
        .remove   = removeFlash,
        .select   = selectFlash,
        .deselect = deselectFlash,
        .write    = writeFlash
    },{
        .init     = dummy_init,
        .remove   = dummy_remote,
        .select   = dummy_select,
        .deselect = dummy_deselect,
        .write    = dummy_write
    },{
        .init     = dummy_init,
        .remove   = dummy_remote,
        .select   = dummy_select,
        .deselect = dummy_deselect,
        .write    = dummy_write
    }
};


int initSPI(struct stMachineState *pM, struct stIO_SPI *pSPI){
    for(int i=0; i<sizeof(workers)/sizeof(workers[0]); i++){
        if( SPI_WORKER_RESULT_OK != workers[i].init(pM, i) ){
            return SPI_WORKER_RESULT_FAIL;
        }
    }
    return SPI_WORKER_RESULT_OK;
}

int removeSPI(struct stMachineState *pM, struct stIO_SPI *pSPI){
    for(int i=0; i<sizeof(workers)/sizeof(workers[0]); i++){
        if( SPI_WORKER_RESULT_OK != workers[i].remove(pM, i) ){
            return SPI_WORKER_RESULT_FAIL;
        }
    }
    return SPI_WORKER_RESULT_OK;
}

uint32_t readSPIReg(struct stMachineState *pM, struct stIO_SPI *pSPI, uint32_t addr){

    switch( (addr&SPI_ADDR_MASK) & (~0x03) ){
        case SPI_FUNC_SEL_REG:
            return pSPI->functionSelect;
        case SPI_CTRL_REG:
            return pSPI->control;
        case SPI_IO_CTRL_REG:
            return pSPI->ioControl;
        case SPI_READ_DATA_REG:
            return pSPI->readDataAddr;
        case SPI_SHIFT_DATAOUT_REG:
            return pSPI->shiftDataOut;
        case SPI_SHIFT_CNT_REG:
            return pSPI->shiftCount;
        case SPI_SHIFT_DATAIN_REG:
            return pSPI->shiftDataIn;
        default:
            PRINTF("unknown SPI address was read (addr 0x%x)\n", addr);
            break;
    }

    return 0;
}

void writeSPIReg(struct stMachineState *pM, struct stIO_SPI *pSPI, uint32_t addr, uint32_t data){

    uint32_t chnlMask[] = {
        (1<<SPI_SHIFT_CNT_BIT_SHIFT_CHNL_CS0), 
        (1<<SPI_SHIFT_CNT_BIT_SHIFT_CHNL_CS1), 
        (1<<SPI_SHIFT_CNT_BIT_SHIFT_CHNL_CS2) 
    };

    switch( (addr&SPI_ADDR_MASK) & (~0x03) ){
        case SPI_FUNC_SEL_REG:
            pSPI->functionSelect = data;
            break;
        case SPI_CTRL_REG:
            pSPI->control = data;
            break;
        case SPI_IO_CTRL_REG:
            pSPI->ioControl = data;
            break;
        case SPI_READ_DATA_REG:
            pSPI->readDataAddr = data;
            break;
        case SPI_SHIFT_DATAOUT_REG:
            pSPI->shiftDataOut = data;
            break;
        case SPI_SHIFT_CNT_REG:
            /*
             * Changing chip-select bits without SHIFT_EN bit and shift-count seems to change no chip select outputs.
             */
            for(int i=0; i<sizeof(chnlMask)/sizeof(chnlMask[0]) && i < sizeof(workers)/sizeof(workers[0]); i++){
                if( ((data ^ pSPI->shiftCount) & chnlMask[i]) && (data & (1<<SPI_SHIFT_CNT_BIT_SHIFT_EN)) && ((data & SPI_SHIFT_CNT_SHIFT_COUNT_MASK) > 0) ){
                    if( data & chnlMask[i] ){
                        workers[i].select(pM, i);
                    }else{
                        workers[i].deselect(pM, i);
                    }
                }
            }

            if( data & (1<<SPI_SHIFT_CNT_BIT_SHIFT_EN) ){

                // Checking for each chip select
                for(int i=0; i<sizeof(chnlMask)/sizeof(chnlMask[0]) && i<sizeof(workers)/sizeof(workers[0]); i++){

                    // tx is carried out for enabled chip selects
                    if( data & chnlMask[i] ){
                        if( (data & SPI_SHIFT_CNT_SHIFT_COUNT_MASK) > 0 ){
                            int cnt = (data & SPI_SHIFT_CNT_SHIFT_COUNT_MASK)/8;
                            uint32_t outdata = pSPI->shiftDataOut;
                            uint32_t indata  = 0;
                            for(int j=cnt-1; j>=0; j--){
                                indata = ((indata << 8) | workers[i].write(pM, i, (outdata>>(8*j))&0xff));
                            }
                            pSPI->shiftDataIn = indata;
                        }
                        if( data & (1<<SPI_SHIFT_CNT_BIT_TERMINATE) ){
                            workers[i].deselect(pM, i);
                            data &= ~chnlMask[i];
                        }
                    }
                }
                data &= ~(1<<SPI_SHIFT_CNT_BIT_SHIFT_EN);
            }

            pSPI->shiftCount = data;
            break;
        case SPI_SHIFT_DATAIN_REG:
            pSPI->shiftDataIn = data;
            break;
        default:
            PRINTF("unknown SPI address was written (addr 0x%x val 0x%x)\n", addr, data);
            break;
    }
}

