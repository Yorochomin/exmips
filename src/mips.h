#ifndef __MIPS_H__ 
#define __MIPS_H__ 

#include "config.h"
#include "cp0def.h"

#include "env_mem.h"
#include "dev_SPIFlashParam.h"

#define KUSEG       0x00000000
#define KUSEG_SIZE  0x80000000
#define KSEG0       0x80000000
#define KSEG0_SIZE  0x20000000
#define KSEG1       0xA0000000
#define KSEG1_SIZE  0x20000000
#define KSEG2       0xC0000000
#define KSEG2_SIZE  0x20000000
#define KSEG3       0xE0000000
#define KSEG3_SIZE  0x20000000

#define KSEG01_TO_PADDR(a)   ((a) & 0x1fffffff)
#define EMU_MEM1_IMAGE(a)    ((a) >= KSEG01_TO_PADDR(EXCEPT_VECT_RESET) ? ((a) & 0xff3fffff) : (a))

// RAM area is at most 256MB
#define RAM_AREA_ADDR (0x00000000)
#define RAM_AREA_SIZE (0x10000000)

// ROM area is at most 16MB
#define ROM_AREA_ADDR (0x1f000000)
#define ROM_AREA_SIZE (0x01000000)

#define EMU_MEM0_ADDR     RAM_AREA_ADDR
#define EMU_MEM0_SIZE     (1<<EMU_MEM0_AWIDTH)
#define EMU_MEM0_AMASK    (EMU_MEM0_SIZE-1)

#define EMU_MEM1_ADDR     ROM_AREA_ADDR
#define EMU_MEM1_SIZE     (1<<EMU_MEM1_AWIDTH)
#define EMU_MEM1_AMASK    (EMU_MEM1_SIZE-1)

#define EMU_START_ADDR    EXCEPT_VECT_RESET

#if !defined( MIPS_MXLEN )
    #define MIPS_MXLEN     32
#elif MIPS_MXLEN == 32
#elif MIPS_MXLEN == 64
#else
    #error "MIPS_MXLEN is invalid"
#endif

#if MIPS_MXLEN == 32 
typedef uint32_t uintmx;
typedef  int32_t  intmx;
#elif MIPS_MXLEN == 64
typedef uint64_t uintmx;
typedef  int64_t  intmx;
#else
#endif


#ifndef DEBUG
    extern int DEBUG;
#endif


//#define EXCEPT_VECT_OTHER(ebase, bev)          ((bev)? 0xbfc0200 : (0x80000000|((ebase)&0x3ffff000)))

struct stEmuSetting{
    uintmx breakPoint;
    uintmx breakMask;
    uint32_t runAfterBreak;
    uint64_t breakCounter;
    uint64_t nExecInsts;
	uint64_t execRate; /* executed instructions per second in the last host timer period */
    int      stop;
    int      bWriteBackFlashData;
    unsigned int log_enabled_cat;
    unsigned int log_level;
    const struct stSPIFlashParam *spiflash_param;
};

#define CP_REG_BITS    5
#define CP_SEL_BITS    3

struct stTLBPhyAddr{
    uintmx field_pfn;
    uintmx field_dirty;
    uintmx field_valid;
};

struct stTLBEntry{
    uintmx entryhi;
    uintmx entrylo0;
    uintmx entrylo1;

    uintmx field_asid; // located in entryhi[ 7: 0]
    uintmx field_pmask;// located in pagemask
    uintmx field_vpn2; // located in entryhi[31:13]
    uintmx field_g;    // logical AND of g bits of EntryLo0 and EntryLo1
    struct stTLBPhyAddr lo[2];
};

struct stAddrCache{
    uintmx vaddr;  /* virtual address */
    uintmx ppage;  /* [31:12] phy page address */
    uint8_t asid;  /* asid */
    uint8_t valid;
    uint8_t mode;  /* [4:3] mode, [2] Error Level, [1] Exception Level */
};

