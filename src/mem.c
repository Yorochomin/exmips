#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "mips.h"
#include "terminal.h"
#include "mem.h"
#include "tlb.h"
#include "logfile.h"
#include "dev_UART.h"
#include "dev_ATH79.h"
#include "dev_SPIFlash.h"
#include "exception.h"

#include "ExInst_common.h"

#include "dev_gmac.h"
#include "dev_SPI.h"
#include "mips_op.h"

#if USE_RUNTIME_COMPILATION
    #include "rc.h"
#endif

inline uintmx readPhysMemWord(struct stMachineState *pM, uintmx addr){
    uintmx addr1 = addr + 1;
    uintmx addr2 = addr + 2;
    uintmx addr3 = addr + 3;

    if( addr >= RAM_AREA_ADDR && addr3 < RAM_AREA_ADDR+RAM_AREA_SIZE ){
        addr  &= EMU_MEM0_AMASK;
        addr1 &= EMU_MEM0_AMASK;
        addr2 &= EMU_MEM0_AMASK;
        addr3 &= EMU_MEM0_AMASK;

        return ( (((uintmx)MEM_READ0(pM,addr )) <<24) |
                    (((uintmx)MEM_READ0(pM,addr1)) <<16) |
                    (((uintmx)MEM_READ0(pM,addr2)) << 8) |
                    (((uintmx)MEM_READ0(pM,addr3))     ) );
    }

    if( addr >= ROM_AREA_ADDR && addr3 < ROM_AREA_ADDR+ROM_AREA_SIZE ){
#if USE_SPI_EMULATION
        uint32_t spi_addr = addr - ROM_AREA_ADDR;
        uint32_t data = 0;
        selectFlash(pM, 0);
        writeFlash(pM, 0, FLASH_CMD_READ);
        writeFlash(pM, 0, (spi_addr>>16) & 0xff);
        writeFlash(pM, 0, (spi_addr>> 8) & 0xff);
        writeFlash(pM, 0, (spi_addr>> 0) & 0xff);

        data |= writeFlash(pM, 0, 0xff); data = data<<8;
        data |= writeFlash(pM, 0, 0xff); data = data<<8;
        data |= writeFlash(pM, 0, 0xff); data = data<<8;
        data |= writeFlash(pM, 0, 0xff);
        deselectFlash(pM, 0);

        return data;
#else
        addr  &= EMU_MEM1_AMASK;
        addr1 &= EMU_MEM1_AMASK;
        addr2 &= EMU_MEM1_AMASK;
        addr3 &= EMU_MEM1_AMASK;

        return ( (((uintmx)MEM_READ1(pM,addr )) <<24) |
                    (((uintmx)MEM_READ1(pM,addr1)) <<16) |
                    (((uintmx)MEM_READ1(pM,addr2)) << 8) |
                    (((uintmx)MEM_READ1(pM,addr3))     ) );
#endif
    }

    return 0;
}

inline void writePhysMemWord(struct stMachineState *pM, uintmx addr, uintmx data){
    uintmx addr1 = addr + 1;
    uintmx addr2 = addr + 2;
    uintmx addr3 = addr + 3;

    if( addr >= RAM_AREA_ADDR && addr3 < RAM_AREA_ADDR+RAM_AREA_SIZE ){
        addr  &= EMU_MEM0_AMASK;
        addr1 &= EMU_MEM0_AMASK;
        addr2 &= EMU_MEM0_AMASK;
        addr3 &= EMU_MEM0_AMASK;

        MEM_WRITE0(pM, addr , (data>>24)&0xff);
        MEM_WRITE0(pM, addr1, (data>>16)&0xff);
        MEM_WRITE0(pM, addr2, (data>> 8)&0xff);
        MEM_WRITE0(pM, addr3, (data    )&0xff);
        return;
    }
}

