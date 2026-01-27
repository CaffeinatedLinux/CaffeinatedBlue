/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2024 GCT Semiconductor, Inc. All rights reserved. */

#ifndef _MW_NETLINK_H
#define _MW_NETLINK_H

#include <linux/netdevice.h>
#include <net/sock.h>
#include "front-end/front-end.h"
#include "mw-fe-uapi.h"

#define UIO_NETLINK_ID 31

struct sock *netlink_init(int unit, void (*cb)(void *priv, u16 type, void *msg, int len));

void mw_netlink_recv_callback(void *priv, u16 type, void *msg, int len);

int mw_netlink_notify_user_front_end(struct uio_device *uio_dev,
	uint8_t front_chn_index,
	uint8_t cmd, uint8_t type,
	void *param1,
	void *param2,
	uint8_t len1,
	uint8_t len2,
	uint32_t timeout);

void mw_netlink_destroy(struct sock *sock);

#endif /* _NETLINK_K_H_ */
