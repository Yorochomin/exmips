#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mips.h"
#include "mem.h"
#include "dev_SPIFlash.h"
#include "dev_SPI.h"
#include "logfile.h"
#include "ExInst_common.h"


#define FLASH_BUF_SIZE 32
#define SPI_FLASH_NUM 1


#define SPI_FLASH_SR1_BIT_SRP0  7  /* Status Register Protect 0 */
#define SPI_FLASH_SR1_BIT_SEC   6  /* Sector/Block protect (0: 64kB blocks, 1: 4kB sectors) */
#define SPI_FLASH_SR1_BIT_TB    5  /* Top/Bottom protect (0: top down, 1: bottom up) */
#define SPI_FLASH_SR1_BIT_BP2   4  /* Block protect bits */
#define SPI_FLASH_SR1_BIT_BP1   3  /* Block protect bits */
#define SPI_FLASH_SR1_BIT_BP0   2  /* Block protect bits */
#define SPI_FLASH_SR1_BIT_BP    2  /* Block protect bits */
#define SPI_FLASH_SR1_BP_MASK   (7<<SPI_FLASH_SR1_BIT_BP)  /* Block protect mask */
#define SPI_FLASH_SR1_BIT_WE    1  /* Write enable latch */
#define SPI_FLASH_SR1_BIT_BUSY  0  /* Embedded operation status */

#define SPI_FLASH_SR2_BIT_SUS   7  /* Suspend status (1: Erase / Program suspended) */
#define SPI_FLASH_SR2_BIT_CMP   6  /* Complement protect (0: normal protection map, 1: inverted protection map) */
#define SPI_FLASH_SR2_BIT_LB3   5  /* Security register lock bits */
#define SPI_FLASH_SR2_BIT_LB2   4  /* Security register lock bits */
#define SPI_FLASH_SR2_BIT_LB1   3  /* Security register lock bits */
#define SPI_FLASH_SR2_BIT_LB0   2  /* Security register lock bits */
#define SPI_FLASH_SR2_BIT_LB    2  /* Security register lock bits */
#define SPI_FLASH_SR2_LB_MASK   (0xf<<SPI_FLASH_SR2_BIT_LB)  /* Security register lock-bit mask */
#define SPI_FLASH_SR2_BIT_QE    1  /* Quad enable */
#define SPI_FLASH_SR2_BIT_SRP1  0  /* Status register protect 1 */

#define SPI_FLASH_SR3_BIT_W6     6  /* Burst wrap length */
#define SPI_FLASH_SR3_BIT_W5     5  /* Burst wrap length */
#define SPI_FLASH_SR3_BIT_BWLEN  5  /* Burst wrap length */
#define SPI_FLASH_SR3_BWLEN_MASK (3<<SPI_FLASH_SR3_BIT_BWLEN)  /* Burst wrap length mask */
#define SPI_FLASH_SR3_BIT_BWE    4  /* Burst wrap enable */
#define SPI_FLASH_SR3_BIT_LC3    3  /* Latency control bits */
#define SPI_FLASH_SR3_BIT_LC2    2  /* Latency control bits */
#define SPI_FLASH_SR3_BIT_LC1    1  /* Latency control bits */
#define SPI_FLASH_SR3_BIT_LC0    0  /* Latency control bits */
#define SPI_FLASH_SR3_BIT_LC     0  /* Latency control bits */
#define SPI_FLASH_SR3_LC_MASK    (0xf<<SPI_FLASH_SR3_BIT_LC)  /* Latency control mask */

#define SPI_FLASH_BURST_WRAP_8B   (0<<SPI_FLASH_SR3_BIT_BWLEN)
#define SPI_FLASH_BURST_WRAP_16B  (1<<SPI_FLASH_SR3_BIT_BWLEN)
#define SPI_FLASH_BURST_WRAP_32B  (2<<SPI_FLASH_SR3_BIT_BWLEN)
#define SPI_FLASH_BURST_WRAP_64B  (3<<SPI_FLASH_SR3_BIT_BWLEN)