inline uintmx readPhysMemHalfWord(struct stMachineState *pM, uintmx addr){
    uintmx addr1 = addr + 1;

    if( addr >= RAM_AREA_ADDR && addr1 < RAM_AREA_ADDR+RAM_AREA_SIZE ){
        addr  &= EMU_MEM0_AMASK;
        addr1 &= EMU_MEM0_AMASK;

        return ( (((uintmx)MEM_READ0(pM, addr)) << 8) |
                    (((uintmx)MEM_READ0(pM, addr1))    ) );
    }

    if( addr >= ROM_AREA_ADDR && addr1 < ROM_AREA_ADDR+ROM_AREA_SIZE ){
#if USE_SPI_EMULATION
        uint32_t spi_addr = addr - ROM_AREA_ADDR;
        uint32_t data = 0;
        selectFlash(pM, 0);
        writeFlash(pM, 0, FLASH_CMD_READ);
        writeFlash(pM, 0, (spi_addr>>16) & 0xff);
        writeFlash(pM, 0, (spi_addr>> 8) & 0xff);
        writeFlash(pM, 0, (spi_addr>> 0) & 0xff);

        data |= writeFlash(pM, 0, 0xff); data = data<<8;
        data |= writeFlash(pM, 0, 0xff);
        deselectFlash(pM, 0);
        return data;
#else
        addr  &= EMU_MEM1_AMASK;
        addr1 &= EMU_MEM1_AMASK;

        return ( (((uintmx)MEM_READ1(pM, addr)) << 8) |
                    (((uintmx)MEM_READ1(pM, addr1))    ) );
#endif
    }

    return 0;
}

inline void writePhysMemHalfWord(struct stMachineState *pM, uintmx addr, uintmx data){
    uintmx addr1 = addr + 1;

    if( addr >= RAM_AREA_ADDR && addr1 < RAM_AREA_ADDR+RAM_AREA_SIZE ){
        addr  &= EMU_MEM0_AMASK;
        addr1 &= EMU_MEM0_AMASK;

        MEM_WRITE0(pM, addr,  (data>> 8)&0xff);
        MEM_WRITE0(pM, addr1, (data    )&0xff);
        return;
    }
}

inline uint8_t readPhysMemByte(struct stMachineState *pM, uintmx addr){

    if( addr >= RAM_AREA_ADDR && addr < RAM_AREA_ADDR+RAM_AREA_SIZE ){
        addr  &= EMU_MEM0_AMASK;
        return MEM_READ0(pM, addr);
    }

    if( addr >= ROM_AREA_ADDR && addr < ROM_AREA_ADDR+ROM_AREA_SIZE ){

#if USE_SPI_EMULATION
        uint32_t spi_addr = addr - ROM_AREA_ADDR;
        uint32_t data = 0;
        selectFlash(pM, 0);
        writeFlash(pM, 0, FLASH_CMD_READ);
        writeFlash(pM, 0, (spi_addr>>16) & 0xff);
        writeFlash(pM, 0, (spi_addr>> 8) & 0xff);
        writeFlash(pM, 0, (spi_addr>> 0) & 0xff);

        data |= writeFlash(pM, 0, 0xff);
        deselectFlash(pM, 0);
        return data;
#else
        addr  &= EMU_MEM1_AMASK;
        return MEM_READ1(pM, addr);
#endif
    }

    return 0;
}

inline void writePhysMemByte(struct stMachineState *pM, uintmx addr, uint8_t data){

    if( addr >= RAM_AREA_ADDR && addr < RAM_AREA_ADDR+RAM_AREA_SIZE ){
        addr  &= EMU_MEM0_AMASK;
        MEM_WRITE0(pM, addr, data);
        return;
    }
}

