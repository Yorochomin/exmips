#ifndef __DEV_SPIFLASH_H__ 
#define __DEV_SPIFLASH_H__ 

#define FLASH_CMD_ID              0x9F
#define FLASH_CMD_READ            0x03
#define FLASH_CMD_FAST_READ       0x0B
#define FLASH_CMD_FAST_READ_DUAL  0x3B
#define FLASH_CMD_FAST_READ_QUAD  0x6B
#define FLASH_CMD_CONT_READ_RESET 0xFF
#define FLASH_CMD_WRITE_ENABLE    0x06
#define FLASH_CMD_WRITE_DISABLE   0x04
#define FLASH_CMD_READ_SFDP       0x5A
#define FLASH_CMD_BLOCK_ERASE     0xD8
#define FLASH_CMD_SECTOR_ERASE    0x20
#define FLASH_CMD_PAGE_PROGRAM    0x02
#define FLASH_CMD_READ_SR1        0x05
#define FLASH_CMD_READ_SR2        0x35
#define FLASH_CMD_READ_SR3        0x33

int setFlashImageName(struct stMachineState *pM, int cs, char *filename);
int      initFlash(struct stMachineState *pM, int cs);
int    removeFlash(struct stMachineState *pM, int cs); 
int    selectFlash(struct stMachineState *pM, int cs); 
int  deselectFlash(struct stMachineState *pM, int cs);
uint8_t writeFlash(struct stMachineState *pM, int cs, uint8_t val);

#endif