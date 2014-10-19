/*
 * Copyright (c) 2007, 2014 Mellanox Technologies. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/mlx4/driver.h>
#include <linux/in.h>
#include <net/ip.h>
#include <linux/bitmap.h>

#include "mlx4_en.h"
#include "en_port.h"

#define EN_ETHTOOL_QP_ATTACH (1ull << 63)

union mlx4_ethtool_flow_union {
	struct ethtool_tcpip4_spec		tcp_ip4_spec;
	struct ethtool_tcpip4_spec		udp_ip4_spec;
	struct ethtool_tcpip4_spec		sctp_ip4_spec;
	struct ethtool_ah_espip4_spec		ah_ip4_spec;
	struct ethtool_ah_espip4_spec		esp_ip4_spec;
	struct ethtool_usrip4_spec		usr_ip4_spec;
	struct ethhdr				ether_spec;
	__u8					hdata[52];
};

struct mlx4_ethtool_flow_ext {
	__u8		padding[2];
	unsigned char	h_dest[ETH_ALEN];
	__be16		vlan_etype;
	__be16		vlan_tci;
	__be32		data[2];
};

struct mlx4_ethtool_rx_flow_spec {
	__u32		flow_type;
	union mlx4_ethtool_flow_union h_u;
	struct mlx4_ethtool_flow_ext h_ext;
	union mlx4_ethtool_flow_union m_u;
	struct mlx4_ethtool_flow_ext m_ext;
	__u64		ring_cookie;
	__u32		location;
};

struct mlx4_ethtool_rxnfc {
	__u32				cmd;
	__u32				flow_type;
	__u64				data;
	struct mlx4_ethtool_rx_flow_spec	fs;
	__u32				rule_cnt;
	__u32				rule_locs[0];
};

#ifndef FLOW_MAC_EXT
#define	FLOW_MAC_EXT	0x40000000
#endif

static void
mlx4_en_get_drvinfo(struct net_device *dev, struct ethtool_drvinfo *drvinfo)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);
	struct mlx4_en_dev *mdev = priv->mdev;

	strlcpy(drvinfo->driver, DRV_NAME, sizeof(drvinfo->driver));
	strlcpy(drvinfo->version, DRV_VERSION " (" DRV_RELDATE ")",
		sizeof(drvinfo->version));
	snprintf(drvinfo->fw_version, sizeof(drvinfo->fw_version),
		"%d.%d.%d",
		(u16) (mdev->dev->caps.fw_ver >> 32),
		(u16) ((mdev->dev->caps.fw_ver >> 16) & 0xffff),
		(u16) (mdev->dev->caps.fw_ver & 0xffff));
	strlcpy(drvinfo->bus_info, pci_name(mdev->dev->pdev),
		sizeof(drvinfo->bus_info));
	drvinfo->n_stats = 0;
	drvinfo->regdump_len = 0;
	drvinfo->eedump_len = 0;
}

static const char main_strings[][ETH_GSTRING_LEN] = {
	/* packet statistics */
	"rx_packets",
	"rx_bytes",
	"rx_multicast_packets",
	"rx_broadcast_packets",
	"rx_errors",
	"rx_dropped",
	"rx_length_errors",
	"rx_over_errors",
	"rx_crc_errors",
	"rx_jabbers",
	"rx_in_range_length_error",
	"rx_out_range_length_error",
	"rx_lt_64_bytes_packets",
	"rx_127_bytes_packets",
	"rx_255_bytes_packets",
	"rx_511_bytes_packets",
	"rx_1023_bytes_packets",
	"rx_1518_bytes_packets",
	"rx_1522_bytes_packets",
	"rx_1548_bytes_packets",
	"rx_gt_1548_bytes_packets",
	"tx_packets",
	"tx_bytes",
	"tx_multicast_packets",
	"tx_broadcast_packets",
	"tx_errors",
	"tx_dropped",
	"tx_lt_64_bytes_packets",
	"tx_127_bytes_packets",
	"tx_255_bytes_packets",
	"tx_511_bytes_packets",
	"tx_1023_bytes_packets",
	"tx_1518_bytes_packets",
	"tx_1522_bytes_packets",
	"tx_1548_bytes_packets",
	"tx_gt_1548_bytes_packets",
	"rx_prio_0_packets", "rx_prio_0_bytes",
	"rx_prio_1_packets", "rx_prio_1_bytes",
	"rx_prio_2_packets", "rx_prio_2_bytes",
	"rx_prio_3_packets", "rx_prio_3_bytes",
	"rx_prio_4_packets", "rx_prio_4_bytes",
	"rx_prio_5_packets", "rx_prio_5_bytes",
	"rx_prio_6_packets", "rx_prio_6_bytes",
	"rx_prio_7_packets", "rx_prio_7_bytes",
	"rx_novlan_packets", "rx_novlan_bytes",
	"tx_prio_0_packets", "tx_prio_0_bytes",
	"tx_prio_1_packets", "tx_prio_1_bytes",
	"tx_prio_2_packets", "tx_prio_2_bytes",
	"tx_prio_3_packets", "tx_prio_3_bytes",
	"tx_prio_4_packets", "tx_prio_4_bytes",
	"tx_prio_5_packets", "tx_prio_5_bytes",
	"tx_prio_6_packets", "tx_prio_6_bytes",
	"tx_prio_7_packets", "tx_prio_7_bytes",
	"tx_novlan_packets", "tx_novlan_bytes",

	/* flow control statistics */
	"rx_pause_prio_0", "rx_pause_duration_prio_0",
	"rx_pause_transition_prio_0", "tx_pause_prio_0",
	"tx_pause_duration_prio_0", "tx_pause_transition_prio_0",
	"rx_pause_prio_1", "rx_pause_duration_prio_1",
	"rx_pause_transition_prio_1", "tx_pause_prio_1",
	"tx_pause_duration_prio_1", "tx_pause_transition_prio_1",
	"rx_pause_prio_2", "rx_pause_duration_prio_2",
	"rx_pause_transition_prio_2", "tx_pause_prio_2",
	"tx_pause_duration_prio_2", "tx_pause_transition_prio_2",
	"rx_pause_prio_3", "rx_pause_duration_prio_3",
	"rx_pause_transition_prio_3", "tx_pause_prio_3",
	"tx_pause_duration_prio_3", "tx_pause_transition_prio_3",
	"rx_pause_prio_4", "rx_pause_duration_prio_4",
	"rx_pause_transition_prio_4", "tx_pause_prio_4",
	"tx_pause_duration_prio_4", "tx_pause_transition_prio_4",
	"rx_pause_prio_5", "rx_pause_duration_prio_5",
	"rx_pause_transition_prio_5", "tx_pause_prio_5",
	"tx_pause_duration_prio_5", "tx_pause_transition_prio_5",
	"rx_pause_prio_6", "rx_pause_duration_prio_6",
	"rx_pause_transition_prio_6", "tx_pause_prio_6",
	"tx_pause_duration_prio_6", "tx_pause_transition_prio_6",
	"rx_pause_prio_7", "rx_pause_duration_prio_7",
	"rx_pause_transition_prio_7", "tx_pause_prio_7",
	"tx_pause_duration_prio_7", "tx_pause_transition_prio_7",

	/* VF statistics */
	"rx_packets",
	"rx_bytes",
	"rx_multicast_packets",
	"rx_broadcast_packets",
	"rx_errors",
	"rx_dropped",
	"tx_packets",
	"tx_bytes",
	"tx_multicast_packets",
	"tx_broadcast_packets",
	"tx_errors",

	/* VPort statistics */
	"vport_rx_unicast_packets",
	"vport_rx_unicast_bytes",
	"vport_rx_multicast_packets",
	"vport_rx_multicast_bytes",
	"vport_rx_broadcast_packets",
	"vport_rx_broadcast_bytes",
	"vport_rx_dropped",
	"vport_rx_errors",
	"vport_tx_unicast_packets",
	"vport_tx_unicast_bytes",
	"vport_tx_multicast_packets",
	"vport_tx_multicast_bytes",
	"vport_tx_broadcast_packets",
	"vport_tx_broadcast_bytes",
	"vport_tx_errors",

	/* port statistics */
	"tx_tso_packets",
	"tx_queue_stopped", "tx_wake_queue", "tx_timeout", "rx_alloc_failed",
	"rx_csum_good", "rx_csum_none", "tx_chksum_offload",
};