static uintmx getPhyAddr(struct  stMachineState *pM, uintmx addr, int isWrite, int *perror){
    *perror = 0;
    uintmx c0_status = C0_VAL(pM, C0_STATUS);
    int asid = (C0_VAL(pM, C0_ENTRYHI) & C0_ENTRYHI_ASID_MASK);

    if( addr < KSEG0 ){
        if( MODE_IS_IN_ERROR(c0_status) && (addr < (1<<29)) ){
            return addr;
        }else{
            return TLBLookUp(pM, asid, isWrite, addr, perror);
        }
    }

    if( MODE_IS_USER( c0_status ) ){
        *perror = REPORT_EXCEPT( isWrite ? EXCEPT_CODE_ADDR_ERR_STORE : EXCEPT_CODE_ADDR_ERR_LOAD);
        return 0;
    }

    if( addr >= KSEG0 && addr < KSEG1 ){
        if( MODE_IS_SUPERVISOR( c0_status ) ){
            *perror = REPORT_EXCEPT( isWrite ? EXCEPT_CODE_ADDR_ERR_STORE : EXCEPT_CODE_ADDR_ERR_LOAD);
            return 0;
        }

        addr = KSEG01_TO_PADDR(addr);
#if USE_SPI_EMULATION
        if( (0 == ((pM->mem.ioSPI.control) & (1<<SPI_CTRL_BIT_REMAP_DISABLE))) && addr >= KSEG01_TO_PADDR(EXCEPT_VECT_RESET) ){
            /*
              Memory image
              [1fc0_0000 - 1fff_ffff] -> [0x1f00_0000 - 0x1f3f_ffff]
            */
            return (addr & 0xff3fffff);
        }else{
            return addr;
        }
#else
        if( addr >= KSEG01_TO_PADDR(EXCEPT_VECT_RESET) ){
            /*
            Memory image
            [1fc0_0000 - 1fff_ffff] -> [0x1f00_0000 - 0x1f3f_ffff]
            */
            return (addr & 0xff3fffff);
        }else{
            return addr;
        }
#endif
    }

    if( addr >= KSEG1 && addr < KSEG2 ){
        if( MODE_IS_SUPERVISOR( c0_status ) ){
            *perror = REPORT_EXCEPT( isWrite ? EXCEPT_CODE_ADDR_ERR_STORE : EXCEPT_CODE_ADDR_ERR_LOAD);
            return 0;
        }

        addr = KSEG01_TO_PADDR(addr);
#if USE_SPI_EMULATION
        if( (0 == ((pM->mem.ioSPI.control) & (1<<SPI_CTRL_BIT_REMAP_DISABLE))) && addr >= KSEG01_TO_PADDR(EXCEPT_VECT_RESET) ){
            /*
              Memory image
              [1fc0_0000 - 1fff_ffff] -> [0x1f00_0000 - 0x1f3f_ffff]
            */
            return (addr & 0xff3fffff);
        }else{
            return addr;
        }
#else
        if( addr >= KSEG01_TO_PADDR(EXCEPT_VECT_RESET) ){
            /*
            Memory image
            [1fc0_0000 - 1fff_ffff] -> [0x1f00_0000 - 0x1f3f_ffff]
            */
            return (addr & 0xff3fffff);
        }else{
            return addr;
        }
#endif
    }

    if( addr >= KSEG2 && addr < KSEG3 ){
        return TLBLookUp(pM, asid, isWrite, addr, perror);
    }

    if( addr >= KSEG3 ){
        if( MODE_IS_SUPERVISOR( c0_status ) ){
            *perror = REPORT_EXCEPT( isWrite ? EXCEPT_CODE_ADDR_ERR_STORE : EXCEPT_CODE_ADDR_ERR_LOAD);
            return 0;
        }
        return TLBLookUp(pM, asid, isWrite, addr, perror);
    }

    return addr;
}

uintmx getPhysicalAddress(struct  stMachineState *pM, uintmx addr, int isWrite, int *perror){
    return getPhyAddr(pM, addr, isWrite, perror);
}