#define FLASH_CMD_READ4B          0x13
#define FLASH_CMD_FAST_READ4B     0x0C
#define FLASH_CMD_BLOCK_ERASE4B   0xDC
#define FLASH_CMD_SECTOR_ERASE4B  0x21
#define FLASH_CMD_PAGE_PROGRAM4B  0x12


struct stIO_SPIFlash{
    uint8_t id;
    uint8_t selected;
    uint8_t error;
    uint8_t sr[3];
    uint8_t flash_buf[FLASH_BUF_SIZE];
    uint8_t *mem;
    char    *filename;
    uint32_t flash_cnt;
    uint32_t addr;
    const struct stSPIFlashParam *param;
};

struct stIO_SPIFlash stSPIFlash[SPI_FLASH_NUM];


static int _initFlash(struct stMachineState *pM, struct stIO_SPIFlash *pFlash){
    FILE *fp = NULL;
    const struct stSPIFlashParam *param = pM->emu.spiflash_param;

    pFlash->param = param;
    pFlash->mem = malloc(param->capacity);
    if( pFlash->mem == NULL ){
        fprintf(stderr, "Memory allocation for a SPI flash memoru was failed.\n");
        exit(1);
    }
    memset(pFlash->mem, 0xff, param->capacity);

    if( pFlash->filename == NULL ){
        return SPI_WORKER_RESULT_OK;
    }

    printf("Opening SPI flash image file \"%s\"\n", pFlash->filename);

    fp = fopen(pFlash->filename, "rb");
    if( fp == NULL ){
        fprintf(stderr, "Failed to open \"%s\" \n", pFlash->filename);
        return SPI_WORKER_RESULT_FAIL;
    }

    size_t nread; 
    if( (nread = fread(pFlash->mem, 1, param->capacity, fp)) < 1 ){
        fprintf(stderr, "Read error \"%s\" \n", pFlash->filename);
        return SPI_WORKER_RESULT_FAIL;
    }
    printf("Stored %d bytes\n", (int)nread);

    pFlash->selected = 0;

    return SPI_WORKER_RESULT_OK;
}

static int _removeFlash(struct stMachineState *pM, struct stIO_SPIFlash *pFlash){ 
    FILE *fp = NULL;

    if( pFlash->mem      == NULL ){
        return SPI_WORKER_RESULT_OK;
    }


    if( pFlash->filename == NULL ){
        goto removeFlashEnd;
    }

    if( pM->emu.bWriteBackFlashData ){
        printf("SPI flash image is write-backed to \"%s\"... (%d bytes)\n", pFlash->filename, pFlash->param->capacity);

        fp = fopen(pFlash->filename, "wb");
        if( fp == NULL ){
            fprintf(stderr, " Failed to open \"%s\" \n", pFlash->filename);
            goto removeFlashEnd;
        }

        size_t nwrite; 
        if( (nwrite = fwrite(pFlash->mem, 1, pFlash->param->capacity, fp)) < 1 ){
            fprintf(stderr, " Write-back error \"%ld\" \n", nwrite);
            goto removeFlashEnd;
        }
    }

removeFlashEnd:
    free( pFlash->mem );
    pFlash->mem = NULL;
    pFlash->selected = 0;

    return SPI_WORKER_RESULT_OK;
}

static int _selectFlash(struct stMachineState *pM, struct stIO_SPIFlash *pFlash){ 

    if( ! pFlash->selected ){
        pFlash->flash_cnt = 0;
        pFlash->flash_buf[0] = 0;
        pFlash->error = 0;
        pFlash->selected = 1;
    }

    return SPI_WORKER_RESULT_OK;
}

static int _deselectFlash(struct stMachineState *pM, struct stIO_SPIFlash *pFlash){
    if( pFlash->error ){
        PRINTF("FLASH deselect %d (len %d, cmd 0x%x, addr 0x%x)\n", (uint32_t)pFlash->id, (uint32_t)pFlash->flash_cnt, (uint32_t)pFlash->flash_buf[0], pFlash->addr);
    }
    pFlash->flash_cnt = 0;
    pFlash->flash_buf[0] = 0;
    pFlash->error = 0;
    pFlash->selected = 0;

    return SPI_WORKER_RESULT_OK;
}

