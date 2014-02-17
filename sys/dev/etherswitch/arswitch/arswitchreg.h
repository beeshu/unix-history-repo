/*-
 * Copyright (c) 2011 Aleksandr Rybalko.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef __AR8X16_SWITCHREG_H__
#define	__AR8X16_SWITCHREG_H__

/* XXX doesn't belong here; stolen shamelessly from ath_hal/ah_internal.h */
/*
 * Register manipulation macros that expect bit field defines
 * to follow the convention that an _S suffix is appended for
 * a shift count, while the field mask has no suffix.
 */
#define	SM(_v, _f)	(((_v) << _f##_S) & (_f))
#define	MS(_v, _f)	(((_v) & (_f)) >> _f##_S)

/* Atheros specific MII registers */
#define	MII_ATH_MMD_ADDR		0x0d
#define	MII_ATH_MMD_DATA		0x0e
#define	MII_ATH_DBG_ADDR		0x1d
#define	MII_ATH_DBG_DATA		0x1e

#define	AR8X16_REG_MASK_CTRL		0x0000
#define		AR8X16_MASK_CTRL_REV_MASK	0x000000ff
#define		AR8X16_MASK_CTRL_VER_MASK	0x0000ff00
#define		AR8X16_MASK_CTRL_VER_SHIFT	8
#define		AR8X16_MASK_CTRL_SOFT_RESET	(1U << 31)

#define	AR8X16_REG_MODE			0x0008
/* DIR-615 E4 U-Boot */
#define		AR8X16_MODE_DIR_615_UBOOT	0x8d1003e0
/* From Ubiquiti RSPRO */
#define		AR8X16_MODE_RGMII_PORT4_ISO	0x81461bea
#define		AR8X16_MODE_RGMII_PORT4_SWITCH	0x01261be2
/* AVM Fritz!Box 7390 */
#define		AR8X16_MODE_GMII		0x010e5b71
/* from avm_cpmac/linux_ar_reg.h */
#define		AR8X16_MODE_RESERVED		0x000e1b20
#define		AR8X16_MODE_MAC0_GMII_EN	(1u <<  0)
#define		AR8X16_MODE_MAC0_RGMII_EN	(1u <<  1)
#define		AR8X16_MODE_PHY4_GMII_EN	(1u <<  2)
#define		AR8X16_MODE_PHY4_RGMII_EN	(1u <<  3)
#define		AR8X16_MODE_MAC0_MAC_MODE	(1u <<  4)
#define		AR8X16_MODE_RGMII_RXCLK_DELAY_EN (1u <<  6)
#define		AR8X16_MODE_RGMII_TXCLK_DELAY_EN (1u <<  7)
#define		AR8X16_MODE_MAC5_MAC_MODE	(1u << 14)
#define		AR8X16_MODE_MAC5_PHY_MODE	(1u << 15)
#define		AR8X16_MODE_TXDELAY_S0		(1u << 21)
#define		AR8X16_MODE_TXDELAY_S1		(1u << 22)
#define		AR8X16_MODE_RXDELAY_S0		(1u << 23)
#define		AR8X16_MODE_LED_OPEN_EN		(1u << 24)
#define		AR8X16_MODE_SPI_EN		(1u << 25)
#define		AR8X16_MODE_RXDELAY_S1		(1u << 26)
#define		AR8X16_MODE_POWER_ON_SEL	(1u << 31)

#define	AR8X16_REG_ISR			0x0010
#define	AR8X16_REG_IMR			0x0014

#define	AR8X16_REG_SW_MAC_ADDR0		0x0020
#define	AR8X16_REG_SW_MAC_ADDR1		0x0024

#define	AR8X16_REG_FLOOD_MASK		0x002c
#define		AR8X16_FLOOD_MASK_BCAST_TO_CPU	(1 << 26)