uint32_t fetchCodeWord(struct stMachineState *pM){
    int aerror;
    uintmx addr, inst, op;
    uintmx asid = (C0_VAL(pM, C0_ENTRYHI) & C0_ENTRYHI_ASID_MASK);
    // if ERL or EXL bits are 1, checkAddrCache will fail
    uintmx mode = (C0_VAL(pM, C0_STATUS)  & (C0_STATUS_KSU_MASK | (1<<C0_STATUS_BIT_ERL) | (1<<C0_STATUS_BIT_EXL)));

    if( (pM->reg.pc & 3) == 2 ){
        // If the lower 2 bits of PC is 10, PC value is misaligned.
        //   ...00 is valid MIPS32 address.
        //   ...01 and ...11 are valid MIPS16 address. 
        //   Note that the LSB, i.e., 1, is the mode bit representing MIPS16 mode.
        prepareException(pM, EXCEPT_CODE_ADDR_ERR_LOAD, pM->reg.pc);
        return fetchCodeWord(pM);
    }

    /*
     * In a 4K-byte page, physical address is linear.
     * This function generates physical address using the cached pair 
     * of a previous virtual address and its corresonding physical page.
     * The mapping from virtual to physical may change 
     * when TLB-write occurs or the setting of memory-remap changes.
     * Thus, this cache is cleared in TLBWrite function and writing to SPI registers.
     */
    if( checkAddrCache(&pM->reg.pc_cache, pM->reg.pc, asid, mode) ){
        addr = getAddrWithAddrCache(&pM->reg.pc_cache, pM->reg.pc);
    }else{
        addr = getPhyAddr(pM, pM->reg.pc, 0, &aerror);
        if(aerror){
            prepareException(pM, GET_EXCEPT_CODE(aerror), pM->reg.pc);
            return fetchCodeWord(pM);
        }else{
            setAddrCache(&pM->reg.pc_cache, pM->reg.pc, asid, mode, addr);
        }
    }

    if( pM->reg.pc & 1 ){
        inst = readPhysMemHalfWord(pM, addr&(~1));

        op = ((inst>>11)&0x1f); 
        if( op == MIPS16E_OP_EXTEND || op == MIPS16E_OP_JAL ){

            uintmx nextaddr = pM->reg.pc + 1;
            if( checkAddrCache(&pM->reg.pc_cache, nextaddr, asid, mode) ){
                addr = getAddrWithAddrCache(&pM->reg.pc_cache, nextaddr);
            }else{
                addr = getPhyAddr(pM, nextaddr, 0, &aerror);
                if(aerror){
                    prepareException(pM, GET_EXCEPT_CODE(aerror), nextaddr);
                    return fetchCodeWord(pM);
                }else{
                    setAddrCache(&pM->reg.pc_cache, nextaddr, asid, mode, addr);
                }
            }

            inst = inst << 16;
            inst|= readPhysMemHalfWord(pM, addr);
        }
        // Because code density is higher than MIPS32, prefetch instruction is not used in MIPS16.

        return inst;
    }else{
        inst = readPhysMemWord(pM, addr&(~3));
        PREFETCH_VM_MEM(pM, addr+4, 0, 0);
        return inst;
    }
}

static inline uintmx accSizeAlign(uintmx width, uintmx addr, uintmx val){
    if(width == 4) return val;
    if(width == 2){
        if( addr & 1 ){
            return (val&0xffff);
        }else{
            return ((val>>16)&0xffff);
        }
    }
    if(width == 1){
        switch( addr & 3 ){
            case 0: return ((val>>24)&0xff);
            case 1: return ((val>>16)&0xff);
            case 2: return ((val>> 8)&0xff);
            case 3: return ((val>> 0)&0xff);
        }
    }
    return 0;
}

static inline uintmx wrDataAlign(uintmx width, uintmx addr, uintmx val){
    if(width == 4) return val;
    if(width == 2){
        if( addr & 1 ){
            return (val&0xffff);
        }else{
            return ((val<<16)&0xffff0000);
        }
    }
    if(width == 1){
        switch( addr & 3 ){
            case 0: return ((val<<24)&0xff000000);
            case 1: return ((val<<16)&0x00ff0000);
            case 2: return ((val<< 8)&0x0000ff00);
            case 3: return ((val<< 0)&0x000000ff);
        }
    }
    return 0;
}