static uint8_t _writeFlash(struct stMachineState *pM, struct stIO_SPIFlash *pFlash, uint8_t val){
    uint8_t result = 0xff;
    unsigned int count = pFlash->flash_cnt;

    /* JEDEC ID */
    const uint8_t spiflash_jdec_id[] = {pFlash->param->manufacturer_id, pFlash->param->devicetype_id, pFlash->param->capacity_id};

    if( count < FLASH_BUF_SIZE ){
        pFlash->flash_buf[ count ] = val;
    }

    switch( pFlash->flash_buf[0] ){
        case FLASH_CMD_ID:
            switch( count ){
                case 0: goto writeFlash_exit;
                case 1: 
                case 2: 
                case 3: result = spiflash_jdec_id[count-1]; goto writeFlash_exit;
            }
            break;
        case FLASH_CMD_READ:
            switch( count ){
                case 0: goto writeFlash_exit;
                case 1: pFlash->addr = (val<<16); goto writeFlash_exit;
                case 2: pFlash->addr|= (val<< 8); goto writeFlash_exit;
                case 3: pFlash->addr|=  val; PREFETCH(&(pFlash->mem[ pFlash->addr & (pFlash->param->capacity-1) ]), 0, 0); goto writeFlash_exit;
                default:
                    result = pFlash->mem[ pFlash->addr & (pFlash->param->capacity-1) ];
                    pFlash->addr ++;
            }
            break;
        case FLASH_CMD_FAST_READ:
        case FLASH_CMD_FAST_READ_DUAL:
        case FLASH_CMD_FAST_READ_QUAD:
            switch( count ){
                case 0: goto writeFlash_exit;
                case 1: pFlash->addr = (val<<16); goto writeFlash_exit;
                case 2: pFlash->addr|= (val<< 8); goto writeFlash_exit;
                case 3: pFlash->addr|=  val; PREFETCH(&(pFlash->mem[ pFlash->addr & (pFlash->param->capacity-1) ]), 0, 0); goto writeFlash_exit;
                case 4: goto writeFlash_exit;
                default:
                    result = pFlash->mem[ pFlash->addr & (pFlash->param->capacity-1) ];
                    pFlash->addr ++;
            }
            break;
        case FLASH_CMD_READ4B:
            switch( count ){
                case 0: goto writeFlash_exit;
                case 1: pFlash->addr = (val<<24); goto writeFlash_exit;
                case 2: pFlash->addr|= (val<<16); goto writeFlash_exit;
                case 3: pFlash->addr|= (val<< 8); goto writeFlash_exit;
                case 4: pFlash->addr|=  val; PREFETCH(&(pFlash->mem[ pFlash->addr & (pFlash->param->capacity-1)]), 0, 0); goto writeFlash_exit;
                default:
                    result = pFlash->mem[ pFlash->addr & (pFlash->param->capacity-1) ];
                    pFlash->addr ++;
            }
            break;
        case FLASH_CMD_FAST_READ4B:
            switch( count ){
                case 0: goto writeFlash_exit;
                case 1: pFlash->addr = (val<<24); goto writeFlash_exit;
                case 2: pFlash->addr|= (val<<16); goto writeFlash_exit;
                case 3: pFlash->addr|= (val<< 8); goto writeFlash_exit;
                case 4: pFlash->addr|=  val; PREFETCH(&(pFlash->mem[ pFlash->addr & (pFlash->param->capacity-1) ]), 0, 0); goto writeFlash_exit;
                case 5: goto writeFlash_exit;
                default:
                    result = pFlash->mem[ pFlash->addr & (pFlash->param->capacity-1) ];
                    pFlash->addr ++;
            }
            break;
        case FLASH_CMD_READ_SFDP:
            switch( count ){
                case 0: goto writeFlash_exit;
                case 1: goto writeFlash_exit;
                case 2: goto writeFlash_exit;
                case 3: pFlash->addr = val; goto writeFlash_exit;
                case 4: goto writeFlash_exit;
                default:
                    result = 0xff;
                    if( pFlash->addr < pFlash->param->sfdp_size ){
                        result = pFlash->param->sfdp[ pFlash->addr ];
                    }
                    pFlash->addr ++;
                    pFlash->error = 1;
            }
            break;
        case FLASH_CMD_BLOCK_ERASE:
            switch( count ){
                case 0: goto writeFlash_exit;
                case 1: pFlash->addr = (val<<16); goto writeFlash_exit;
                case 2: pFlash->addr|= (val<< 8); goto writeFlash_exit;
                case 3: pFlash->addr|=  val;
                    result = 0xff;
                    if( pFlash->sr[0] & (1<<SPI_FLASH_SR1_BIT_WE) ){
                        pFlash->addr &= (~(pFlash->param->block_size-1));
                        for(int i = 0; i < pFlash->param->block_size; i++){
                            pFlash->mem[ (pFlash->addr + i) & (pFlash->param->capacity - 1) ] = 0xff;
                        }
                    }
                default:
                    break;
            }
            break;
        case FLASH_CMD_SECTOR_ERASE:
            switch( count ){
                case 0: goto writeFlash_exit;
                case 1: pFlash->addr = (val<<16); goto writeFlash_exit;
                case 2: pFlash->addr|= (val<< 8); goto writeFlash_exit;
                case 3: pFlash->addr|=  val;
                    result = 0xff;
                    if( pFlash->sr[0] & (1<<SPI_FLASH_SR1_BIT_WE) ){
                        pFlash->addr &= (~(pFlash->param->sector_size-1));
                        for(int i = 0; i < pFlash->param->sector_size; i++){
                            pFlash->mem[ (pFlash->addr + i) & (pFlash->param->capacity - 1) ] = 0xff;
                        }
                    }
                default:
                    break;
            }
            break;
        case FLASH_CMD_PAGE_PROGRAM:
            switch( count ){
                case 0: goto writeFlash_exit;
                case 1: pFlash->addr = (val<<16); goto writeFlash_exit;
                case 2: pFlash->addr|= (val<< 8); goto writeFlash_exit;
                case 3: pFlash->addr|=  val;      goto writeFlash_exit;
                default:
                    result = 0xff;
                    if( pFlash->sr[0] & (1<<SPI_FLASH_SR1_BIT_WE) ){
                        pFlash->mem[ (pFlash->addr + count-4) & (pFlash->param->capacity - 1) ] = val;
                    }
                    break;
            }
            break;
        case FLASH_CMD_BLOCK_ERASE4B:
            switch( count ){
                case 0: goto writeFlash_exit;
                case 1: pFlash->addr = (val<<24); goto writeFlash_exit;
                case 2: pFlash->addr|= (val<<16); goto writeFlash_exit;
                case 3: pFlash->addr|= (val<< 8); goto writeFlash_exit;
                case 4: pFlash->addr|=  val;
                    result = 0xff;
                    if( pFlash->sr[0] & (1<<SPI_FLASH_SR1_BIT_WE) ){
                        pFlash->addr &= (~(pFlash->param->block_size-1));
                        for(int i = 0; i < pFlash->param->block_size; i++){
                            pFlash->mem[ (pFlash->addr + i) & (pFlash->param->capacity - 1) ] = 0xff;
                        }
                    }
                default:
                    break;
            }
            break;
        case FLASH_CMD_SECTOR_ERASE4B:
            switch( count ){
                case 0: goto writeFlash_exit;
                case 1: pFlash->addr = (val<<24); goto writeFlash_exit;
                case 2: pFlash->addr|= (val<<16); goto writeFlash_exit;
                case 3: pFlash->addr|= (val<< 8); goto writeFlash_exit;
                case 4: pFlash->addr|=  val;
                    result = 0xff;
                    if( pFlash->sr[0] & (1<<SPI_FLASH_SR1_BIT_WE) ){
                        pFlash->addr &= (~(pFlash->param->sector_size-1));
                        for(int i = 0; i < pFlash->param->sector_size; i++){
                            pFlash->mem[ (pFlash->addr + i) & (pFlash->param->capacity - 1) ] = 0xff;
                        }
                    }
                default:
                    break;
            }
            break;
        case FLASH_CMD_PAGE_PROGRAM4B:
            switch( count ){
                case 0: goto writeFlash_exit;
                case 1: pFlash->addr = (val<<24); goto writeFlash_exit;
                case 2: pFlash->addr|= (val<<16); goto writeFlash_exit;
                case 3: pFlash->addr|= (val<< 8); goto writeFlash_exit;
                case 4: pFlash->addr|=  val;      goto writeFlash_exit;
                default:
                    result = 0xff;
                    if( pFlash->sr[0] & (1<<SPI_FLASH_SR1_BIT_WE) ){
                        pFlash->mem[ pFlash->addr & (pFlash->param->capacity-1) ] = val;
                    }
                    pFlash->addr++;
                    break;
            }
            break;
        case FLASH_CMD_READ_SR1:
            switch( count ){
                case 0: goto writeFlash_exit;
                case 1: 
                    result = (pFlash->sr[0] & (~(1<<SPI_FLASH_SR1_BIT_BUSY)));
                default:
                    break;
            }
            break;
        case FLASH_CMD_WRITE_ENABLE:
            pFlash->sr[0] |=  (1<< SPI_FLASH_SR1_BIT_WE);
            break;
        case FLASH_CMD_WRITE_DISABLE:
            pFlash->sr[0] &= ~(1<< SPI_FLASH_SR1_BIT_WE);
            break;
        default:
            if(!pFlash->error){ printf("SPI command 0x%x is issued. But it cannot be handled\n", pFlash->flash_buf[0]); }
            pFlash->error = 1;
            break;
    }

writeFlash_exit:

//    PRINTF("FLASH SPI write CS=%d (0x%x -> 0x%x)\n", pFlash->id, val, result);
    pFlash->flash_cnt++;
    return result;
}