static const char mlx4_en_test_names[][ETH_GSTRING_LEN]= {
	"Interrupt Test",
	"Link Test",
	"Speed Test",
	"Register Test",
	"Loopback Test",
};

static u32 mlx4_en_get_msglevel(struct net_device *dev)
{
	return ((struct mlx4_en_priv *) netdev_priv(dev))->msg_enable;
}

static void mlx4_en_set_msglevel(struct net_device *dev, u32 val)
{
	((struct mlx4_en_priv *) netdev_priv(dev))->msg_enable = val;
}

static void mlx4_en_get_wol(struct net_device *netdev,
			    struct ethtool_wolinfo *wol)
{
	struct mlx4_en_priv *priv = netdev_priv(netdev);
	int err = 0;
	u64 config = 0;
	u64 mask;

	if ((priv->port < 1) || (priv->port > 2)) {
		en_err(priv, "Failed to get WoL information\n");
		return;
	}

	mask = (priv->port == 1) ? MLX4_DEV_CAP_FLAG_WOL_PORT1 :
		MLX4_DEV_CAP_FLAG_WOL_PORT2;

	if (!(priv->mdev->dev->caps.flags & mask)) {
		wol->supported = 0;
		wol->wolopts = 0;
		return;
	}

	err = mlx4_wol_read(priv->mdev->dev, &config, priv->port);
	if (err) {
		en_err(priv, "Failed to get WoL information\n");
		return;
	}

	if (config & MLX4_EN_WOL_MAGIC)
		wol->supported = WAKE_MAGIC;
	else
		wol->supported = 0;

	if (config & MLX4_EN_WOL_ENABLED)
		wol->wolopts = WAKE_MAGIC;
	else
		wol->wolopts = 0;
}

static int mlx4_en_set_wol(struct net_device *netdev,
			    struct ethtool_wolinfo *wol)
{
	struct mlx4_en_priv *priv = netdev_priv(netdev);
	u64 config = 0;
	int err = 0;
	u64 mask;

	if ((priv->port < 1) || (priv->port > 2))
		return -EOPNOTSUPP;

	mask = (priv->port == 1) ? MLX4_DEV_CAP_FLAG_WOL_PORT1 :
		MLX4_DEV_CAP_FLAG_WOL_PORT2;

	if (!(priv->mdev->dev->caps.flags & mask))
		return -EOPNOTSUPP;

	if (wol->supported & ~WAKE_MAGIC)
		return -EINVAL;

	err = mlx4_wol_read(priv->mdev->dev, &config, priv->port);
	if (err) {
		en_err(priv, "Failed to get WoL info, unable to modify\n");
		return err;
	}

	if (wol->wolopts & WAKE_MAGIC) {
		config |= MLX4_EN_WOL_DO_MODIFY | MLX4_EN_WOL_ENABLED |
				MLX4_EN_WOL_MAGIC;
	} else {
		config &= ~(MLX4_EN_WOL_ENABLED | MLX4_EN_WOL_MAGIC);
		config |= MLX4_EN_WOL_DO_MODIFY;
	}

	err = mlx4_wol_write(priv->mdev->dev, config, priv->port);
	if (err)
		en_err(priv, "Failed to set WoL information\n");

	return err;
}

struct bitmap_sim_iterator {
	bool advance_array;
	unsigned long *stats_bitmap;
	unsigned int count;
	unsigned int j;
};

static inline void bitmap_sim_iterator_init(struct bitmap_sim_iterator *h,
					    unsigned long *stats_bitmap,
					    int count)
{
	h->j = 0;
	h->advance_array = !bitmap_empty(stats_bitmap, count);
	h->count = h->advance_array ? bitmap_weight(stats_bitmap, count)
		: count;
	h->stats_bitmap = stats_bitmap;
}

static inline int bitmap_sim_iterator_test(struct bitmap_sim_iterator *h)
{
	return !h->advance_array ? 1 : test_bit(h->j, h->stats_bitmap);
}

static inline int bitmap_sim_iterator_inc(struct bitmap_sim_iterator *h)
{
	return h->j++;
}

static inline unsigned int bitmap_sim_iterator_count(
		struct bitmap_sim_iterator *h)
{
	return h->count;
}

int mlx4_en_get_sset_count(struct net_device *dev, int sset)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);
	struct bitmap_sim_iterator it;

	int num_of_stats = NUM_ALL_STATS -
		((priv->mdev->dev->caps.flags2 &
		 MLX4_DEV_CAP_FLAG2_FLOWSTATS_EN) ? 0 : NUM_FLOW_STATS);

	bitmap_sim_iterator_init(&it, priv->stats_bitmap, num_of_stats);

	switch (sset) {
	case ETH_SS_STATS:
		return bitmap_sim_iterator_count(&it) +
			(priv->tx_ring_num * 2) +
#ifdef LL_EXTENDED_STATS
			(priv->rx_ring_num * 5);
#else
			(priv->rx_ring_num * 2);
#endif
	case ETH_SS_TEST:
		return MLX4_EN_NUM_SELF_TEST - !(priv->mdev->dev->caps.flags
					& MLX4_DEV_CAP_FLAG_UC_LOOPBACK) * 2;
	default:
		return -EOPNOTSUPP;
	}
}

void mlx4_en_get_ethtool_stats(struct net_device *dev,
		struct ethtool_stats *stats, u64 *data)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);
	int index = 0;
	int i;
	struct bitmap_sim_iterator it;

	int num_of_stats = NUM_ALL_STATS -
		((priv->mdev->dev->caps.flags2 &
		 MLX4_DEV_CAP_FLAG2_FLOWSTATS_EN) ? 0 : NUM_FLOW_STATS);

	bitmap_sim_iterator_init(&it, priv->stats_bitmap, num_of_stats);

	if (!data || !priv->port_up)
		return;

	spin_lock_bh(&priv->stats_lock);

	for (i = 0; i < NUM_PKT_STATS; i++,
			bitmap_sim_iterator_inc(&it))
		if (bitmap_sim_iterator_test(&it))
			data[index++] =
				((unsigned long *)&priv->pkstats)[i];
	for (i = 0; i < NUM_FLOW_STATS; i++,
			bitmap_sim_iterator_inc(&it))
		if (priv->mdev->dev->caps.flags2 &
		    MLX4_DEV_CAP_FLAG2_FLOWSTATS_EN)
			if (bitmap_sim_iterator_test(&it))
				data[index++] =
					((u64 *)&priv->flowstats)[i];
	for (i = 0; i < NUM_VF_STATS; i++,
			bitmap_sim_iterator_inc(&it))
		if (bitmap_sim_iterator_test(&it))
			data[index++] =
				((unsigned long *)&priv->vf_stats)[i];
	for (i = 0; i < NUM_VPORT_STATS; i++,
			bitmap_sim_iterator_inc(&it))
		if (bitmap_sim_iterator_test(&it))
			data[index++] =
				((unsigned long *)&priv->vport_stats)[i];
	for (i = 0; i < NUM_PORT_STATS; i++,
			bitmap_sim_iterator_inc(&it))
		if (bitmap_sim_iterator_test(&it))
			data[index++] =
				((unsigned long *)&priv->port_stats)[i];

	for (i = 0; i < priv->tx_ring_num; i++) {
		data[index++] = priv->tx_ring[i]->packets;
		data[index++] = priv->tx_ring[i]->bytes;
	}
	for (i = 0; i < priv->rx_ring_num; i++) {
		data[index++] = priv->rx_ring[i]->packets;
		data[index++] = priv->rx_ring[i]->bytes;
#ifdef LL_EXTENDED_STATS
		data[index++] = priv->rx_ring[i]->yields;
		data[index++] = priv->rx_ring[i]->misses;
		data[index++] = priv->rx_ring[i]->cleaned;
#endif
	}
	spin_unlock_bh(&priv->stats_lock);

}