static inline uintmx loadMemory(struct stMachineState *pM, uintmx addr, int accwidth, int *perror){ 
    uintmx asid = (C0_VAL(pM, C0_ENTRYHI) & C0_ENTRYHI_ASID_MASK);
    uintmx mode = (C0_VAL(pM, C0_STATUS)  & (C0_STATUS_KSU_MASK | (1<<C0_STATUS_BIT_ERL) | (1<<C0_STATUS_BIT_EXL)));
    if( checkAddrCache(&pM->reg.dr_cache, addr, asid, mode) ){
        addr = getAddrWithAddrCache(&pM->reg.dr_cache, addr);
        *perror = 0;
    }else{
        uintmx vaddr = addr;
        addr = getPhyAddr(pM, addr, 0, perror);
        if(*perror){
            return 0;
        }else{
            setAddrCache(&pM->reg.dr_cache, vaddr, asid, mode, addr);
        }
    }

    uintmx alignAddr = (addr & (~3));

    if( addr >= RAM_AREA_ADDR && addr < RAM_AREA_ADDR+RAM_AREA_SIZE ){

        switch( accwidth ){
            case 4: return readPhysMemWord(pM, addr);
            case 1: return readPhysMemByte(pM, addr);
            case 2: return readPhysMemHalfWord(pM, addr);
        }

    }else if(addr >= ROM_AREA_ADDR && addr < ROM_AREA_ADDR+ROM_AREA_SIZE){
        // SPI peripheral or SPI flash

#if USE_SPI_EMULATION
        if( pM->mem.ioSPI.functionSelect & 1 ){
            // GPIO mode is enabled. FLASH memory is not mapped to memory area.
            // Peripheral registers are visible.
            if(addr >= SPI0_BASE_ADDRESS && addr < SPI0_BASE_ADDRESS+SPI_ADDR_SIZE){
                return accSizeAlign(accwidth, addr, readSPIReg(pM, &(pM->mem.ioSPI), alignAddr));
            }else{
                return 0;
            }
        }else{
            // GPIO mode is disabled. SPI flash memory data is mapped to this region.
            // Peripheral registers are not visible.
            switch( accwidth ){
                case 4: return readPhysMemWord(pM, addr);
                case 1: return readPhysMemByte(pM, addr);
                case 2: return readPhysMemHalfWord(pM, addr);
            }
        }
#else
        switch( accwidth ){
            case 4: return readPhysMemWord(pM, addr);
            case 1: return readPhysMemByte(pM, addr);
            case 2: return readPhysMemHalfWord(pM, addr);
        }
#endif

    }else if(addr >= IOADDR_UART0_BASE && addr < IOADDR_UART0_BASE+IOADDR_UART_SIZE){
        // UART
        return accSizeAlign(accwidth, addr, readUARTReg(pM, &(pM->mem.ioUART0), alignAddr));

    }else if(addr >= GPIO_BASE_REG && addr < GPIO_BASE_REG+0x100){
        // GPIO

        if( alignAddr == GPIO_BASE_REG + 0x00 /*GPIO_OE*/  ) return accSizeAlign(accwidth, addr, pM->mem.ioGPIO.oe);
        if( alignAddr == GPIO_BASE_REG + 0x08 /*GPIO_OUT*/ ) return accSizeAlign(accwidth, addr, pM->mem.ioGPIO.out);

    }else if(addr >= RTC_BASE_REG && addr < RTC_BASE_REG+0x5c){
        // RTC

        if( alignAddr == RTC_BASE_REG + 0x44 ) return accSizeAlign(accwidth, addr, 2);

    }else if(addr >= RST_BASE_REG && addr < RST_BASE_REG+0x100){
        // RESET

        switch( alignAddr ){
            case RST_MISC_INTERRUPT_STATUS_REG: return accSizeAlign(accwidth, addr, readMiscIntStatusReg(pM));
            case RST_BOOTSTRAP_REG            : return accSizeAlign(accwidth, addr, (7<<8) | (1<<2) | (1<<4)); // Reference clock : 40MHz
            case RST_REVISION_ID_REG          : return accSizeAlign(accwidth, addr, RST_REVISION_ID_MAJOR_AR9342_VAL | 3); // SOC index (AR9342)
            case RST_MISC_INTERRUPT_MASK_REG  : return accSizeAlign(accwidth, addr, pM->mem.ioMISC.int_mask);
        }

    }else if(addr >= GMAC_BASE_REG && addr < GMAC_BASE_REG + 0x1000){
        // GMAC

//        printf("read GMAC register 0x%x\n", addr);

    }else if(addr >= GMAC0_BASE_REG && addr <= GMAC0_BASE_REG + GMACX_ADDR_MASK){
        // GMAC0

        return accSizeAlign(accwidth, addr, readGMACXReg(pM, &(pM->mem.ioGMACX0), alignAddr));
        
    }else if(addr >= GMAC1_BASE_REG && addr <= GMAC1_BASE_REG + GMACX_ADDR_MASK){
        // GMAC1
        
//        printf("read GMAC1 register 0x%x\n", addr);

    }else if(addr >= PLL_BASE_REG && addr < PLL_BASE_REG+0x100){
        // PLL

        if( alignAddr == PLL_CPU_DDR_CLK_CTRL_REG ) return accSizeAlign(accwidth, addr, (1<<20)); // CPU clock from CPU PLL

    }else if(addr >= PLL_SRIF_CPU_DPLL_BASE_REG && addr < PLL_SRIF_CPU_DPLL_BASE_REG+0x100){
        // SRIF

        if( alignAddr == PLL_SRIF_CPU_DPLL1_REG ) return accSizeAlign(accwidth, addr, ((1<<27 /*refdiv*/) + (10<<18 /*nint*/) + (0 /*nfrac*/))); //
        if( alignAddr == PLL_SRIF_CPU_DPLL2_REG ) return accSizeAlign(accwidth, addr, (1<<30) + (0<<13 /*outdiv*/)); //
    }

    //printf("[memory read: 0x%x]\n", addr);

    return 0;
}