#define	AR8X16_REG_GLOBAL_CTRL		0x0030
#define		AR8216_GLOBAL_CTRL_MTU_MASK	0x00000fff
#define		AR8216_GLOBAL_CTRL_MTU_MASK_S	0
#define		AR8316_GLOBAL_CTRL_MTU_MASK	0x00007fff
#define		AR8316_GLOBAL_CTRL_MTU_MASK_S	0
#define		AR8236_GLOBAL_CTRL_MTU_MASK	0x00007fff
#define		AR8236_GLOBAL_CTRL_MTU_MASK_S	0
#define		AR7240_GLOBAL_CTRL_MTU_MASK	0x00003fff
#define		AR7240_GLOBAL_CTRL_MTU_MASK_S	0

#define	AR8X16_REG_VLAN_CTRL			0x0040
#define		AR8X16_VLAN_OP			0x00000007
#define		AR8X16_VLAN_OP_NOOP		0x0
#define		AR8X16_VLAN_OP_FLUSH		0x1
#define		AR8X16_VLAN_OP_LOAD		0x2
#define		AR8X16_VLAN_OP_PURGE		0x3
#define		AR8X16_VLAN_OP_REMOVE_PORT	0x4
#define		AR8X16_VLAN_OP_GET_NEXT		0x5
#define		AR8X16_VLAN_OP_GET		0x6
#define		AR8X16_VLAN_ACTIVE		(1 << 3)
#define		AR8X16_VLAN_FULL		(1 << 4)
#define		AR8X16_VLAN_PORT		0x00000f00
#define		AR8X16_VLAN_PORT_SHIFT		8
#define		AR8X16_VLAN_VID			0x0fff0000
#define		AR8X16_VLAN_VID_SHIFT		16
#define		AR8X16_VLAN_PRIO		0x70000000
#define		AR8X16_VLAN_PRIO_SHIFT		28
#define		AR8X16_VLAN_PRIO_EN		(1U << 31)

#define	AR8X16_REG_VLAN_DATA		0x0044
#define		AR8X16_VLAN_MEMBER		0x0000003f
#define		AR8X16_VLAN_VALID		(1 << 11)

#define	AR8X16_REG_ARL_CTRL0		0x0050
#define	AR8X16_REG_ARL_CTRL1		0x0054
#define	AR8X16_REG_ARL_CTRL2		0x0058

#define	AR8X16_REG_AT_CTRL		0x005c
#define		AR8X16_AT_CTRL_ARP_EN		(1 << 20)

#define	AR8X16_REG_IP_PRIORITY_1     	0x0060
#define	AR8X16_REG_IP_PRIORITY_2     	0x0064
#define	AR8X16_REG_IP_PRIORITY_3     	0x0068
#define	AR8X16_REG_IP_PRIORITY_4     	0x006C

#define	AR8X16_REG_TAG_PRIO		0x0070

#define	AR8X16_REG_SERVICE_TAG		0x0074
#define		AR8X16_SERVICE_TAG_MASK		0x0000ffff

#define	AR8X16_REG_CPU_PORT		0x0078
#define		AR8X16_MIRROR_PORT_SHIFT	4
#define		AR8X16_MIRROR_PORT_MASK		(0xf << AR8X16_MIRROR_PORT_SHIFT)
#define		AR8X16_CPU_MIRROR_PORT(_p)	((_p) << AR8X16_MIRROR_PORT_SHIFT)
#define		AR8X16_CPU_MIRROR_DIS		AR8X16_CPU_MIRROR_PORT(0xf)
#define		AR8X16_CPU_PORT_EN		(1 << 8)

#define	AR8X16_REG_MIB_FUNC0		0x0080
#define		AR8X16_MIB_TIMER_MASK		0x0000ffff
#define		AR8X16_MIB_AT_HALF_EN		(1 << 16)
#define		AR8X16_MIB_BUSY			(1 << 17)
#define		AR8X16_MIB_FUNC_SHIFT		24
#define		AR8X16_MIB_FUNC_NO_OP		0x0
#define		AR8X16_MIB_FUNC_FLUSH		0x1
#define		AR8X16_MIB_FUNC_CAPTURE		0x3
#define		AR8X16_MIB_FUNC_XXX		(1 << 30) /* 0x40000000 */