static inline void clearAddrCache(struct stAddrCache *stAC){
    stAC->valid = 0;
}
static inline int checkAddrCache(struct stAddrCache *stAC, uintmx vaddr, uint8_t asid, uint8_t mode){
    uintmx mask = ~((uintmx)0xfff);

    if( ! stAC->valid ) return 0;

    if( mode != stAC->mode ) return 0;

    if( (vaddr & mask) == stAC->vaddr ){
        if( asid == stAC->asid ){
            return 1;
        }
    }
    return 0;
}
static inline uintmx getAddrWithAddrCache(struct stAddrCache *stAC, uintmx vaddr){
    return (stAC->ppage | (vaddr & 0xfff));
}
static inline void setAddrCache(struct stAddrCache *stAC, uintmx vaddr, uintmx asid, uintmx mode, uintmx paddr){
    uintmx mask = ~((uintmx)0xfff);

    stAC->valid  = 1;
    stAC->ppage  = ( paddr & mask );
    stAC->asid   = ( asid & C0_ENTRYHI_ASID_MASK );
    stAC->mode   = ( mode & 0xff );
    stAC->vaddr  = ( vaddr & mask );
}


struct stReg{
    uintmx r[32];
    uintmx pc;
    uintmx pc_delay;
    uintmx pc_prev_jump;
    uintmx hi;
    uintmx lo;

    uint8_t delay_en;
    uintmx ll_sc;

    uint64_t c0_count_basetime;       /* base time in usec */
    uint64_t c0_count_currenttime;    /* current time in usec */
    uint64_t c0_count_ninst_in_ctime; /* nExecInsts in the current time */
    uint64_t c0_compare_long;         /* long version of c0_compare */

    struct stAddrCache pc_cache;
    struct stAddrCache dr_cache;
    struct stAddrCache dw_cache;
    struct stTLBEntry tlb[NUM_TLB_ENTRY];
    uintmx cp0[1<<(CP_REG_BITS + CP_SEL_BITS)];
};

#include <time.h>

struct stIO_UART{
    int buffered;
    uint8_t intEnable;
    uint8_t intIdent;
    uint8_t lineControl;
    uint8_t modemControl;
    uint8_t divisor[2];
    uint8_t buf[1];
    uint8_t scratch;
	uint8_t breakRequest;
};

struct stIO_GPIO{
    uint32_t oe;
    uint32_t out;
};

#include <linux/if.h>

struct stIO_GMACX{
    int avail;

    int fd;
    char dev[IFNAMSIZ];

    uint32_t mac_config1; /* value written for GMACX_MAC_CONFIG1_REG */
    uint32_t mac_config2; /* value written for GMACX_MAC_CONFIG2_REG */

    uint32_t maxFrameLen; /* value written for GMACX_MAX_FRAME_LEN_REG */

    uint32_t mii_config;  /* value written for GMACX_MII_CONFIG_REG */
    uint32_t mii_cmd;     /* value written for GMACX_MII_CMD_REG */
    uint32_t mii_addr;    /* value written for GMACX_MII_ADDR_REG */
    uint32_t mii_status;  /* value for GMACX_MII_STATUS_REG */

    uint32_t mii[5];      /* MII register */

    uint32_t sta_address[2]; /* value of GMACX_STA_ADDRESSx_REG */
    uint32_t eth_config[6];  /* value of GMACX_ETH_CONFIGx_REG */

    uint32_t interfaceCtrl; /* value written for GMACX_INTERFACE_CONTROL_REG */

    uint32_t tx_ctrl;
    uint32_t tx_desc;
    uint32_t tx_status;
    uint32_t rx_ctrl;
    uint32_t rx_desc;
    uint32_t rx_status;
    uint32_t int_enable;
    uint32_t int_status;
    uint32_t fifo_depth;
};

struct stIO_SPI{
    uint32_t functionSelect;
    uint32_t control;
    uint32_t ioControl;
    uint32_t readDataAddr;
    uint32_t shiftDataOut;
    uint32_t shiftCount;
    uint32_t shiftDataIn;
};

struct stIO_MISC{
    uint32_t int_mask;
    uint32_t reset_request;
};

struct stMemIORegion{
    uint32_t watchAddr; /* watch address (disabled if watchAddr >= EMU_MEM_SIZE) */

    struct stIO_UART ioUART0;
//    struct stIO_UART ioUART1;

    struct stIO_GMACX ioGMACX0;
//    struct stIO_GMACX ioGMACX1;

    struct stIO_GPIO ioGPIO;
    struct stIO_MISC ioMISC;
    struct stIO_SPI  ioSPI;

    struct stMemRegion stMem; /* main memory. implementation is system dependent. */
};

struct stMachineState{
    struct stEmuSetting  emu;
    struct stReg reg;
    struct stMemIORegion mem;
};

#include "env_unix.h"

uint32_t parseHex(char *str);

#endif