void mlx4_en_restore_ethtool_stats(struct mlx4_en_priv *priv, u64 *data)
{
	int index = 0;
	int i;
	struct bitmap_sim_iterator it;

	int num_of_stats = NUM_ALL_STATS -
		((priv->mdev->dev->caps.flags2 &
		 MLX4_DEV_CAP_FLAG2_FLOWSTATS_EN) ? 0 : NUM_FLOW_STATS);

	bitmap_sim_iterator_init(&it, priv->stats_bitmap, num_of_stats);

	if (!data || !priv->port_up)
		return;

	spin_lock_bh(&priv->stats_lock);

	for (i = 0; i < NUM_PKT_STATS; i++,
	      bitmap_sim_iterator_inc(&it))
			if (bitmap_sim_iterator_test(&it))
				((unsigned long *)&priv->pkstats)[i] =
					data[index++];
	for (i = 0; i < NUM_FLOW_STATS; i++,
	      bitmap_sim_iterator_inc(&it))
		if (priv->mdev->dev->caps.flags2 &
		    MLX4_DEV_CAP_FLAG2_FLOWSTATS_EN)
			if (bitmap_sim_iterator_test(&it))
				((u64 *)&priv->flowstats)[i] =
					 data[index++];
	for (i = 0; i < NUM_VF_STATS; i++,
	      bitmap_sim_iterator_inc(&it))
			if (bitmap_sim_iterator_test(&it))
				((unsigned long *)&priv->vf_stats)[i] =
					data[index++];
	for (i = 0; i < NUM_VPORT_STATS; i++,
	      bitmap_sim_iterator_inc(&it))
			if (bitmap_sim_iterator_test(&it))
				((unsigned long *)&priv->vport_stats)[i] =
					data[index++];
	for (i = 0; i < NUM_PORT_STATS; i++,
	      bitmap_sim_iterator_inc(&it))
			if (bitmap_sim_iterator_test(&it))
				((unsigned long *)&priv->port_stats)[i] =
					data[index++];

	for (i = 0; i < priv->tx_ring_num; i++) {
		priv->tx_ring[i]->packets = data[index++];
		priv->tx_ring[i]->bytes = data[index++];
	}
	for (i = 0; i < priv->rx_ring_num; i++) {
		priv->rx_ring[i]->packets = data[index++];
		priv->rx_ring[i]->bytes = data[index++];
	}
	spin_unlock_bh(&priv->stats_lock);
}

static void mlx4_en_self_test(struct net_device *dev,
			      struct ethtool_test *etest, u64 *buf)
{
	mlx4_en_ex_selftest(dev, &etest->flags, buf);
}

static void mlx4_en_get_strings(struct net_device *dev,
				uint32_t stringset, uint8_t *data)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);
	int index = 0;
	int i, k;
	struct bitmap_sim_iterator it;

	int num_of_stats = NUM_ALL_STATS -
		((priv->mdev->dev->caps.flags2 &
		 MLX4_DEV_CAP_FLAG2_FLOWSTATS_EN) ? 0 : NUM_FLOW_STATS);

	bitmap_sim_iterator_init(&it, priv->stats_bitmap, num_of_stats);

	switch (stringset) {
	case ETH_SS_TEST:
		for (i = 0; i < MLX4_EN_NUM_SELF_TEST - 2; i++)
			strcpy(data + i * ETH_GSTRING_LEN, mlx4_en_test_names[i]);
		if (priv->mdev->dev->caps.flags & MLX4_DEV_CAP_FLAG_UC_LOOPBACK)
			for (; i < MLX4_EN_NUM_SELF_TEST; i++)
				strcpy(data + i * ETH_GSTRING_LEN, mlx4_en_test_names[i]);
		break;

	case ETH_SS_STATS:
		/* Add main counters */
		for (i = 0; i < NUM_PKT_STATS; i++,
		     bitmap_sim_iterator_inc(&it))
			if (bitmap_sim_iterator_test(&it))
				strcpy(data + (index++) * ETH_GSTRING_LEN,
				       main_strings[i]);

		for (k = 0; k < NUM_FLOW_STATS; k++,
		     bitmap_sim_iterator_inc(&it))
			if (priv->mdev->dev->caps.flags2 &
			    MLX4_DEV_CAP_FLAG2_FLOWSTATS_EN)
				if (bitmap_sim_iterator_test(&it))
					strcpy(data + (index++) *
					       ETH_GSTRING_LEN,
						main_strings[i + k]);

		for (; (i + k) < num_of_stats; i++,
		     bitmap_sim_iterator_inc(&it))
			if (bitmap_sim_iterator_test(&it))
				strcpy(data + (index++) * ETH_GSTRING_LEN,
				       main_strings[i + k]);

		for (i = 0; i < priv->tx_ring_num; i++) {
			sprintf(data + (index++) * ETH_GSTRING_LEN,
				"tx%d_packets", i);
			sprintf(data + (index++) * ETH_GSTRING_LEN,
				"tx%d_bytes", i);
		}
		for (i = 0; i < priv->rx_ring_num; i++) {
			sprintf(data + (index++) * ETH_GSTRING_LEN,
				"rx%d_packets", i);
			sprintf(data + (index++) * ETH_GSTRING_LEN,
				"rx%d_bytes", i);
#ifdef LL_EXTENDED_STATS
			sprintf(data + (index++) * ETH_GSTRING_LEN,
				"rx%d_napi_yield", i);
			sprintf(data + (index++) * ETH_GSTRING_LEN,
				"rx%d_misses", i);
			sprintf(data + (index++) * ETH_GSTRING_LEN,
				"rx%d_cleaned", i);
#endif
		}
		break;
	}
}

static u32 mlx4_en_autoneg_get(struct net_device *dev)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);
	struct mlx4_en_dev *mdev = priv->mdev;
	u32 autoneg = AUTONEG_DISABLE;

	if ((mdev->dev->caps.flags2 & MLX4_DEV_CAP_FLAG2_ETH_BACKPL_AN_REP) &&
	    priv->port_state.autoneg) {
		autoneg = AUTONEG_ENABLE;
	}

	return autoneg;
}