#define		AR934X_MIB_ENABLE		(1 << 30)

#define	AR8X16_REG_MDIO_HIGH_ADDR	0x0094

#define	AR8X16_REG_MDIO_CTRL		0x0098
#define		AR8X16_MDIO_CTRL_DATA_MASK	0x0000ffff
#define		AR8X16_MDIO_CTRL_REG_ADDR_SHIFT	16
#define		AR8X16_MDIO_CTRL_PHY_ADDR_SHIFT	21
#define		AR8X16_MDIO_CTRL_CMD_WRITE	0
#define		AR8X16_MDIO_CTRL_CMD_READ	(1 << 27)
#define		AR8X16_MDIO_CTRL_MASTER_EN	(1 << 30)
#define		AR8X16_MDIO_CTRL_BUSY		(1U << 31)

#define	AR8X16_REG_PORT_BASE(_p)	(0x0100 + (_p) * 0x0100)

#define	AR8X16_REG_PORT_STS(_p)		(AR8X16_REG_PORT_BASE((_p)) + 0x0000)
#define		AR8X16_PORT_STS_SPEED_MASK	0x00000003
#define		AR8X16_PORT_STS_SPEED_10	0
#define		AR8X16_PORT_STS_SPEED_100	1
#define		AR8X16_PORT_STS_SPEED_1000	2
#define		AR8X16_PORT_STS_TXMAC		(1 << 2)
#define		AR8X16_PORT_STS_RXMAC		(1 << 3)
#define		AR8X16_PORT_STS_TXFLOW		(1 << 4)
#define		AR8X16_PORT_STS_RXFLOW		(1 << 5)
#define		AR8X16_PORT_STS_DUPLEX		(1 << 6)
#define		AR8X16_PORT_STS_LINK_UP		(1 << 8)
#define		AR8X16_PORT_STS_LINK_AUTO	(1 << 9)
#define		AR8X16_PORT_STS_LINK_PAUSE	(1 << 10)

#define	AR8X16_REG_PORT_CTRL(_p)	(AR8X16_REG_PORT_BASE((_p)) + 0x0004)
#define		AR8X16_PORT_CTRL_STATE_MASK	0x00000007
#define		AR8X16_PORT_CTRL_STATE_DISABLED	0
#define		AR8X16_PORT_CTRL_STATE_BLOCK	1
#define		AR8X16_PORT_CTRL_STATE_LISTEN	2
#define		AR8X16_PORT_CTRL_STATE_LEARN	3
#define		AR8X16_PORT_CTRL_STATE_FORWARD	4
#define		AR8X16_PORT_CTRL_LEARN_LOCK	(1 << 7)
#define		AR8X16_PORT_CTRL_EGRESS_VLAN_MODE_SHIFT 8
#define		AR8X16_PORT_CTRL_EGRESS_VLAN_MODE_KEEP	0
#define		AR8X16_PORT_CTRL_EGRESS_VLAN_MODE_STRIP 1
#define		AR8X16_PORT_CTRL_EGRESS_VLAN_MODE_ADD 2
#define		AR8X16_PORT_CTRL_EGRESS_VLAN_MODE_DOUBLE_TAG 3
#define		AR8X16_PORT_CTRL_IGMP_SNOOP	(1 << 10)
#define		AR8X16_PORT_CTRL_HEADER		(1 << 11)
#define		AR8X16_PORT_CTRL_MAC_LOOP	(1 << 12)
#define		AR8X16_PORT_CTRL_SINGLE_VLAN	(1 << 13)
#define		AR8X16_PORT_CTRL_LEARN		(1 << 14)
#define		AR8X16_PORT_CTRL_DOUBLE_TAG	(1 << 15)
#define		AR8X16_PORT_CTRL_MIRROR_TX	(1 << 16)
#define		AR8X16_PORT_CTRL_MIRROR_RX	(1 << 17)