int setFlashImageName(struct stMachineState *pM, int cs, char *filename){
    if(cs < SPI_FLASH_NUM){
        stSPIFlash[cs].filename = filename;
        return SPI_WORKER_RESULT_OK;
    }else{
        return SPI_WORKER_RESULT_FAIL;
    }    
}


int initFlash(struct stMachineState *pM, int cs){ 
    if(cs < SPI_FLASH_NUM){
        stSPIFlash[cs].id = cs;
        return _initFlash(pM, &stSPIFlash[cs]);
    }else{
        return SPI_WORKER_RESULT_FAIL;
    }
}
int removeFlash(struct stMachineState *pM, int cs){ 
    if(cs < SPI_FLASH_NUM){
        stSPIFlash[cs].id = cs;
        return _removeFlash(pM, &stSPIFlash[cs]);
    }else{
        return SPI_WORKER_RESULT_FAIL;
    }
}

int selectFlash(struct stMachineState *pM, int cs){ 
    if(cs < SPI_FLASH_NUM){
        stSPIFlash[cs].id = cs;
        return _selectFlash(pM, &stSPIFlash[cs]);
    }else{
        return SPI_WORKER_RESULT_FAIL;
    }
}

int deselectFlash(struct stMachineState *pM, int cs){
    if(cs < SPI_FLASH_NUM){
        return _deselectFlash(pM, &stSPIFlash[cs]);
    }else{
        return SPI_WORKER_RESULT_FAIL;
    }
}

uint8_t writeFlash(struct stMachineState *pM, int cs, uint8_t val){
    if(cs < SPI_FLASH_NUM){
        return _writeFlash(pM, &stSPIFlash[cs], val);
    }else{
        return SPI_WORKER_RESULT_FAIL;
    }
}