static int mlx4_en_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);
	int trans_type;

	/* SUPPORTED_1000baseT_Half isn't supported */
	cmd->supported = SUPPORTED_1000baseT_Full
			|SUPPORTED_10000baseT_Full;

	cmd->advertising = ADVERTISED_1000baseT_Full
			  |ADVERTISED_10000baseT_Full;

	cmd->supported |= SUPPORTED_1000baseKX_Full
			|SUPPORTED_10000baseKX4_Full
			|SUPPORTED_10000baseKR_Full
			|SUPPORTED_10000baseR_FEC
			|SUPPORTED_40000baseKR4_Full
			|SUPPORTED_40000baseCR4_Full
			|SUPPORTED_40000baseSR4_Full
			|SUPPORTED_40000baseLR4_Full;

	/* ADVERTISED_1000baseT_Half isn't advertised */
	cmd->advertising |= ADVERTISED_1000baseKX_Full
			  |ADVERTISED_10000baseKX4_Full
			  |ADVERTISED_10000baseKR_Full
			  |ADVERTISED_10000baseR_FEC
			  |ADVERTISED_40000baseKR4_Full
			  |ADVERTISED_40000baseCR4_Full
			  |ADVERTISED_40000baseSR4_Full
			  |ADVERTISED_40000baseLR4_Full;

	if (mlx4_en_QUERY_PORT(priv->mdev, priv->port))
		return -ENOMEM;

	cmd->autoneg = mlx4_en_autoneg_get(dev);
	if (cmd->autoneg == AUTONEG_ENABLE) {
		cmd->supported |= SUPPORTED_Autoneg;
		cmd->advertising |= ADVERTISED_Autoneg;
	}

	trans_type = priv->port_state.transciver;
	if (netif_carrier_ok(dev)) {
		ethtool_cmd_speed_set(cmd, priv->port_state.link_speed);
		cmd->duplex = DUPLEX_FULL;
	} else {
		ethtool_cmd_speed_set(cmd, -1);
		cmd->duplex = -1;
	}

	if (trans_type > 0 && trans_type <= 0xC) {
		cmd->port = PORT_FIBRE;
		cmd->transceiver = XCVR_EXTERNAL;
		cmd->supported |= SUPPORTED_FIBRE;
		cmd->advertising |= ADVERTISED_FIBRE;
	} else if (trans_type == 0x80 || trans_type == 0) {
		cmd->port = PORT_TP;
		cmd->transceiver = XCVR_INTERNAL;
		cmd->supported |= SUPPORTED_TP;
		cmd->advertising |= ADVERTISED_TP;
	} else  {
		cmd->port = -1;
		cmd->transceiver = -1;
	}
	return 0;
}

static const char *mlx4_en_duplex_to_string(int duplex)
{
	switch (duplex) {
	case DUPLEX_FULL:
		return "FULL";
	case DUPLEX_HALF:
		return "HALF";
	default:
		break;
	}
	return "UNKNOWN";
}

static int mlx4_en_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);
	struct mlx4_en_port_state *port_state = &priv->port_state;

	if ((cmd->autoneg != port_state->autoneg) ||
	    (ethtool_cmd_speed(cmd) != port_state->link_speed) ||
	    (cmd->duplex != DUPLEX_FULL)) {
		en_info(priv, "Changing port state properties (auto-negotiation"
			      " , speed/duplex) is not supported. Current:"
			      " auto-negotiation=%d speed/duplex=%d/%s\n",
			      port_state->autoneg, port_state->link_speed,
			      mlx4_en_duplex_to_string(DUPLEX_FULL));
		return -EOPNOTSUPP;
	}

	/* User provided same port state properties that are currently set.
	 * Nothing to change
	 */
	return 0;
}

static int mlx4_en_get_coalesce(struct net_device *dev,
			      struct ethtool_coalesce *coal)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);

	coal->tx_coalesce_usecs = priv->tx_usecs;
	coal->tx_max_coalesced_frames = priv->tx_frames;
	coal->rx_coalesce_usecs = priv->rx_usecs;
	coal->rx_max_coalesced_frames = priv->rx_frames;

	coal->pkt_rate_low = priv->pkt_rate_low;
	coal->rx_coalesce_usecs_low = priv->rx_usecs_low;
	coal->pkt_rate_high = priv->pkt_rate_high;
	coal->rx_coalesce_usecs_high = priv->rx_usecs_high;
	coal->rate_sample_interval = priv->sample_interval;
	coal->use_adaptive_rx_coalesce = priv->adaptive_rx_coal;
	return 0;
}

static int mlx4_en_set_coalesce(struct net_device *dev,
			      struct ethtool_coalesce *coal)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);
	int err, i;

	priv->rx_frames = (coal->rx_max_coalesced_frames ==
			   MLX4_EN_AUTO_CONF) ?
				MLX4_EN_RX_COAL_TARGET /
				priv->dev->mtu + 1 :
				coal->rx_max_coalesced_frames;
	priv->rx_usecs = (coal->rx_coalesce_usecs ==
			  MLX4_EN_AUTO_CONF) ?
				MLX4_EN_RX_COAL_TIME :
				coal->rx_coalesce_usecs;

	/* Setting TX coalescing parameters */
	if (coal->tx_coalesce_usecs != priv->tx_usecs ||
	    coal->tx_max_coalesced_frames != priv->tx_frames) {
		priv->tx_usecs = coal->tx_coalesce_usecs;
		priv->tx_frames = coal->tx_max_coalesced_frames;
		if (priv->port_up) {
			for (i = 0; i < priv->tx_ring_num; i++) {
				priv->tx_cq[i]->moder_cnt = priv->tx_frames;
				priv->tx_cq[i]->moder_time = priv->tx_usecs;
				if (mlx4_en_set_cq_moder(priv, priv->tx_cq[i]))
					en_warn(priv, "Failed changing moderation for TX cq %d\n", i);
			}
		}
	}

	/* Set adaptive coalescing params */
	priv->pkt_rate_low = coal->pkt_rate_low;
	priv->rx_usecs_low = coal->rx_coalesce_usecs_low;
	priv->pkt_rate_high = coal->pkt_rate_high;
	priv->rx_usecs_high = coal->rx_coalesce_usecs_high;
	priv->sample_interval = coal->rate_sample_interval;
	priv->adaptive_rx_coal = coal->use_adaptive_rx_coalesce;
	if (priv->adaptive_rx_coal)
		return 0;

	if (priv->port_up) {
		for (i = 0; i < priv->rx_ring_num; i++) {
			priv->rx_cq[i]->moder_cnt = priv->rx_frames;
			priv->rx_cq[i]->moder_time = priv->rx_usecs;
			priv->last_moder_time[i] = MLX4_EN_AUTO_CONF;
				err = mlx4_en_set_cq_moder(priv, priv->rx_cq[i]);
			if (err)
				return err;
		}
	}

	return 0;
}

static int mlx4_en_set_pauseparam(struct net_device *dev,
				struct ethtool_pauseparam *pause)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);
	struct mlx4_en_dev *mdev = priv->mdev;
	int err;

	if (pause->autoneg)
		return -EOPNOTSUPP;

	priv->prof->tx_pause = pause->tx_pause != 0;
	priv->prof->rx_pause = pause->rx_pause != 0;
	err = mlx4_SET_PORT_general(mdev->dev, priv->port,
				    priv->rx_skb_size + ETH_FCS_LEN,
				    priv->prof->tx_pause,
				    priv->prof->tx_ppp,
				    priv->prof->rx_pause,
				    priv->prof->rx_ppp);
	if (err)
		en_err(priv, "Failed setting pause params\n");

	return err;
}

static void mlx4_en_get_pauseparam(struct net_device *dev,
				 struct ethtool_pauseparam *pause)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);

	pause->tx_pause = priv->prof->tx_pause;
	pause->rx_pause = priv->prof->rx_pause;
	pause->autoneg = mlx4_en_autoneg_get(dev);
}