#define	AR8X16_REG_PORT_VLAN(_p)	(AR8X16_REG_PORT_BASE((_p)) + 0x0008)

#define		AR8X16_PORT_VLAN_DEFAULT_ID_SHIFT	0
#define		AR8X16_PORT_VLAN_DEST_PORTS_SHIFT	16
#define		AR8X16_PORT_VLAN_MODE_MASK		0xc0000000
#define		AR8X16_PORT_VLAN_MODE_SHIFT		30
#define		AR8X16_PORT_VLAN_MODE_PORT_ONLY		0
#define		AR8X16_PORT_VLAN_MODE_PORT_FALLBACK	1
#define		AR8X16_PORT_VLAN_MODE_VLAN_ONLY		2
#define		AR8X16_PORT_VLAN_MODE_SECURE		3

#define	AR8X16_REG_PORT_RATE_LIM(_p)	(AR8X16_REG_PORT_BASE((_p)) + 0x000c)
#define		AR8X16_PORT_RATE_LIM_128KB	0
#define		AR8X16_PORT_RATE_LIM_256KB	1
#define		AR8X16_PORT_RATE_LIM_512KB	2
#define		AR8X16_PORT_RATE_LIM_1MB	3
#define		AR8X16_PORT_RATE_LIM_2MB	4
#define		AR8X16_PORT_RATE_LIM_4MB	5
#define		AR8X16_PORT_RATE_LIM_8MB	6
#define		AR8X16_PORT_RATE_LIM_16MB	7
#define		AR8X16_PORT_RATE_LIM_32MB	8
#define		AR8X16_PORT_RATE_LIM_64MB	9
#define		AR8X16_PORT_RATE_LIM_IN_EN	(1 << 24)
#define		AR8X16_PORT_RATE_LIM_OUT_EN	(1 << 23)
#define		AR8X16_PORT_RATE_LIM_IN_MASK	0x000f0000
#define		AR8X16_PORT_RATE_LIM_IN_SHIFT	16
#define		AR8X16_PORT_RATE_LIM_OUT_MASK	0x0000000f
#define		AR8X16_PORT_RATE_LIM_OUT_SHIFT	0

#define	AR8X16_REG_PORT_PRIORITY(_p)	(AR8X16_REG_PORT_BASE((_p)) + 0x0010)

#define	AR8X16_REG_STATS_BASE(_p)	(0x20000 + (_p) * 0x100)

#define	AR8X16_STATS_RXBROAD		0x0000
#define	AR8X16_STATS_RXPAUSE		0x0004
#define	AR8X16_STATS_RXMULTI		0x0008
#define	AR8X16_STATS_RXFCSERR		0x000c
#define	AR8X16_STATS_RXALIGNERR		0x0010
#define	AR8X16_STATS_RXRUNT		0x0014
#define	AR8X16_STATS_RXFRAGMENT		0x0018
#define	AR8X16_STATS_RX64BYTE		0x001c
#define	AR8X16_STATS_RX128BYTE		0x0020
#define	AR8X16_STATS_RX256BYTE		0x0024
#define	AR8X16_STATS_RX512BYTE		0x0028
#define	AR8X16_STATS_RX1024BYTE		0x002c
#define	AR8X16_STATS_RX1518BYTE		0x0030
#define	AR8X16_STATS_RXMAXBYTE		0x0034
#define	AR8X16_STATS_RXTOOLONG		0x0038
#define	AR8X16_STATS_RXGOODBYTE		0x003c
#define	AR8X16_STATS_RXBADBYTE		0x0044
#define	AR8X16_STATS_RXOVERFLOW		0x004c
#define	AR8X16_STATS_FILTERED		0x0050
#define	AR8X16_STATS_TXBROAD		0x0054
#define	AR8X16_STATS_TXPAUSE		0x0058
#define	AR8X16_STATS_TXMULTI		0x005c
#define	AR8X16_STATS_TXUNDERRUN		0x0060
#define	AR8X16_STATS_TX64BYTE		0x0064
#define	AR8X16_STATS_TX128BYTE		0x0068
#define	AR8X16_STATS_TX256BYTE		0x006c
#define	AR8X16_STATS_TX512BYTE		0x0070
#define	AR8X16_STATS_TX1024BYTE		0x0074
#define	AR8X16_STATS_TX1518BYTE		0x0078
#define	AR8X16_STATS_TXMAXBYTE		0x007c
#define	AR8X16_STATS_TXOVERSIZE		0x0080
#define	AR8X16_STATS_TXBYTE		0x0084
#define	AR8X16_STATS_TXCOLLISION	0x008c
#define	AR8X16_STATS_TXABORTCOL		0x0090
#define	AR8X16_STATS_TXMULTICOL		0x0094
#define	AR8X16_STATS_TXSINGLECOL	0x0098
#define	AR8X16_STATS_TXEXCDEFER		0x009c
#define	AR8X16_STATS_TXDEFER		0x00a0
#define	AR8X16_STATS_TXLATECOL		0x00a4

