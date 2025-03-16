/*
 * Qualcomm/Atheros Wireless SOC common registers definitions
 *
 * Copyright (C) 2016 Piotr Dymacz <piotr@dymacz.pl>
 * Copyright (C) 2014 Qualcomm Atheros, Inc.
 * Copyright (C) 2008-2010 Atheros Communications Inc.
 *
 * Partially based on:
 * Linux/arch/mips/include/asm/mach-ath79/ar71xx_regs.h
 * u-boot/include/soc/qca_soc_common.h
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef __DEV_ATH79_H__ 
#define __DEV_ATH79_H__ 

#define APB_BASE_REG            0x18000000

#define USB_CFG_BASE_REG        (APB_BASE_REG + 0x00030000)
#define GPIO_BASE_REG           (APB_BASE_REG + 0x00040000)
#define PLL_BASE_REG            (APB_BASE_REG + 0x00050000)
#define RST_BASE_REG            (APB_BASE_REG + 0x00060000)
#define GMAC_BASE_REG           (APB_BASE_REG + 0x00070000)
#define RTC_BASE_REG            (APB_BASE_REG + 0x00107000)
#define PLL_SRIF_BASE_REG       (APB_BASE_REG + 0x00116000)
#define PCIE_RC0_CTRL_BASE_REG  (APB_BASE_REG + 0x000F0000)
#define PCIE_RC1_CTRL_BASE_REG  (APB_BASE_REG + 0x00280000)

#define RST_MISC_INTERRUPT_STATUS_REG    (RST_BASE_REG + 0x10)
#define RST_MISC_INTERRUPT_MASK_REG      (RST_BASE_REG + 0x14)
#define RST_GLOBALINTERRUPT_STATUS_REG   (RST_BASE_REG + 0x18)
#define RST_RESET_REG                    (RST_BASE_REG + 0x1C)

#define RST_BOOTSTRAP_REG                (RST_BASE_REG + 0xB0)
#define RST_REVISION_ID_REG              (RST_BASE_REG + 0x90)
#define RST_REVISION_ID_MAJOR_AR9342_VAL 0x1120

#define PLL_CPU_DDR_CLK_CTRL_REG         (PLL_BASE_REG + 0x08)

#define PLL_SRIF_CPU_DPLL_BASE_REG       (PLL_SRIF_BASE_REG + 0x1C0)
#define PLL_SRIF_CPU_DPLL1_REG           (PLL_SRIF_CPU_DPLL_BASE_REG + 0x0)
#define PLL_SRIF_CPU_DPLL2_REG           (PLL_SRIF_CPU_DPLL_BASE_REG + 0x4)


uint32_t readMiscIntStatusReg(struct stMachineState *pM);

#endif