/* rtnl lock must be taken before calling */
int mlx4_en_pre_config(struct mlx4_en_priv *priv)
{
#ifdef CONFIG_RFS_ACCEL
	struct cpu_rmap *rmap;

	if (!priv->dev->rx_cpu_rmap)
		return 0;

	/* Disable RFS events
	 * Must have all RFS jobs flushed before freeing resources
	 */
	rmap = priv->dev->rx_cpu_rmap;
	priv->dev->rx_cpu_rmap = NULL;

	rtnl_unlock();
	free_irq_cpu_rmap(rmap);
	rtnl_lock();

	if (priv->dev->rx_cpu_rmap)
		return -EBUSY; /* another configuration completed while lock
				* was free
				*/

	/* Make sure all currently running filter_work are being processed
	 * Other work will return immediatly because of disable_rfs
	 */
	flush_workqueue(priv->mdev->workqueue);

#endif

	return 0;
}

static int mlx4_en_set_ringparam(struct net_device *dev,
				 struct ethtool_ringparam *param)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);
	struct mlx4_en_dev *mdev = priv->mdev;
	u32 rx_size, tx_size;
	int port_up = 0;
	int err = 0;
	int i, n_stats;
	u64 *data = NULL;

	if (!priv->port_up)
		return -ENOMEM;

	if (param->rx_jumbo_pending || param->rx_mini_pending)
		return -EINVAL;

	rx_size = roundup_pow_of_two(param->rx_pending);
	rx_size = max_t(u32, rx_size, MLX4_EN_MIN_RX_SIZE);
	rx_size = min_t(u32, rx_size, MLX4_EN_MAX_RX_SIZE);
	tx_size = roundup_pow_of_two(param->tx_pending);
	tx_size = max_t(u32, tx_size, MLX4_EN_MIN_TX_SIZE);
	tx_size = min_t(u32, tx_size, MLX4_EN_MAX_TX_SIZE);

	if (rx_size == (priv->port_up ? priv->rx_ring[0]->actual_size :
					priv->rx_ring[0]->size) &&
	    tx_size == priv->tx_ring[0]->size)
		return 0;
	err = mlx4_en_pre_config(priv);
	if (err)
		return err;

	mutex_lock(&mdev->state_lock);
	if (priv->port_up) {
		port_up = 1;
		mlx4_en_stop_port(dev);
	}

	/* Cache port statistics */
	n_stats = mlx4_en_get_sset_count(dev, ETH_SS_STATS);
	if (n_stats > 0) {
		data = kmalloc(n_stats * sizeof(u64), GFP_KERNEL);
		if (data)
			mlx4_en_get_ethtool_stats(dev, NULL, data);
	}

	mlx4_en_free_resources(priv);

	priv->prof->tx_ring_size = tx_size;
	priv->prof->rx_ring_size = rx_size;

	err = mlx4_en_alloc_resources(priv);
	if (err) {
		en_err(priv, "Failed reallocating port resources\n");
		goto out;
	}

	/* Restore port statistics */
	if (n_stats > 0 && data)
		mlx4_en_restore_ethtool_stats(priv, data);

	if (port_up) {
		err = mlx4_en_start_port(dev);
		if (err) {
			en_err(priv, "Failed starting port\n");
			goto out;
		}

		for (i = 0; i < priv->rx_ring_num; i++) {
			priv->rx_cq[i]->moder_cnt = priv->rx_frames;
			priv->rx_cq[i]->moder_time = priv->rx_usecs;
			priv->last_moder_time[i] = MLX4_EN_AUTO_CONF;
			err = mlx4_en_set_cq_moder(priv, priv->rx_cq[i]);
			if (err)
				goto out;
		}
	}

out:
	kfree(data);
	mutex_unlock(&mdev->state_lock);
	return err;
}

static void mlx4_en_get_ringparam(struct net_device *dev,
				  struct ethtool_ringparam *param)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);

	if (!priv->port_up)
		return;

	memset(param, 0, sizeof(*param));
	param->rx_max_pending = MLX4_EN_MAX_RX_SIZE;
	param->tx_max_pending = MLX4_EN_MAX_TX_SIZE;
	param->rx_pending = priv->port_up ?
		priv->rx_ring[0]->actual_size : priv->rx_ring[0]->size;
	param->tx_pending = priv->tx_ring[0]->size;
}

static u32 mlx4_en_get_rxfh_indir_size(struct net_device *dev)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);

	return priv->rx_ring_num;
}

static int mlx4_en_get_rxfh_indir(struct net_device *dev, u32 *ring_index)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);
	struct mlx4_en_rss_map *rss_map = &priv->rss_map;
	int rss_rings;
	size_t n = priv->rx_ring_num;
	int err = 0;

	rss_rings = priv->prof->rss_rings ?: priv->rx_ring_num;
	rss_rings = 1 << ilog2(rss_rings);

	while (n--) {
		ring_index[n] = rss_map->qps[n % rss_rings].qpn -
			rss_map->base_qpn;
	}

	return err;
}

static int mlx4_en_set_rxfh_indir(struct net_device *dev,
		const u32 *ring_index)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);
	struct mlx4_en_dev *mdev = priv->mdev;
	int port_up = 0;
	int err = 0;
	int i;
	int rss_rings = 0;

	/* Calculate RSS table size and make sure flows are spread evenly
	 * between rings
	 */
	for (i = 0; i < priv->rx_ring_num; i++) {
		if (i > 0 && !ring_index[i] && !rss_rings)
			rss_rings = i;

		if (ring_index[i] != (i % (rss_rings ?: priv->rx_ring_num)))
			return -EINVAL;
	}

	if (!rss_rings)
		rss_rings = priv->rx_ring_num;

	/* RSS table size must be an order of 2 */
	if (!is_power_of_2(rss_rings))
		return -EINVAL;

	mutex_lock(&mdev->state_lock);
	if (priv->port_up) {
		port_up = 1;
		mlx4_en_stop_port(dev);
	}

	priv->prof->rss_rings = rss_rings;

	if (port_up) {
		err = mlx4_en_start_port(dev);
		if (err)
			en_err(priv, "Failed starting port\n");
	}

	mutex_unlock(&mdev->state_lock);
	return err;
}

#define all_zeros_or_all_ones(field)		\
	((field) == 0 || (field) == (__force typeof(field))-1)

static int mlx4_en_validate_flow(struct net_device *dev,
				 struct mlx4_ethtool_rxnfc *cmd)
{
	struct ethtool_usrip4_spec *l3_mask;
	struct ethtool_tcpip4_spec *l4_mask;
	struct ethhdr *eth_mask;

	if (cmd->fs.location >= MAX_NUM_OF_FS_RULES)
		return -EINVAL;

	if (cmd->fs.flow_type & FLOW_MAC_EXT) {
		/* dest mac mask must be ff:ff:ff:ff:ff:ff */
		if (!is_broadcast_ether_addr(cmd->fs.m_ext.h_dest))
			return -EINVAL;
	}

	switch (cmd->fs.flow_type & ~(FLOW_EXT | FLOW_MAC_EXT)) {
	case TCP_V4_FLOW:
	case UDP_V4_FLOW:
		if (cmd->fs.m_u.tcp_ip4_spec.tos)
			return -EINVAL;
		l4_mask = &cmd->fs.m_u.tcp_ip4_spec;
		/* don't allow mask which isn't all 0 or 1 */
		if (!all_zeros_or_all_ones(l4_mask->ip4src) ||
		    !all_zeros_or_all_ones(l4_mask->ip4dst) ||
		    !all_zeros_or_all_ones(l4_mask->psrc) ||
		    !all_zeros_or_all_ones(l4_mask->pdst))
			return -EINVAL;
		break;
	case IP_USER_FLOW:
		l3_mask = &cmd->fs.m_u.usr_ip4_spec;
		if (l3_mask->l4_4_bytes || l3_mask->tos || l3_mask->proto ||
		    cmd->fs.h_u.usr_ip4_spec.ip_ver != ETH_RX_NFC_IP4 ||
		    (!l3_mask->ip4src && !l3_mask->ip4dst) ||
		    !all_zeros_or_all_ones(l3_mask->ip4src) ||
		    !all_zeros_or_all_ones(l3_mask->ip4dst))
			return -EINVAL;
		break;
	case ETHER_FLOW:
		eth_mask = &cmd->fs.m_u.ether_spec;
		/* source mac mask must not be set */
		if (!is_zero_ether_addr(eth_mask->h_source))
			return -EINVAL;

		/* dest mac mask must be ff:ff:ff:ff:ff:ff */
		if (!is_broadcast_ether_addr(eth_mask->h_dest))
			return -EINVAL;

		if (!all_zeros_or_all_ones(eth_mask->h_proto))
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}

