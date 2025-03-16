/*


A part of this file is based on the following linux source code
drivers/net/ethernet/atheros/ag71xx.c

The license of the code is following.
*/
// SPDX-License-Identifier: GPL-2.0
/*  Atheros AR71xx built-in ethernet mac driver
 *
 *  Copyright (C) 2019 Oleksij Rempel <o.rempel@pengutronix.de>
 *
 *  List of authors contributed to this driver before mainlining:
 *  Alexander Couzens <lynxis@fe80.eu>
 *  Christian Lamparter <chunkeey@gmail.com>
 *  Chuanhong Guo <gch981213@gmail.com>
 *  Daniel F. Dickinson <cshored@thecshore.com>
 *  David Bauer <mail@david-bauer.net>
 *  Felix Fietkau <nbd@nbd.name>
 *  Gabor Juhos <juhosg@freemail.hu>
 *  Hauke Mehrtens <hauke@hauke-m.de>
 *  Johann Neuhauser <johann@it-neuhauser.de>
 *  John Crispin <john@phrozen.org>
 *  Jo-Philipp Wich <jo@mein.io>
 *  Koen Vandeputte <koen.vandeputte@ncentric.com>
 *  Lucian Cristian <lucian.cristian@gmail.com>
 *  Matt Merhar <mattmerhar@protonmail.com>
 *  Milan Krstic <milan.krstic@gmail.com>
 *  Petr Štetiar <ynezz@true.cz>
 *  Rosen Penev <rosenp@gmail.com>
 *  Stephen Walker <stephendwalker+github@gmail.com>
 *  Vittorio Gambaletta <openwrt@vittgam.net>
 *  Weijie Gao <hackpascal@gmail.com>
 *  Imre Kaloz <kaloz@openwrt.org>
 */

#ifndef __GMAC_H__
#define __GMAC_H__


#define DESC_EMPTY		(1<<31)
#define DESC_MORE		(1<<24)
#define DESC_PKTLEN_M		0xfff
struct ag71xx_desc {
	uint32_t data;
	uint32_t ctrl;
	uint32_t next;
	uint32_t pad;
};


// Base addresses
#define GMAC0_BASE_REG 0x19000000
#define GMAC1_BASE_REG 0x1A000000

#define GMACX_ADDR_MASK 0x1ff

// Offset addresses
#define GMACX_MAC_CONFIG1_REG       0x00
#define GMACX_MAC_CONFIG2_REG       0x04
#define GMACX_IPG_IFG_REG           0x08
#define GMACX_HALF_DUPLEX_REG       0x0C
#define GMACX_MAX_FRAME_LEN_REG     0x10

#define GMACX_MII_CONFIG_REG        0x20
#define GMACX_MII_CMD_REG           0x24
#define GMACX_MII_ADDR_REG          0x28
#define GMACX_MII_CONTROL_REG       0x2C
#define GMACX_MII_STATUS_REG        0x30
#define GMACX_MII_INDICATOR_REG     0x34
#define GMACX_INTERFACE_CONTROL_REG 0x38
#define GMACX_INTERFACE_STATUS_REG  0x3C

#define GMACX_STA_ADDRESS1_REG      0x40
#define GMACX_STA_ADDRESS2_REG      0x44

#define GMACX_ETH_CONFIG0_REG       0x48
#define GMACX_ETH_CONFIG1_REG       0x4C
#define GMACX_ETH_CONFIG2_REG       0x50
#define GMACX_ETH_CONFIG3_REG       0x54
#define GMACX_ETH_CONFIG4_REG       0x58
#define GMACX_ETH_CONFIG5_REG       0x5C

#define GMACX_TX_CTRL_REG           0x0180
#define GMACX_TX_DESC_REG           0x0184
#define GMACX_TX_STATUS_REG         0x0188
#define GMACX_RX_CTRL_REG           0x018c
#define GMACX_RX_DESC_REG           0x0190
#define GMACX_RX_STATUS_REG         0x0194
#define GMACX_INT_ENABLE_REG        0x0198
#define GMACX_INT_STATUS_REG        0x019c
#define GMACX_FIFO_DEPTH_REG        0x01a8
#define GMACX_RX_SM_REG             0x01b0
#define GMACX_TX_SM_REG             0x01b4

// Offset addresses for counters
#define GMACX_TR64_REG              0x80
#define GMACX_TR127_REG             0x84
#define GMACX_TR255_REG             0x88
#define GMACX_TR511_REG             0x8C
#define GMACX_TR1K_REG              0x90
#define GMACX_TRMAX_REG             0x94
#define GMACX_TRMGV_REG             0x98
#define GMACX_RBYT_REG              0x9C
#define GMACX_RPKT_REG              0xA0
#define GMACX_RFCS_REG              0xA4
#define GMACX_RMCA_REG              0xA8
#define GMACX_RBCA_REG              0xAC
#define GMACX_RXCF_REG              0xB0
#define GMACX_RXPF_REG              0xB4
#define GMACX_RXUO_REG              0xB8
#define GMACX_RALN_REG              0xBC
#define GMACX_RFLR_REG              0xC0
#define GMACX_RCDE_REG              0xC4
#define GMACX_RCSE_REG              0xC8
#define GMACX_RUND_REG              0xCC
#define GMACX_ROVR_REG              0xD0
#define GMACX_RFRG_REG              0xD4
#define GMACX_RJBR_REG              0xD8
#define GMACX_RDRP_REG              0xDC
#define GMACX_TBYT_REG              0xE0
#define GMACX_TPKT_REG              0xE4
#define GMACX_TMCA_REG              0xE8
#define GMACX_TBCA_REG              0xEC
#define GMACX_TXPF_REG              0xF0
#define GMACX_TDFR_REG              0xF4
#define GMACX_TEDF_REG              0xF8
#define GMACX_TSCL_REG              0xFC

void setGMACXTAPIFName(struct stMachineState *pM, struct stIO_GMACX *pGMACX, char *ifname);
int      initGMACX   (struct stMachineState *pM, struct stIO_GMACX *pGMACX);
void   removeGMACX   (struct stMachineState *pM, struct stIO_GMACX *pGMACX);
void   updateGMACX   (struct stMachineState *pM, struct stIO_GMACX *pGMACX);
uint32_t readGMACXReg(struct stMachineState *pM, struct stIO_GMACX *pGMACX, uint32_t addr);
void    writeGMACXReg(struct stMachineState *pM, struct stIO_GMACX *pGMACX, uint32_t addr, uint32_t data);

#endif
