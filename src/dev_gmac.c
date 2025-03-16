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
#include "exception.h"

#include "ExInst_common.h"

#include "dev_gmac.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/select.h>

#include "mii.h"

#if defined(DEBUG) && DEBUG == 1
#define GMAC_DEBUG
#endif

#define GMACX_INT_STATUS_BIT_TXPKTSENT     0
#define GMACX_INT_STATUS_BIT_RXPKTRECEIVED 4


void setGMACXTAPIFName(struct stMachineState *pM, struct stIO_GMACX *pGMACX, char *ifname){
    if( ifname != NULL && *ifname != 0 ){
        strncpy(pGMACX->dev, ifname, IFNAMSIZ-1);
    }else{
        pGMACX->dev[0] = '\0';
    }
}

/*
 This function is based on https://docs.kernel.org/networking/tuntap.html
 */
int initGMACX(struct stMachineState *pM, struct stIO_GMACX *pGMACX){
    int fd, err;
    struct ifreq ifr;
    
    pGMACX->fd = -1;

    if( (fd = open("/dev/net/tun", O_RDWR)) < 0 ){
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));


    // Ethernet packet (IFF_TAP) and no packet information (IFF_NO_PI)
    ifr.ifr_flags = (IFF_TAP | IFF_NO_PI);
    strncpy(ifr.ifr_name, pGMACX->dev, IFNAMSIZ);
    if( pGMACX->dev[0] ){
        PRINTF("Try to use TAP/TUN interface \"%s\"\n", pGMACX->dev);
    }

    if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ){
       close(fd);
       return err;
    }
    strncpy(pGMACX->dev, ifr.ifr_name, IFNAMSIZ);


    if( pGMACX->dev[0] ){
        PRINTF("Use TAP/TUN interface \"%s\"\n", pGMACX->dev);
    }

    pGMACX->fd = fd;

    return fd;
}

void removeGMACX(struct stMachineState *pM, struct stIO_GMACX *pGMACX){

}


static int loadDescriptor(struct stMachineState *pM, struct ag71xx_desc *pDesc, uint32_t descAddr){
    descAddr &= (~0xf);
    pDesc->data = loadPhysMemWord(pM, descAddr+ 0);
    pDesc->ctrl = loadPhysMemWord(pM, descAddr+ 4);
    pDesc->next = loadPhysMemWord(pM, descAddr+ 8);
    pDesc->pad  = loadPhysMemWord(pM, descAddr+12);

#if defined(GMAC_DEBUG)
    PRINTF("Descriptor: addr 0x%x data 0x%x, ctrl 0x%x, next 0x%x, pad 0x%x\n", descAddr, pDesc->data, pDesc->ctrl, pDesc->next, pDesc->pad);
#endif
    return 0;
}

static void txDMA(struct stMachineState *pM, struct stIO_GMACX *pGMACX){
    struct ag71xx_desc desc;
    int i=0, len, int_req = 0;
    char buf[4096], b;
    ssize_t sent;

    if( pGMACX->fd < 0 ) return;

    do{
        if( pGMACX->tx_desc == 0) break ;

        loadDescriptor(pM, &desc, pGMACX->tx_desc);

        if( desc.ctrl & DESC_EMPTY ){
            break ;
        }

        if( pGMACX->tx_desc == desc.next ) break ;

        len = (desc.ctrl&0xfff);

        if( len == 0 ) break;

#if defined(GMAC_DEBUG)
        PRINTF("TX Packet: ");
#endif

        for(int a = 0; a < len; a++){
            b = loadPhysMemByte(pM, desc.data+a);
            buf[a] = b;
#if defined(GMAC_DEBUG)
            PRINTF("%02x ", (uint8_t)b);
#endif
        }
        if( desc.ctrl & (1<<24) ){
            int_req = 1;
        }
        sent = write(pGMACX->fd, buf, len);

#if defined(GMAC_DEBUG)
        PRINTF(" Packet TX %s (len %d, sent %ld)\n", (len==sent) ? "OK": "fail", len, sent);
#endif

        storePhysMemWord( pM, pGMACX->tx_desc+4,  (1<<31) | desc.ctrl );

        pGMACX->tx_desc = desc.next;

        uint32_t count = ((pGMACX->tx_status>>16) & 0xff);
        count++;
        pGMACX->tx_status &= 0xff00ffff;
        pGMACX->tx_status |= ((count<<16) & 0x00ff0000);
    }while( ++i < 100 );

    if( pGMACX->tx_status & 0x00ff0000 ){
        pGMACX->tx_status  |= 1;
        pGMACX->int_status |= (1<<GMACX_INT_STATUS_BIT_TXPKTSENT);
    }else{
        pGMACX->tx_status  &= (~1);
        pGMACX->int_status &=~(1<<GMACX_INT_STATUS_BIT_TXPKTSENT);
    }
    if( int_req ){
        pGMACX->int_status |= (1<<GMACX_INT_STATUS_BIT_TXPKTSENT);
    }

    pGMACX->tx_ctrl &= (~1);
}