	if ((cmd->fs.flow_type & FLOW_EXT)) {
		if (cmd->fs.m_ext.vlan_etype ||
		    !(cmd->fs.m_ext.vlan_tci == 0 ||
		      cmd->fs.m_ext.vlan_tci == cpu_to_be16(0xfff)))
			return -EINVAL;
		if (cmd->fs.m_ext.vlan_tci) {
			if (be16_to_cpu(cmd->fs.h_ext.vlan_tci) <
			    VLAN_MIN_VALUE ||
			    be16_to_cpu(cmd->fs.h_ext.vlan_tci) >
			    VLAN_MAX_VALUE)
				return -EINVAL;
		}
	}

	return 0;
}

static int mlx4_en_ethtool_add_mac_rule(struct mlx4_ethtool_rxnfc *cmd,
					struct list_head *rule_list_h,
					struct mlx4_spec_list *spec_l2,
					unsigned char *mac)
{
	int err = 0;
	__be64 mac_msk = cpu_to_be64(MLX4_MAC_MASK << 16);

	spec_l2->id = MLX4_NET_TRANS_RULE_ID_ETH;
	memcpy(spec_l2->eth.dst_mac_msk, &mac_msk, ETH_ALEN);
	memcpy(spec_l2->eth.dst_mac, mac, ETH_ALEN);

	if ((cmd->fs.flow_type & FLOW_EXT) && cmd->fs.m_ext.vlan_tci) {
		spec_l2->eth.vlan_id = cmd->fs.h_ext.vlan_tci;
		spec_l2->eth.vlan_id_msk = cpu_to_be16(0xfff);
	}

	list_add_tail(&spec_l2->list, rule_list_h);

	return err;
}

static int mlx4_en_ethtool_add_mac_rule_by_ipv4(struct mlx4_en_priv *priv,
						struct mlx4_ethtool_rxnfc *cmd,
						struct list_head *rule_list_h,
						struct mlx4_spec_list *spec_l2,
						__be32 ipv4_dst)
{
	unsigned char mac[ETH_ALEN];

	if (!ipv4_is_multicast(ipv4_dst)) {
		if (cmd->fs.flow_type & FLOW_MAC_EXT)
			memcpy(&mac, cmd->fs.h_ext.h_dest, ETH_ALEN);
		else
			memcpy(&mac, priv->dev->dev_addr, ETH_ALEN);
	} else {
		ip_eth_mc_map(ipv4_dst, mac);
	}

	return mlx4_en_ethtool_add_mac_rule(cmd, rule_list_h, spec_l2, &mac[0]);
}

static int add_ip_rule(struct mlx4_en_priv *priv,
				struct mlx4_ethtool_rxnfc *cmd,
				struct list_head *list_h)
{
	struct mlx4_spec_list *spec_l2 = NULL;
	struct mlx4_spec_list *spec_l3 = NULL;
	struct ethtool_usrip4_spec *l3_mask = &cmd->fs.m_u.usr_ip4_spec;

	spec_l3 = kzalloc(sizeof(*spec_l3), GFP_KERNEL);
	spec_l2 = kzalloc(sizeof(*spec_l2), GFP_KERNEL);
	if (!spec_l2 || !spec_l3) {
		en_err(priv, "Fail to alloc ethtool rule.\n");
		kfree(spec_l2);
		kfree(spec_l3);
		return -ENOMEM;
	}

	mlx4_en_ethtool_add_mac_rule_by_ipv4(priv, cmd, list_h, spec_l2,
					     cmd->fs.h_u.
					     usr_ip4_spec.ip4dst);
	spec_l3->id = MLX4_NET_TRANS_RULE_ID_IPV4;
	spec_l3->ipv4.src_ip = cmd->fs.h_u.usr_ip4_spec.ip4src;
	if (l3_mask->ip4src)
		spec_l3->ipv4.src_ip_msk = MLX4_BE_WORD_MASK;
	spec_l3->ipv4.dst_ip = cmd->fs.h_u.usr_ip4_spec.ip4dst;
	if (l3_mask->ip4dst)
		spec_l3->ipv4.dst_ip_msk = MLX4_BE_WORD_MASK;
	list_add_tail(&spec_l3->list, list_h);

	return 0;
}

static int add_tcp_udp_rule(struct mlx4_en_priv *priv,
			     struct mlx4_ethtool_rxnfc *cmd,
			     struct list_head *list_h, int proto)
{
	struct mlx4_spec_list *spec_l2 = NULL;
	struct mlx4_spec_list *spec_l3 = NULL;
	struct mlx4_spec_list *spec_l4 = NULL;
	struct ethtool_tcpip4_spec *l4_mask = &cmd->fs.m_u.tcp_ip4_spec;

	spec_l2 = kzalloc(sizeof(*spec_l2), GFP_KERNEL);
	spec_l3 = kzalloc(sizeof(*spec_l3), GFP_KERNEL);
	spec_l4 = kzalloc(sizeof(*spec_l4), GFP_KERNEL);
	if (!spec_l2 || !spec_l3 || !spec_l4) {
		en_err(priv, "Fail to alloc ethtool rule.\n");
		kfree(spec_l2);
		kfree(spec_l3);
		kfree(spec_l4);
		return -ENOMEM;
	}

	spec_l3->id = MLX4_NET_TRANS_RULE_ID_IPV4;

	if (proto == TCP_V4_FLOW) {
		mlx4_en_ethtool_add_mac_rule_by_ipv4(priv, cmd, list_h,
						     spec_l2,
						     cmd->fs.h_u.
						     tcp_ip4_spec.ip4dst);
		spec_l4->id = MLX4_NET_TRANS_RULE_ID_TCP;
		spec_l3->ipv4.src_ip = cmd->fs.h_u.tcp_ip4_spec.ip4src;
		spec_l3->ipv4.dst_ip = cmd->fs.h_u.tcp_ip4_spec.ip4dst;
		spec_l4->tcp_udp.src_port = cmd->fs.h_u.tcp_ip4_spec.psrc;
		spec_l4->tcp_udp.dst_port = cmd->fs.h_u.tcp_ip4_spec.pdst;
	} else {
		mlx4_en_ethtool_add_mac_rule_by_ipv4(priv, cmd, list_h,
						     spec_l2,
						     cmd->fs.h_u.
						     udp_ip4_spec.ip4dst);
		spec_l4->id = MLX4_NET_TRANS_RULE_ID_UDP;
		spec_l3->ipv4.src_ip = cmd->fs.h_u.udp_ip4_spec.ip4src;
		spec_l3->ipv4.dst_ip = cmd->fs.h_u.udp_ip4_spec.ip4dst;
		spec_l4->tcp_udp.src_port = cmd->fs.h_u.udp_ip4_spec.psrc;
		spec_l4->tcp_udp.dst_port = cmd->fs.h_u.udp_ip4_spec.pdst;
	}