uint8_t loadByte(struct stMachineState *pM, uintmx addr, int *perror){
    return loadMemory(pM, addr, 1, perror);
}
uintmx loadHalfWord(struct stMachineState *pM, uintmx addr, int *perror){ 
    // Checking the alignment
    if( (addr&1) != 0 ){
        *perror = REPORT_EXCEPT( EXCEPT_CODE_ADDR_ERR_LOAD );
        return 0;
    }
    return loadMemory(pM, addr, 2, perror);
}
uintmx loadWord(struct stMachineState *pM, uintmx addr, int *perror){ 
    // Checking the alignment
    if( (addr&3) != 0 ){
        *perror = REPORT_EXCEPT( EXCEPT_CODE_ADDR_ERR_LOAD );
        return 0;
    }
    return loadMemory(pM, addr, 4, perror);
}


static inline void storeMemory(struct stMachineState *pM, uintmx addr, int accwidth, uintmx data, int *perror){

#if !USE_RUNTIME_COMPILATION
    if( ((accwidth == 1) && ( addr ==  pM->mem.watchAddr     )) ||
        ((accwidth == 2) && ( addr == (pM->mem.watchAddr&(~1))))||
        ((accwidth == 4) && ( addr == (pM->mem.watchAddr&(~3)))) ){
        printf("<WRITE addr:0x%x, data:0x%x (at pc = 0x%08x)", addr, data, REG_PC(pM));
        logfile_printf(LOGCAT_CPU_MEM|LOGLV_ERROR, "<WRITE addr:0x%x, data:0x%x (at pc = 0x%08x)\n", addr, data, REG_PC(pM));
    }
#endif

    uintmx asid = (C0_VAL(pM, C0_ENTRYHI) & C0_ENTRYHI_ASID_MASK);
    uintmx mode = (C0_VAL(pM, C0_STATUS)  & (C0_STATUS_KSU_MASK | (1<<C0_STATUS_BIT_ERL) | (1<<C0_STATUS_BIT_EXL)));
    if( checkAddrCache(&pM->reg.dw_cache, addr, asid, mode) ){
        addr = getAddrWithAddrCache(&pM->reg.dw_cache, addr);
        *perror = 0;
    }else{
        uintmx vaddr = addr;
        addr = getPhyAddr(pM, addr, 1, perror);
        if(*perror){
            return ;
        }else{
            setAddrCache(&pM->reg.dw_cache, vaddr, asid, mode, addr);
        }
    }

    uintmx alignAddr = (addr & (~3));

    if( addr >= RAM_AREA_ADDR && addr < RAM_AREA_ADDR+RAM_AREA_SIZE ){

        switch( accwidth ){
            case 1: writePhysMemByte(pM, addr, data); break;
            case 2: writePhysMemHalfWord(pM, addr, data); break;
            case 4: writePhysMemWord(pM, addr, data); break;
        }
        return;

    }else if(addr >= SPI0_BASE_ADDRESS && addr <= SPI0_BASE_ADDRESS+SPI_ADDR_SIZE){
        // SPI

        uint32_t alignData = wrDataAlign(accwidth, addr, data);
#if USE_SPI_EMULATION
        /* 
         * Writing SPI register, SPI_CONTROL_ADDR, may change memory mapping.
         * Therefore, address caches should be cleared.
         */
        if( alignAddr == SPI0_BASE_ADDRESS + SPI_CTRL_REG && ( (pM->mem.ioSPI.control ^ alignData) & (1<<SPI_CTRL_BIT_REMAP_DISABLE) ) ){
#if USE_RUNTIME_COMPILATION
            rc_clear();
#endif
            clearAddrCache(&pM->reg.pc_cache);
            clearAddrCache(&pM->reg.dr_cache);
            clearAddrCache(&pM->reg.dw_cache);
        }

        writeSPIReg(pM, &(pM->mem.ioSPI), alignAddr, alignData );
        return;
#endif

    }else if( addr >= IOADDR_UART0_BASE && addr < IOADDR_UART0_BASE+IOADDR_UART_SIZE ){
        // UART

        writeUARTReg(pM, &(pM->mem.ioUART0), alignAddr, wrDataAlign(accwidth, addr, data) );
        return;

    }else if(addr >= GPIO_BASE_REG && addr < GPIO_BASE_REG+0x100){
        // GPIO

        if( alignAddr == GPIO_BASE_REG + 0x00 /*GPIO_OE*/  ){ pM->mem.ioGPIO.oe  = wrDataAlign(accwidth, addr, data); }
        if( alignAddr == GPIO_BASE_REG + 0x08 /*GPIO_OUT*/ ){ pM->mem.ioGPIO.out = wrDataAlign(accwidth, addr, data); }
        if( alignAddr == GPIO_BASE_REG + 0x0C /*GPIO_SET*/ ){ pM->mem.ioGPIO.out|= wrDataAlign(accwidth, addr, data); }
        if( alignAddr == GPIO_BASE_REG + 0x10 /*GPIO_CLR*/ ){ pM->mem.ioGPIO.out&=~wrDataAlign(accwidth, addr, data); }

    }else if(addr >= RST_BASE_REG && addr < RST_BASE_REG+0x100){
        // RESET

//        if( alignAddr == RST_MISC_INTERRUPT_STATUS_REG ){ printf("write misc int status  %x\n", data); }
        if( alignAddr == RST_MISC_INTERRUPT_MASK_REG   ){/* printf("write misc int mask    %x\n", data);*/ pM->mem.ioMISC.int_mask = data; }
//        if( alignAddr == RST_GLOBALINTERRUPT_STATUS_REG){ printf("write globl int status %x\n", data); }
        if( alignAddr == RST_RESET_REG                 ){/* printf("write reset reg        %x\n", data);*/ pM->mem.ioMISC.reset_request = (data&(1<<24)); /* FULL CHIP RESET */ }

//        printf("wrote 0x%x to 0x%x @ 0x%x\n",data, alignAddr, REG_PC(pM));

    }else if(addr >= GMAC_BASE_REG && addr < GMAC_BASE_REG + 0x1000){

//        printf("write GMAC register 0x%x val 0x%x\n", addr, data);

    }else if(addr >= GMAC0_BASE_REG && addr <= GMAC0_BASE_REG + GMACX_ADDR_MASK){

        writeGMACXReg(pM, &(pM->mem.ioGMACX0), alignAddr, wrDataAlign(accwidth, addr, data) );
        return;

    }else if(addr >= GMAC1_BASE_REG && addr < GMAC1_BASE_REG + 0x100){

//        printf("write GMAC1 register 0x%x val 0x%x\n", addr, data);
    }
}