#define	AR8X16_PORT_CPU			0
#define	AR8X16_NUM_PORTS		6
#define	AR8X16_NUM_PHYS			5
#define	AR8X16_MAGIC			0xc000050e

#define	AR8X16_PHY_ID1			0x004d
#define	AR8X16_PHY_ID2			0xd041

#define	AR8X16_PORT_MASK(_port)		(1 << (_port))
#define	AR8X16_PORT_MASK_ALL		((1<<AR8X16_NUM_PORTS)-1)
#define	AR8X16_PORT_MASK_BUT(_port)	(AR8X16_PORT_MASK_ALL & ~(1 << (_port)))

#define	AR8X16_MAX_VLANS		16

/*
 * AR9340 switch specific definitions.
 */

/* XXX Linux define compatibility stuff */
#define	BITM(_count)			((1 << _count) - 1)
#define	BITS(_shift, _count)		(BITM(_count) << _shift)

#define	AR934X_REG_OPER_MODE0		0x04
#define		AR934X_OPER_MODE0_MAC_GMII_EN	(1 << 6)
#define		AR934X_OPER_MODE0_PHY_MII_EN	(1 << 10)

#define	AR934X_REG_OPER_MODE1		0x08
#define		AR934X_REG_OPER_MODE1_PHY4_MII_EN	(1 << 28)

#define	AR934X_REG_FLOOD_MASK		0x2c
#define		AR934X_FLOOD_MASK_MC_DP(_p)	(1 << (16 + (_p)))
#define		AR934X_FLOOD_MASK_BC_DP(_p)	(1 << (25 + (_p)))

#define	AR934X_REG_QM_CTRL		0x3c
#define		AR934X_QM_CTRL_ARP_EN	(1 << 15)

#define	AR934X_REG_AT_CTRL		0x5c
#define		AR934X_AT_CTRL_AGE_TIME		BITS(0, 15)
#define		AR934X_AT_CTRL_AGE_EN		(1 << 17)
#define		AR934X_AT_CTRL_LEARN_CHANGE	(1 << 18)

#define	AR934X_REG_PORT_BASE(_port)	(0x100 + (_port) * 0x100)

#define	AR934X_REG_PORT_VLAN1(_port)	(AR934X_REG_PORT_BASE((_port)) + 0x08)
#define		AR934X_PORT_VLAN1_DEFAULT_SVID_S		0
#define		AR934X_PORT_VLAN1_FORCE_DEFAULT_VID_EN		(1 << 12)
#define		AR934X_PORT_VLAN1_PORT_TLS_MODE			(1 << 13)
#define		AR934X_PORT_VLAN1_PORT_VLAN_PROP_EN		(1 << 14)
#define		AR934X_PORT_VLAN1_PORT_CLONE_EN			(1 << 15)
#define		AR934X_PORT_VLAN1_DEFAULT_CVID_S		16
#define		AR934X_PORT_VLAN1_FORCE_PORT_VLAN_EN		(1 << 28)
#define		AR934X_PORT_VLAN1_ING_PORT_PRI_S		29

