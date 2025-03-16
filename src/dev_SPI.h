#ifndef __DEV_SPI_H__ 
#define __DEV_SPI_H__ 

// Base addresses
#define SPI0_BASE_ADDRESS       0x1F000000

#define SPI_ADDR_SIZE 0x1c

#define SPI_ADDR_MASK 0x1f

#define SPI_MAX_SLAVES  3

/*
 * SPI serial flash registers
 */
#define SPI_FUNC_SEL_REG        0x00
#define SPI_CTRL_REG            0x04
#define SPI_IO_CTRL_REG         0x08
#define SPI_READ_DATA_REG       0x0C
#define SPI_SHIFT_DATAOUT_REG   0x10
#define SPI_SHIFT_CNT_REG       0x14
#define SPI_SHIFT_DATAIN_REG    0x18

#define SPI_CTRL_BIT_REMAP_DISABLE      6

#define SPI_IO_CTRL_BIT_IO_DO   0
#define SPI_IO_CTRL_BIT_IO_CLK  8
#define SPI_IO_CTRL_BIT_IO_CS0  16
#define SPI_IO_CTRL_BIT_IO_CS1  17
#define SPI_IO_CTRL_BIT_IO_CS2  18

#define SPI_SHIFT_CNT_BIT_TERMINATE       26
#define SPI_SHIFT_CNT_BIT_SHIFT_CLKOUT    27
#define SPI_SHIFT_CNT_BIT_SHIFT_CHNL_CS0  28
#define SPI_SHIFT_CNT_BIT_SHIFT_CHNL_CS1  29
#define SPI_SHIFT_CNT_BIT_SHIFT_CHNL_CS2  30
#define SPI_SHIFT_CNT_BIT_SHIFT_EN        31
#define SPI_SHIFT_CNT_BIT_SHIFT_COUNT     0
#define SPI_SHIFT_CNT_SHIFT_COUNT_MASK    0x7f

#define SPI_WORKER_RESULT_OK    1
#define SPI_WORKER_RESULT_FAIL -1



int         initSPI(struct stMachineState *pM, struct stIO_SPI *pSPI);
int       removeSPI(struct stMachineState *pM, struct stIO_SPI *pSPI);
uint32_t readSPIReg(struct stMachineState *pM, struct stIO_SPI *pSPI, uint32_t addr);
void    writeSPIReg(struct stMachineState *pM, struct stIO_SPI *pSPI, uint32_t addr, uint32_t data);


struct stSPIWorker{
    int (*init)(struct stMachineState *, int);
    int (*remove)(struct stMachineState *, int);
    int (*select)(struct stMachineState *, int);
    int (*deselect)(struct stMachineState *, int);
    uint8_t (*write)(struct stMachineState *, int, uint8_t val);
};

#endif