void updateGMACX(struct stMachineState *pM, struct stIO_GMACX *pGMACX){
    struct ag71xx_desc desc;
    fd_set rd_set;
    int ret;
    char buf[4096], b;
	struct timeval tv;

    if( pGMACX->fd < 0) return;

    if( pGMACX->tx_ctrl & 1 ) txDMA(pM, pGMACX);

    if(!(pGMACX->rx_ctrl & 1)) return;

    FD_ZERO(&rd_set);
    FD_SET(pGMACX->fd, &rd_set);

    tv.tv_sec  = 0;
    tv.tv_usec = 0;
    ret = select(pGMACX->fd + 1, &rd_set, NULL, NULL, &tv);

    if (ret < 0 && errno == EINTR){
        return;
    }

    if (ret < 0) {
        pGMACX->fd = -1;
        return;
    }

    if( ! FD_ISSET(pGMACX->fd, &rd_set)) {
        return ;
    }

   	ssize_t len = read(pGMACX->fd, buf, sizeof(buf));

    loadDescriptor(pM, &desc, pGMACX->rx_desc);

#if defined(GMAC_DEBUG)
    if( ! (desc.ctrl & DESC_EMPTY) ){
        PRINTF("Rx buf unavailable\n");
    }
    PRINTF("RX Packet: ");
#endif

    for(int a = 0; a < len; a++){
        b = buf[a];
        storePhysMemByte(pM, desc.data+a, b);
#if defined(GMAC_DEBUG)
        PRINTF("%02x ", (uint8_t)b);
#endif
    }

#if defined(GMAC_DEBUG)
    PRINTF(" (%ld byte)\n", len);
#endif

    storePhysMemWord( pM, pGMACX->rx_desc+4,  len +4 ); // TODO: FCS
    pGMACX->rx_desc = desc.next;


    uint32_t count = ((pGMACX->rx_status>>16) & 0xff);
    count++;
    pGMACX->rx_status &= 0xff00ffff;
    pGMACX->rx_status |= ((count<<16) & 0x00ff0000);
    if( pGMACX->rx_status & 0x00ff0000 ){
        pGMACX->rx_status  |= 1;
        pGMACX->int_status |= (1<<GMACX_INT_STATUS_BIT_RXPKTRECEIVED);
    }else{
        pGMACX->rx_status  &= (~1);
        pGMACX->int_status &=~(1<<GMACX_INT_STATUS_BIT_RXPKTRECEIVED);
    }
}



void GMACX_performMiiWrite(struct stMachineState *pM, struct stIO_GMACX *pGMACX, uint32_t data){
    unsigned int regAddr = (((pGMACX->mii_addr)>>0) & 0x1f);

#if defined(GMAC_DEBUG)
    unsigned int phyAddr = (((pGMACX->mii_addr)>>8) & 0x1f);
    unsigned int control = (data & 0xffff);

    PRINTF("Write: Phy %d Addr %d Val 0x%x\n",phyAddr,regAddr, control);
#endif

    switch(regAddr){
        case 0:
            pGMACX->mii[0] = (data & 0xffff); break;
        case 1:
            pGMACX->mii[1] = (data & 0xffff); break;
        case 4:
            pGMACX->mii[4] = (data & 0xfc00); break;
        default:
            pGMACX->mii_status = 0; break;
    }
}

#define MII_BMSR_RO_PERM_VAL (BMSR_100FULL | BMSR_ANEGCAPABLE)

void GMACX_performMiiRead(struct stMachineState *pM, struct stIO_GMACX *pGMACX, uint32_t data){
    unsigned int phyAddr = (((pGMACX->mii_addr)>>8) & 0x1f);
    unsigned int regAddr = (((pGMACX->mii_addr)>>0) & 0x1f);

    switch(regAddr){
        case 0:
            // the reset bit is cleared in the result
            pGMACX->mii_status = (pGMACX->mii[0] & (~(0x8000|0x1200))); break;
        case 1:
            if( (pGMACX->mii[0]&BMCR_RESET) || (pGMACX->mii[0]&BMCR_PDOWN) ){
                pGMACX->mii_status = 0;
            }else if( (pGMACX->mii[0]&BMCR_ANENABLE) || (pGMACX->mii[0]&BMCR_ANRESTART) ){
                if( pGMACX->fd < 0 ){
                    // When TAP is not available
                    pGMACX->mii_status = (MII_BMSR_RO_PERM_VAL);
                }else{
                    pGMACX->mii_status = (MII_BMSR_RO_PERM_VAL | BMSR_LSTATUS | BMSR_ANEGCOMPLETE);
                }
            }else if( (pGMACX->mii[0]&0x2000) || (pGMACX->mii[0]&0x100) ){
                if( pGMACX->fd < 0 ){
                    // When TAP is not available
                    pGMACX->mii_status = (MII_BMSR_RO_PERM_VAL);
                }else{
                    pGMACX->mii_status = (MII_BMSR_RO_PERM_VAL | BMSR_LSTATUS);
                }
            }else{
                pGMACX->mii_status = (MII_BMSR_RO_PERM_VAL);
            }
            break;
        case 2:
            pGMACX->mii_status = 0x123; break;
        case 3:
            pGMACX->mii_status = 0x456; break;
        case 4:
            pGMACX->mii_status = (pGMACX->mii[4] | ADVERTISE_100HALF | ADVERTISE_100FULL | 0x01 ); break;
        case 5:
            pGMACX->mii_status = (ADVERTISE_100HALF | ADVERTISE_100FULL); break;
        default:
            pGMACX->mii_status = 0; break;
    }
}