void storeByte(struct stMachineState *pM, uintmx addr, uint8_t  data, int *perror){
    storeMemory(pM, addr, 1, data, perror);
}
void storeHalfWord(struct stMachineState *pM, uintmx addr, uintmx data, int *perror){
    if( (addr&1) != 0 ){
        *perror = REPORT_EXCEPT( EXCEPT_CODE_ADDR_ERR_STORE );
    }else{
        storeMemory(pM, addr, 2, data, perror);
    }
}
void storeWord(struct stMachineState *pM, uintmx addr, uintmx data, int *perror){
    if( (addr&3) != 0 ){
        *perror = REPORT_EXCEPT( EXCEPT_CODE_ADDR_ERR_STORE );
    }else{
        storeMemory(pM, addr, 4, data, perror);
    }
}

/*
 * Following memory access codes are for DMA memory access and not used for emulating the processor.
 */
uint32_t loadPhysMemWord(struct stMachineState *pM, uintmx addr){
    return readPhysMemWord(pM, addr);
}

void storePhysMemWord(struct stMachineState *pM, uintmx addr, uintmx data){
    writePhysMemWord(pM, addr, data);
}

uint32_t loadPhysMemHalfWord(struct stMachineState *pM, uintmx addr){
    return readPhysMemHalfWord(pM, addr);
}

uint8_t loadPhysMemByte(struct stMachineState *pM, uintmx addr){
    return readPhysMemByte(pM, addr);
}

void storePhysMemByte(struct stMachineState *pM, uintmx addr, uintmx data){
    writePhysMemByte(pM, addr, data);
}