#define	AR934X_REG_PORT_VLAN2(_port)	(AR934X_REG_PORT_BASE((_port)) + 0x0c)
#define		AR934X_PORT_VLAN2_PORT_VID_MEM_S		16
#define		AR934X_PORT_VLAN2_8021Q_MODE_S			30
#define		AR934X_PORT_VLAN2_8021Q_MODE_PORT_ONLY		0
#define		AR934X_PORT_VLAN2_8021Q_MODE_PORT_FALLBACK	1
#define		AR934X_PORT_VLAN2_8021Q_MODE_VLAN_ONLY		2
#define		AR934X_PORT_VLAN2_8021Q_MODE_SECURE		3

/*
 * AR8327 specific registers
 */
#define	AR8327_NUM_PORTS		7
#define	AR8327_NUM_PHYS			5
#define	AR8327_PORTS_ALL		0x7f

#define	AR8327_REG_MASK			0x000

#define	AR8327_REG_PAD0_MODE		0x004
#define	AR8327_REG_PAD5_MODE		0x008
#define	AR8327_REG_PAD6_MODE		0x00c

#define		AR8327_PAD_MAC_MII_RXCLK_SEL	(1 << 0)
#define		AR8327_PAD_MAC_MII_TXCLK_SEL	(1 << 1)
#define		AR8327_PAD_MAC_MII_EN		(1 << 2)
#define		AR8327_PAD_MAC_GMII_RXCLK_SEL	(1 << 4)
#define		AR8327_PAD_MAC_GMII_TXCLK_SEL	(1 << 5)
#define		AR8327_PAD_MAC_GMII_EN		(1 << 6)
#define		AR8327_PAD_SGMII_EN		(1 << 7)
#define		AR8327_PAD_PHY_MII_RXCLK_SEL	(1 << 8)
#define		AR8327_PAD_PHY_MII_TXCLK_SEL	(1 << 9)
#define		AR8327_PAD_PHY_MII_EN		(1 << 10)
#define		AR8327_PAD_PHY_GMII_PIPE_RXCLK_SEL	(1 << 11)
#define		AR8327_PAD_PHY_GMII_RXCLK_SEL	(1 << 12)
#define		AR8327_PAD_PHY_GMII_TXCLK_SEL	(1 << 13)
#define		AR8327_PAD_PHY_GMII_EN		(1 << 14)
#define		AR8327_PAD_PHYX_GMII_EN		(1 << 16)
#define		AR8327_PAD_PHYX_RGMII_EN	(1 << 17)
#define		AR8327_PAD_PHYX_MII_EN		(1 << 18)
#define		AR8327_PAD_SGMII_DELAY_EN	(1 << 19)
#define		AR8327_PAD_RGMII_RXCLK_DELAY_SEL	BITS(20, 2)
#define		AR8327_PAD_RGMII_RXCLK_DELAY_SEL_S		20
#define		AR8327_PAD_RGMII_TXCLK_DELAY_SEL	BITS(22, 2)
#define		AR8327_PAD_RGMII_TXCLK_DELAY_SEL_S	22
#define		AR8327_PAD_RGMII_RXCLK_DELAY_EN	(1 << 24)
#define		AR8327_PAD_RGMII_TXCLK_DELAY_EN	(1 << 25)
#define		AR8327_PAD_RGMII_EN		(1 << 26)

#define	AR8327_REG_POWER_ON_STRIP	0x010
#define		AR8327_POWER_ON_STRIP_POWER_ON_SEL	(1U << 31)
#define		AR8327_POWER_ON_STRIP_LED_OPEN_EN	(1 << 24)
#define		AR8327_POWER_ON_STRIP_SERDES_AEN	(1 << 7)

#define	AR8327_REG_INT_STATUS0		0x020
#define		AR8327_INT0_VT_DONE			(1 << 20)