uint32_t readGMACXReg(struct stMachineState *pM, struct stIO_GMACX *pGMACX, uint32_t addr){

    switch(addr & GMACX_ADDR_MASK ){
        case GMACX_MAC_CONFIG1_REG:
            return pGMACX->mac_config1;
        case GMACX_MAC_CONFIG2_REG:
            return pGMACX->mac_config2;
        case GMACX_IPG_IFG_REG: /* Unused in Linux */
            break;
        case GMACX_HALF_DUPLEX_REG: /* Unused in Linux */
            break;
        case GMACX_MAX_FRAME_LEN_REG: 
            return pGMACX->maxFrameLen;

        case GMACX_MII_CONFIG_REG:
            return pGMACX->mii_config;
        case GMACX_MII_CMD_REG:
            return pGMACX->mii_cmd;
        case GMACX_MII_ADDR_REG:
            return pGMACX->mii_addr;
        case GMACX_MII_CONTROL_REG: /* write only */
            return 0;
        case GMACX_MII_STATUS_REG:
            return pGMACX->mii_status;
        case GMACX_MII_INDICATOR_REG:
            return 0x00; // mii operation has been finished.

        case GMACX_INTERFACE_CONTROL_REG:
            return pGMACX->interfaceCtrl;
        case GMACX_INTERFACE_STATUS_REG:
            return ((1<<6) | (1<<5) | (1<<4));

        case GMACX_STA_ADDRESS1_REG:
            return pGMACX->sta_address[0];
        case GMACX_STA_ADDRESS2_REG:
            return pGMACX->sta_address[1];
        case GMACX_ETH_CONFIG0_REG:
            return pGMACX->eth_config[0];
        case GMACX_ETH_CONFIG1_REG:
            return pGMACX->eth_config[1];
        case GMACX_ETH_CONFIG2_REG:
            return pGMACX->eth_config[2];
        case GMACX_ETH_CONFIG3_REG:
            return pGMACX->eth_config[3];
        case GMACX_ETH_CONFIG4_REG:
            return pGMACX->eth_config[4];
        case GMACX_ETH_CONFIG5_REG:
            return pGMACX->eth_config[5];

        case GMACX_TX_CTRL_REG:
            return pGMACX->tx_ctrl;
        case GMACX_TX_DESC_REG:
            return pGMACX->tx_desc;
        case GMACX_TX_STATUS_REG:
            return pGMACX->tx_status;
        case GMACX_RX_CTRL_REG:
            return pGMACX->rx_ctrl;
        case GMACX_RX_DESC_REG:
            return pGMACX->rx_desc;
        case GMACX_RX_STATUS_REG:
            return pGMACX->rx_status;
        case GMACX_INT_ENABLE_REG:
            return pGMACX->int_enable;
        case GMACX_INT_STATUS_REG:
            return pGMACX->int_status;
        case GMACX_FIFO_DEPTH_REG:
            return pGMACX->fifo_depth;
        case GMACX_RX_SM_REG:
            break;
        case GMACX_TX_SM_REG:
            return 1;

        default:
            break;
    }

#if defined(GMAC_DEBUG)
    PRINTF("read GMAC0 register 0x%x\n", addr);
#endif

    return 0;
}