	if (l4_mask->ip4src)
		spec_l3->ipv4.src_ip_msk = MLX4_BE_WORD_MASK;
	if (l4_mask->ip4dst)
		spec_l3->ipv4.dst_ip_msk = MLX4_BE_WORD_MASK;

	if (l4_mask->psrc)
		spec_l4->tcp_udp.src_port_msk = MLX4_BE_SHORT_MASK;
	if (l4_mask->pdst)
		spec_l4->tcp_udp.dst_port_msk = MLX4_BE_SHORT_MASK;

	list_add_tail(&spec_l3->list, list_h);
	list_add_tail(&spec_l4->list, list_h);

	return 0;
}

static int mlx4_en_ethtool_to_net_trans_rule(struct net_device *dev,
					     struct mlx4_ethtool_rxnfc *cmd,
					     struct list_head *rule_list_h)
{
	int err;
	struct ethhdr *eth_spec;
	struct mlx4_spec_list *spec_l2;
	struct mlx4_en_priv *priv = netdev_priv(dev);

	err = mlx4_en_validate_flow(dev, cmd);
	if (err)
		return err;

	switch (cmd->fs.flow_type & ~(FLOW_EXT | FLOW_MAC_EXT)) {
	case ETHER_FLOW:
		spec_l2 = kzalloc(sizeof(*spec_l2), GFP_KERNEL);
		if (!spec_l2)
			return -ENOMEM;

		eth_spec = &cmd->fs.h_u.ether_spec;
		mlx4_en_ethtool_add_mac_rule(cmd, rule_list_h, spec_l2, &eth_spec->h_dest[0]);
		spec_l2->eth.ether_type = eth_spec->h_proto;
		if (eth_spec->h_proto)
			spec_l2->eth.ether_type_enable = 1;
		break;
	case IP_USER_FLOW:
		err = add_ip_rule(priv, cmd, rule_list_h);
		break;
	case TCP_V4_FLOW:
		err = add_tcp_udp_rule(priv, cmd, rule_list_h, TCP_V4_FLOW);
		break;
	case UDP_V4_FLOW:
		err = add_tcp_udp_rule(priv, cmd, rule_list_h, UDP_V4_FLOW);
		break;
	}

	return err;
}

static int mlx4_en_flow_replace(struct net_device *dev,
				struct mlx4_ethtool_rxnfc *cmd)
{
	int err;
	struct mlx4_en_priv *priv = netdev_priv(dev);
	struct mlx4_en_dev *mdev = priv->mdev;
	struct ethtool_flow_id *loc_rule;
	struct mlx4_spec_list *spec, *tmp_spec;
	u32 qpn;
	u64 reg_id;

	struct mlx4_net_trans_rule rule = {
		.queue_mode = MLX4_NET_TRANS_Q_FIFO,
		.exclusive = 0,
		.allow_loopback = 1,
		.promisc_mode = MLX4_FS_REGULAR,
	};

	rule.port = priv->port;
	rule.priority = MLX4_DOMAIN_ETHTOOL | cmd->fs.location;
	INIT_LIST_HEAD(&rule.list);

	/* Allow direct QP attaches if the EN_ETHTOOL_QP_ATTACH flag is set */
	if (cmd->fs.ring_cookie == RX_CLS_FLOW_DISC)
		qpn = priv->drop_qp.qpn;
	else if (cmd->fs.ring_cookie & EN_ETHTOOL_QP_ATTACH) {
		qpn = cmd->fs.ring_cookie & (EN_ETHTOOL_QP_ATTACH - 1);
	} else {
		if (cmd->fs.ring_cookie >= priv->rx_ring_num) {
			en_warn(priv, "rxnfc: RX ring (%llu) doesn't exist.\n",
				cmd->fs.ring_cookie);
			return -EINVAL;
		}
		qpn = priv->rss_map.qps[cmd->fs.ring_cookie].qpn;
		if (!qpn) {
			en_warn(priv, "rxnfc: RX ring (%llu) is inactive.\n",
				cmd->fs.ring_cookie);
			return -EINVAL;
		}
	}
	rule.qpn = qpn;
	err = mlx4_en_ethtool_to_net_trans_rule(dev, cmd, &rule.list);
	if (err)
		goto out_free_list;

	mutex_lock(&mdev->state_lock);
	loc_rule = &priv->ethtool_rules[cmd->fs.location];
	if (loc_rule->id) {
		err = mlx4_flow_detach(priv->mdev->dev, loc_rule->id);
		if (err) {
			en_err(priv, "Fail to detach network rule at location %d. registration id = %llx\n",
			       cmd->fs.location, loc_rule->id);
			goto unlock;
		}
		loc_rule->id = 0;
		memset(&loc_rule->flow_spec, 0,
		       sizeof(struct ethtool_rx_flow_spec));
		list_del(&loc_rule->list);
	}
	err = mlx4_flow_attach(priv->mdev->dev, &rule, &reg_id);
	if (err) {
		en_err(priv, "Fail to attach network rule at location %d.\n",
		       cmd->fs.location);
		goto unlock;
	}
	loc_rule->id = reg_id;
	memcpy(&loc_rule->flow_spec, &cmd->fs,
	       sizeof(struct ethtool_rx_flow_spec));
	list_add_tail(&loc_rule->list, &priv->ethtool_list);

unlock:
	mutex_unlock(&mdev->state_lock);
out_free_list:
	list_for_each_entry_safe(spec, tmp_spec, &rule.list, list) {
		list_del(&spec->list);
		kfree(spec);
	}
	return err;
}

static int mlx4_en_flow_detach(struct net_device *dev,
			       struct mlx4_ethtool_rxnfc *cmd)
{
	int err = 0;
	struct ethtool_flow_id *rule;
	struct mlx4_en_priv *priv = netdev_priv(dev);
	struct mlx4_en_dev *mdev = priv->mdev;

	if (cmd->fs.location >= MAX_NUM_OF_FS_RULES)
		return -EINVAL;

	mutex_lock(&mdev->state_lock);
	rule = &priv->ethtool_rules[cmd->fs.location];
	if (!rule->id) {
		err =  -ENOENT;
		goto out;
	}

	err = mlx4_flow_detach(priv->mdev->dev, rule->id);
	if (err) {
		en_err(priv, "Fail to detach network rule at location %d. registration id = 0x%llx\n",
		       cmd->fs.location, rule->id);
		goto out;
	}
	rule->id = 0;
	memset(&rule->flow_spec, 0, sizeof(struct ethtool_rx_flow_spec));

	list_del(&rule->list);
out:
	mutex_unlock(&mdev->state_lock);
	return err;

}

static int mlx4_en_get_flow(struct net_device *dev, struct mlx4_ethtool_rxnfc *cmd,
			    int loc)
{
	int err = 0;
	struct ethtool_flow_id *rule;
	struct mlx4_en_priv *priv = netdev_priv(dev);

	if (loc < 0 || loc >= MAX_NUM_OF_FS_RULES)
		return -EINVAL;

	rule = &priv->ethtool_rules[loc];
	if (rule->id)
		memcpy(&cmd->fs, &rule->flow_spec,
		       sizeof(struct ethtool_rx_flow_spec));
	else
		err = -ENOENT;

	return err;
}

static int mlx4_en_get_num_flows(struct mlx4_en_priv *priv)
{

	int i, res = 0;
	for (i = 0; i < MAX_NUM_OF_FS_RULES; i++) {
		if (priv->ethtool_rules[i].id)
			res++;
	}
	return res;

}