#define	AR8327_REG_INT_STATUS1		0x024
#define	AR8327_REG_INT_MASK0		0x028
#define	AR8327_REG_INT_MASK1		0x02c

#define	AR8327_REG_MODULE_EN		0x030
#define		AR8327_MODULE_EN_MIB		(1 << 0)

#define	AR8327_REG_MIB_FUNC		0x034
#define		AR8327_MIB_CPU_KEEP		(1 << 20)

#define	AR8327_REG_MDIO_CTRL		0x03c

#define	AR8327_REG_SERVICE_TAG		0x048
#define	AR8327_REG_LED_CTRL0		0x050
#define	AR8327_REG_LED_CTRL1		0x054
#define	AR8327_REG_LED_CTRL2		0x058
#define	AR8327_REG_LED_CTRL3		0x05c
#define	AR8327_REG_MAC_ADDR0		0x060
#define	AR8327_REG_MAC_ADDR1		0x064

#define	AR8327_REG_MAX_FRAME_SIZE	0x078
#define		AR8327_MAX_FRAME_SIZE_MTU	BITS(0, 14)

#define	AR8327_REG_PORT_STATUS(_i)	(0x07c + (_i) * 4)

#define	AR8327_REG_HEADER_CTRL		0x098
#define	AR8327_REG_PORT_HEADER(_i)		(0x09c + (_i) * 4)

#define	AR8327_REG_SGMII_CTRL		0x0e0
#define		AR8327_SGMII_CTRL_EN_PLL		(1 << 1)
#define		AR8327_SGMII_CTRL_EN_RX			(1 << 2)
#define		AR8327_SGMII_CTRL_EN_TX			(1 << 3)

#define	AR8327_REG_PORT_VLAN0(_i)		(0x420 + (_i) * 0x8)
#define		AR8327_PORT_VLAN0_DEF_SVID		BITS(0, 12)
#define		AR8327_PORT_VLAN0_DEF_SVID_S		0
#define		AR8327_PORT_VLAN0_DEF_CVID		BITS(16, 12)
#define		AR8327_PORT_VLAN0_DEF_CVID_S		16

#define	AR8327_REG_PORT_VLAN1(_i)		(0x424 + (_i) * 0x8)
#define		AR8327_PORT_VLAN1_PORT_VLAN_PROP	(1 << 6)
#define		AR8327_PORT_VLAN1_OUT_MODE		BITS(12, 2)
#define		AR8327_PORT_VLAN1_OUT_MODE_S		12
#define		AR8327_PORT_VLAN1_OUT_MODE_UNMOD	0
#define		AR8327_PORT_VLAN1_OUT_MODE_UNTAG	1
#define		AR8327_PORT_VLAN1_OUT_MODE_TAG		2
#define		AR8327_PORT_VLAN1_OUT_MODE_UNTOUCH	3

#define	AR8327_REG_ATU_DATA0		0x600
#define	AR8327_REG_ATU_DATA1		0x604
#define	AR8327_REG_ATU_DATA2		0x608

#define	AR8327_REG_ATU_FUNC		0x60c
#define		AR8327_ATU_FUNC_OP		BITS(0, 4)
#define		AR8327_ATU_FUNC_OP_NOOP			0x0
#define		AR8327_ATU_FUNC_OP_FLUSH		0x1
#define		AR8327_ATU_FUNC_OP_LOAD			0x2
#define		AR8327_ATU_FUNC_OP_PURGE		0x3
#define		AR8327_ATU_FUNC_OP_FLUSH_LOCKED		0x4
#define		AR8327_ATU_FUNC_OP_FLUSH_UNICAST	0x5
#define		AR8327_ATU_FUNC_OP_GET_NEXT		0x6
#define		AR8327_ATU_FUNC_OP_SEARCH_MAC		0x7
#define		AR8327_ATU_FUNC_OP_CHANGE_TRUNK		0x8
#define		AR8327_ATU_FUNC_BUSY			(1U << 31)