void writeGMACXReg(struct stMachineState *pM, struct stIO_GMACX *pGMACX, uint32_t addr, uint32_t data){

    switch( addr & GMACX_ADDR_MASK ){
        case GMACX_MAC_CONFIG1_REG:
            pGMACX->mac_config1 = data;
            break;
        case GMACX_MAC_CONFIG2_REG:
            pGMACX->mac_config2 = data;
            break;
        case GMACX_IPG_IFG_REG:
            break;
        case GMACX_HALF_DUPLEX_REG:
            break;
        case GMACX_MAX_FRAME_LEN_REG:
            pGMACX->maxFrameLen = data;
            return;

        case GMACX_MII_CONFIG_REG:
            pGMACX->mii_config = data;
            return;
        case GMACX_MII_CMD_REG:
            pGMACX->mii_cmd = data;
            GMACX_performMiiRead(pM, pGMACX, data);
            return;
        case GMACX_MII_ADDR_REG:
            pGMACX->mii_addr = data;
            return;
        case GMACX_MII_CONTROL_REG:
            GMACX_performMiiWrite(pM, pGMACX, data);
            return;
        case GMACX_MII_STATUS_REG:    /* read only */
        case GMACX_MII_INDICATOR_REG: /* read only */
            return;

        case GMACX_INTERFACE_CONTROL_REG:
            pGMACX->interfaceCtrl = data;
            break;
        case GMACX_INTERFACE_STATUS_REG: /* read only */
            break;

        case GMACX_STA_ADDRESS1_REG:
            pGMACX->sta_address[0] = data;
            return;
        case GMACX_STA_ADDRESS2_REG:
            pGMACX->sta_address[1] = data;
            return;
        case GMACX_ETH_CONFIG0_REG:
            pGMACX->eth_config[0] = data;
            break;
        case GMACX_ETH_CONFIG1_REG:
            pGMACX->eth_config[1] = data;
            break;
        case GMACX_ETH_CONFIG2_REG:
            pGMACX->eth_config[2] = data;
            break;
        case GMACX_ETH_CONFIG3_REG:
            pGMACX->eth_config[3] = data;
            break;
        case GMACX_ETH_CONFIG4_REG:
            pGMACX->eth_config[4] = data;
            break;
        case GMACX_ETH_CONFIG5_REG:
            pGMACX->eth_config[5] = data;
            break;

        case GMACX_TX_CTRL_REG:
            pGMACX->tx_ctrl = data;
#if defined(GMAC_DEBUG)
            if( data & 1 ) PRINTF("TX enabled\n");
#endif
            return ;
        case GMACX_TX_DESC_REG:
            pGMACX->tx_desc = data;
            return ;
        case GMACX_TX_STATUS_REG:
            if( data & 1 ){
                uint32_t count = ((pGMACX->tx_status>>16) & 0xff);
                if( count ) count--; // saturation counter
                pGMACX->tx_status &= 0xff00ffff;
                pGMACX->tx_status |= ((count<<16) & 0x00ff0000);
#if defined(GMAC_DEBUG)
                PRINTF("TX cnt decl\n");
#endif
            }
            if( pGMACX->tx_status & 0x00ff0000 ){
                pGMACX->tx_status  |= 1;
                pGMACX->int_status |= (1<<GMACX_INT_STATUS_BIT_TXPKTSENT);
            }else{
                pGMACX->tx_status  &= (~1);
                pGMACX->int_status &=~(1<<GMACX_INT_STATUS_BIT_TXPKTSENT);
            }
            return;
        case GMACX_RX_CTRL_REG:
            pGMACX->rx_ctrl = data;
#if defined(GMAC_DEBUG)
            if( data & 1 ) PRINTF("RX enabled\n");
#endif
            return ;
        case GMACX_RX_DESC_REG:
            pGMACX->rx_desc = data;
            return ;
        case GMACX_RX_STATUS_REG:
            if( data & 1 ){
                uint32_t count = ((pGMACX->rx_status>>16) & 0xff);
                if( count ) count--; // saturation counter
                pGMACX->rx_status &= 0xff00ffff;
                pGMACX->rx_status |= ((count<<16) & 0x00ff0000);
#if defined(GMAC_DEBUG)
                PRINTF("RX cnt decl\n");
#endif
            }
            if( pGMACX->rx_status & 0x00ff0000 ){
                pGMACX->rx_status  |= 1;
                pGMACX->int_status |= (1<<GMACX_INT_STATUS_BIT_RXPKTRECEIVED);
            }else{
                pGMACX->rx_status  &= (~1);
                pGMACX->int_status &=~(1<<GMACX_INT_STATUS_BIT_RXPKTRECEIVED);
            }
            return;
        case GMACX_INT_ENABLE_REG:
            pGMACX->int_enable = data;
            break;
        case GMACX_INT_STATUS_REG:
            pGMACX->int_status &= (~data); // TODO: uncertain
            break;
        case GMACX_FIFO_DEPTH_REG:
            pGMACX->fifo_depth = data;
            break;
        case GMACX_RX_SM_REG:
            break;
        case GMACX_TX_SM_REG:
            break;

        default:
            break;
    }

#if defined(GMAC_DEBUG)
    PRINTF("write GMAC0 register 0x%x val 0x%x\n", addr, data);
#endif
}