static int mlx4_en_get_rxnfc(struct net_device *dev, struct ethtool_rxnfc *c,
			     u32 *rule_locs)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);
	struct mlx4_en_dev *mdev = priv->mdev;
	int err = 0;
	int i = 0, priority = 0;
	struct mlx4_ethtool_rxnfc *cmd = (struct mlx4_ethtool_rxnfc *)c;

	if ((cmd->cmd == ETHTOOL_GRXCLSRLCNT ||
	     cmd->cmd == ETHTOOL_GRXCLSRULE ||
	     cmd->cmd == ETHTOOL_GRXCLSRLALL) &&
	    (mdev->dev->caps.steering_mode !=
	     MLX4_STEERING_MODE_DEVICE_MANAGED || !priv->port_up))
		return -EINVAL;

	switch (cmd->cmd) {
	case ETHTOOL_GRXRINGS:
		cmd->data = priv->rx_ring_num;
		break;
	case ETHTOOL_GRXCLSRLCNT:
		cmd->rule_cnt = mlx4_en_get_num_flows(priv);
		break;
	case ETHTOOL_GRXCLSRULE:
		err = mlx4_en_get_flow(dev, cmd, cmd->fs.location);
		break;
	case ETHTOOL_GRXCLSRLALL:
		while ((!err || err == -ENOENT) && priority < cmd->rule_cnt) {
			err = mlx4_en_get_flow(dev, cmd, i);
			if (!err)
				rule_locs[priority++] = i;
			i++;
		}
		err = 0;
		break;
	default:
		err = -EOPNOTSUPP;
		break;
	}

	return err;
}

static int mlx4_en_set_rxnfc(struct net_device *dev, struct ethtool_rxnfc *c)
{
	int err = 0;
	struct mlx4_en_priv *priv = netdev_priv(dev);
	struct mlx4_en_dev *mdev = priv->mdev;
	struct mlx4_ethtool_rxnfc *cmd = (struct mlx4_ethtool_rxnfc *)c;

	if (mdev->dev->caps.steering_mode !=
	    MLX4_STEERING_MODE_DEVICE_MANAGED || !priv->port_up)
		return -EINVAL;

	switch (cmd->cmd) {
	case ETHTOOL_SRXCLSRLINS:
		err = mlx4_en_flow_replace(dev, cmd);
		break;
	case ETHTOOL_SRXCLSRLDEL:
		err = mlx4_en_flow_detach(dev, cmd);
		break;
	default:
		en_warn(priv, "Unsupported ethtool command. (%d)\n", cmd->cmd);
		return -EINVAL;
	}

	return err;
}

static void mlx4_en_get_channels(struct net_device *dev,
				 struct ethtool_channels *channel)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);

	memset(channel, 0, sizeof(*channel));

	channel->max_rx = MAX_RX_RINGS;
	channel->max_tx = MLX4_EN_MAX_TX_RING_P_UP;

	channel->rx_count = priv->rx_ring_num;
	channel->tx_count = priv->tx_ring_num / MLX4_EN_NUM_UP;
}

static int mlx4_en_set_channels(struct net_device *dev,
				struct ethtool_channels *channel)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);
	struct mlx4_en_dev *mdev = priv->mdev;
	int port_up = 0;
	int i;
	int err = 0;

	if (channel->other_count || channel->combined_count ||
	    channel->tx_count > MLX4_EN_MAX_TX_RING_P_UP ||
	    channel->rx_count > MAX_RX_RINGS ||
	    !channel->tx_count || !channel->rx_count)
		return -EINVAL;

	err = mlx4_en_pre_config(priv);
	if (err)
		return err;

	mutex_lock(&mdev->state_lock);
	if (priv->port_up) {
		port_up = 1;
		mlx4_en_stop_port(dev);
	}

	mlx4_en_free_resources(priv);

	priv->num_tx_rings_p_up = channel->tx_count;
	priv->tx_ring_num = channel->tx_count * MLX4_EN_NUM_UP;
	priv->rx_ring_num = channel->rx_count;

	err = mlx4_en_alloc_resources(priv);
	if (err) {
		en_err(priv, "Failed reallocating port resources\n");
		goto out;
	}

	netif_set_real_num_tx_queues(dev, priv->tx_ring_num);
	netif_set_real_num_rx_queues(dev, priv->rx_ring_num);

	mlx4_en_setup_tc(dev, MLX4_EN_NUM_UP);

	en_warn(priv, "Using %d TX rings\n", priv->tx_ring_num);
	en_warn(priv, "Using %d RX rings\n", priv->rx_ring_num);

	if (port_up) {
		err = mlx4_en_start_port(dev);
		if (err)
			en_err(priv, "Failed starting port\n");

		for (i = 0; i < priv->rx_ring_num; i++) {
			priv->rx_cq[i]->moder_cnt = priv->rx_frames;
			priv->rx_cq[i]->moder_time = priv->rx_usecs;
			priv->last_moder_time[i] = MLX4_EN_AUTO_CONF;
			err = mlx4_en_set_cq_moder(priv, priv->rx_cq[i]);
			if (err)
				goto out;
		}
	}

out:
	mutex_unlock(&mdev->state_lock);
	return err;
}

static int mlx4_en_get_ts_info(struct net_device *dev,
			       struct ethtool_ts_info *info)
{
	struct mlx4_en_priv *priv = netdev_priv(dev);
	struct mlx4_en_dev *mdev = priv->mdev;
	int ret;

	ret = ethtool_op_get_ts_info(dev, info);
	if (ret)
		return ret;

	if (mdev->dev->caps.flags2 & MLX4_DEV_CAP_FLAG2_TS) {
		info->so_timestamping |=
			SOF_TIMESTAMPING_TX_HARDWARE |
			SOF_TIMESTAMPING_RX_HARDWARE |
			SOF_TIMESTAMPING_RAW_HARDWARE;

		info->tx_types =
			(1 << HWTSTAMP_TX_OFF) |
			(1 << HWTSTAMP_TX_ON);

		info->rx_filters =
			(1 << HWTSTAMP_FILTER_NONE) |
			(1 << HWTSTAMP_FILTER_ALL);
	}

	return ret;
}

const struct ethtool_ops mlx4_en_ethtool_ops = {
	.get_drvinfo = mlx4_en_get_drvinfo,
	.get_settings = mlx4_en_get_settings,
	.set_settings = mlx4_en_set_settings,
	.get_link = ethtool_op_get_link,
	.get_strings = mlx4_en_get_strings,
	.get_sset_count = mlx4_en_get_sset_count,
	.get_ethtool_stats = mlx4_en_get_ethtool_stats,
	.self_test = mlx4_en_self_test,
	.get_wol = mlx4_en_get_wol,
	.set_wol = mlx4_en_set_wol,
	.get_msglevel = mlx4_en_get_msglevel,
	.set_msglevel = mlx4_en_set_msglevel,
	.get_coalesce = mlx4_en_get_coalesce,
	.set_coalesce = mlx4_en_set_coalesce,
	.get_pauseparam = mlx4_en_get_pauseparam,
	.set_pauseparam = mlx4_en_set_pauseparam,
	.get_ringparam = mlx4_en_get_ringparam,
	.set_ringparam = mlx4_en_set_ringparam,
	.get_rxnfc = mlx4_en_get_rxnfc,
	.set_rxnfc = mlx4_en_set_rxnfc,
	.get_rxfh_indir_size = mlx4_en_get_rxfh_indir_size,
	.get_rxfh_indir = mlx4_en_get_rxfh_indir,
	.set_rxfh_indir = mlx4_en_set_rxfh_indir,
	.get_channels = mlx4_en_get_channels,
	.set_channels = mlx4_en_set_channels,
	.get_ts_info = mlx4_en_get_ts_info,
};