#define	AR8327_REG_VTU_FUNC0		0x0610
#define		AR8327_VTU_FUNC0_EG_MODE	BITS(4, 14)
#define		AR8327_VTU_FUNC0_EG_MODE_S(_i)	(4 + (_i) * 2)
#define		AR8327_VTU_FUNC0_EG_MODE_KEEP	0
#define		AR8327_VTU_FUNC0_EG_MODE_UNTAG	1
#define		AR8327_VTU_FUNC0_EG_MODE_TAG	2
#define		AR8327_VTU_FUNC0_EG_MODE_NOT	3
#define		AR8327_VTU_FUNC0_IVL		(1 << 19)
#define		AR8327_VTU_FUNC0_VALID		(1 << 20)

#define	AR8327_REG_VTU_FUNC1		0x0614
#define		AR8327_VTU_FUNC1_OP		BITS(0, 3)
#define		AR8327_VTU_FUNC1_OP_NOOP	0
#define		AR8327_VTU_FUNC1_OP_FLUSH	1
#define		AR8327_VTU_FUNC1_OP_LOAD	2
#define		AR8327_VTU_FUNC1_OP_PURGE	3
#define		AR8327_VTU_FUNC1_OP_REMOVE_PORT	4
#define		AR8327_VTU_FUNC1_OP_GET_NEXT	5
#define		AR8327_VTU_FUNC1_OP_GET_ONE	6
#define		AR8327_VTU_FUNC1_FULL		(1 << 4)
#define		AR8327_VTU_FUNC1_PORT		(1 << 8, 4)
#define		AR8327_VTU_FUNC1_PORT_S		8
#define		AR8327_VTU_FUNC1_VID		(1 << 16, 12)
#define		AR8327_VTU_FUNC1_VID_S		16
#define		AR8327_VTU_FUNC1_BUSY		(1U << 31)

#define	AR8327_REG_FWD_CTRL0		0x620
#define		AR8327_FWD_CTRL0_CPU_PORT_EN	(1 << 10)
#define		AR8327_FWD_CTRL0_MIRROR_PORT	BITS(4, 4)
#define		AR8327_FWD_CTRL0_MIRROR_PORT_S	4

#define	AR8327_REG_FWD_CTRL1		0x624
#define		AR8327_FWD_CTRL1_UC_FLOOD	BITS(0, 7)
#define		AR8327_FWD_CTRL1_UC_FLOOD_S	0
#define		AR8327_FWD_CTRL1_MC_FLOOD	BITS(8, 7)
#define		AR8327_FWD_CTRL1_MC_FLOOD_S	8
#define		AR8327_FWD_CTRL1_BC_FLOOD	BITS(16, 7)
#define		AR8327_FWD_CTRL1_BC_FLOOD_S	16
#define		AR8327_FWD_CTRL1_IGMP		BITS(24, 7)
#define		AR8327_FWD_CTRL1_IGMP_S		24

#define	AR8327_REG_PORT_LOOKUP(_i)	(0x660 + (_i) * 0xc)
#define		AR8327_PORT_LOOKUP_MEMBER	BITS(0, 7)
#define		AR8327_PORT_LOOKUP_IN_MODE	BITS(8, 2)
#define		AR8327_PORT_LOOKUP_IN_MODE_S	8
#define		AR8327_PORT_LOOKUP_STATE	BITS(16, 3)
#define		AR8327_PORT_LOOKUP_STATE_S	16
#define		AR8327_PORT_LOOKUP_LEARN	(1 << 20)
#define		AR8327_PORT_LOOKUP_ING_MIRROR_EN	(1 << 25)

#define	AR8327_REG_PORT_PRIO(_i)	(0x664 + (_i) * 0xc)

#define	AR8327_REG_PORT_HOL_CTRL1(_i)		(0x974 + (_i) * 0x8)
#define		AR8327_PORT_HOL_CTRL1_EG_MIRROR_EN	(1 << 16)

#define	AR8327_REG_PORT_STATS_BASE(_i)		(0x1000 + (_i) * 0x100)

#endif /* __AR8X16_SWITCHREG_H__ */
