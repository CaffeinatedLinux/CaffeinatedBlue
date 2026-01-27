// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2024 GCT Semiconductor, Inc. All rights reserved. */
#include <ospi/ospi.h>
#include <linux/export.h>
#include <linux/mutex.h>
#include <linux/etherdevice.h>
#include <linux/netlink.h>
#include <asm/byteorder.h>
#include <net/sock.h>

#include "mw-netlink.h"
#include "avstream/xi-driver-priv.h"

static DEFINE_MUTEX(netlink_mutex);

#define ND_MAX_GROUP        30
#define ND_IFINDEX_LEN      sizeof(int)
#define ND_NLMSG_SPACE(len) (NLMSG_SPACE(len) + ND_IFINDEX_LEN)
#define ND_NLMSG_DATA(nlh)  ((void *)((char *)NLMSG_DATA(nlh) + ND_IFINDEX_LEN))
#define ND_NLMSG_S_LEN(len) ((len) + ND_IFINDEX_LEN)
#define ND_NLMSG_R_LEN(nlh) ((nlh)->nlmsg_len - ND_IFINDEX_LEN)
#define ND_NLMSG_IFIDX(nlh) NLMSG_DATA(nlh)
#define ND_MAX_MSG_LEN      (512)

static void (*rcv_cb)(void *priv, u16 type, void *msg, int len);

static void netlink_rcv_cb(struct sk_buff *skb)
{
	struct nlmsghdr	*nlh;
	u32 mlen;
	void *msg;
	int i;

	if (!rcv_cb) {
		xi_debug(1, "nl cb - unregistered\n");
		return;
	}

	if (skb->len < NLMSG_HDRLEN) {
		xi_debug(1, "nl cb - invalid skb length\n");
		return;
	}

	nlh = (struct nlmsghdr *)skb->data;

	xi_debug(10, "skb len:%d\n", skb->len);
	for (i = 0; i < skb->len; i++)
		xi_debug(10, "skb data[%d]:0x%x\n", i, *((uint8_t *)nlh + i));

	if (skb->len < nlh->nlmsg_len || nlh->nlmsg_len > ND_MAX_MSG_LEN)
		return;

	msg = NLMSG_DATA(nlh);
	mlen = skb->len - NLMSG_HDRLEN;

	rcv_cb(skb->sk, nlh->nlmsg_type, msg, mlen);
}

static void netlink_rcv(struct sk_buff *skb)
{
	mutex_lock(&netlink_mutex);
	netlink_rcv_cb(skb);
	mutex_unlock(&netlink_mutex);
}

struct sock *netlink_init(int unit,
		void (*cb)(void *priv, u16 type,
		void *msg, int len))
{
	struct sock *sock;
	struct netlink_kernel_cfg cfg = {
		.input  = netlink_rcv,
	};

	sock = netlink_kernel_create(&init_net, unit, &cfg);

	xi_debug(5, "sock:%p\n", sock);

	if (sock)
		rcv_cb = cb;

	return sock;
}

static void mw_nl_send_msg_to_user(struct sock *sock, int pid, uint16_t msg_type, uint8_t *msg)
{
	struct sk_buff *skb_out;
	struct nlmsghdr *nlh;
	int msg_size = 512;
	int res;

	if (!msg | !sock) {
		xi_debug(1, "arg is null\n");
		return;
	}

	xi_debug(20, "start msg_size:%d,NLMSG_SPACE(msg_size):%d\n", msg_size, NLMSG_SPACE(msg_size));

	skb_out = nlmsg_new(NLMSG_SPACE(msg_size), GFP_KERNEL);
	if (!skb_out) {
		xi_debug(0, "Failed to allocate new skb\n");
		return;
	}

	nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
	if (!nlh) {
		xi_debug(0, "Failed to put nlmsg\n");
		nlmsg_free(skb_out);
		return;
	}

	memcpy((char *)nlmsg_data(nlh), msg, msg_size);
	res = netlink_unicast(sock, skb_out, pid, MSG_DONTWAIT);
	if (res < 0)
		xi_debug(20, "Failed to send message: %d\n", res);
}

void mw_netlink_destroy(struct sock *sock)
{
	netlink_kernel_release(sock);
}

void mw_netlink_recv_callback(void *priv, u16 type, void *msg, int len)
{
	struct mw_netlink_msg *recv;
	struct sock *sock = priv;
	struct uio_device *idev;
	uint8_t cmd;

	if (!sock) {
		xi_debug(5, "sock is null\n");
		return;
	}

	recv = (struct mw_netlink_msg *)msg;

	idev = mw_uio_dev_manager_find_dev(recv->ack.handle);
	if (!idev) {
		xi_debug(5, "uio dev is null\n");
		return;
	}

	// for (int i = 0; i < len; i++)
	// 	xi_debug(10, "msg[%d]:0x%x\n", i, *((uint8_t *)msg + i));
	xi_debug(10, "ack_complete:%p\n", idev->ack_complete);
	cmd = (uint8_t)os_atomic_read(&idev->netlink_waiting_cmd);
	xi_debug(5, "recv cmd %d,wait cmd:%d\n", recv->ack.cmd, cmd);
	if (cmd == recv->ack.cmd) {
		xi_debug(10, "recv cmd %d, cmd:%d\n", recv->ack.cmd, cmd);
		memcpy(&idev->ack_msg, recv, len);
		os_event_set(idev->ack_complete);
	}
}

static int mw_netlink_send_msg_with_timeout(struct uio_device *uio_dev, struct mw_netlink_msg *msg, void *get_param1, void *get_param2, uint32_t timeout)
{
	int ret = 0;
	void *get_data_ptr1 = NULL;
	void *get_data_ptr2 = NULL;
	struct mw_netlink_msg *ack_msg = msg;

	if (!uio_dev) {
		xi_debug(1, "uio dev is null\n");
		return -EINVAL;
	}

	mw_nl_send_msg_to_user(uio_dev->fe_sock, uio_dev->user_pid, uio_dev->handle, (uint8_t *)ack_msg);
	xi_debug(10, "msg[0]:%d\n", ack_msg->ack.cmd);

	if (timeout != 0) {
		os_mutex_lock(uio_dev->ack_mutex);

		os_atomic_set(&uio_dev->netlink_waiting_cmd, ack_msg->ack.cmd);
		get_data_ptr1 = get_param1;
		get_data_ptr2 = get_param2;

		xi_debug(10, "ack_complete:%p\n", uio_dev->ack_complete);
		ret = os_event_wait(uio_dev->ack_complete, timeout);
		if (ret <= 0) {
			xi_debug(5, "wait failed\n");
			os_atomic_set(&uio_dev->netlink_waiting_cmd, 0);
			os_mutex_unlock(uio_dev->ack_mutex);
			return -EINVAL;
		}

		os_atomic_set(&uio_dev->netlink_waiting_cmd, 0);

		xi_debug(10, "ack_status:%d\n", uio_dev->ack_msg.ack.status);
		switch (uio_dev->ack_msg.ack.status) {
		case MW_NETLINK_ACK_OK:
			ret = 0;
			if (uio_dev->ack_msg.ack.type == MW_NETLINK_TYPE_GET) {
				if (get_data_ptr1)
					os_memcpy(get_data_ptr1, uio_dev->ack_msg.ack.param1, uio_dev->ack_msg.ack.len1);
				if (get_data_ptr2)
					os_memcpy(get_data_ptr2, uio_dev->ack_msg.ack.param2, uio_dev->ack_msg.ack.len2);
			}
			break;
		case MW_NETLINK_ACK_ERROR:
			ret = -EFAULT;
			break;
		default:
			ret = -EFAULT;
			break;
		}

		uio_dev->ack_msg.ack.status = 0;

		os_mutex_unlock(uio_dev->ack_mutex);
	}

	return ret;
}

int mw_netlink_notify_user_front_end(struct uio_device *uio_dev,
	uint8_t front_chn_index,
	uint8_t cmd, uint8_t type,
	void *param1,
	void *param2,
	uint8_t len1,
	uint8_t len2,
	uint32_t timeout)
{
	struct mw_netlink_msg msg;
	void *get_data_ptr1 = NULL;
	void *get_data_ptr2 = NULL;

	if (!uio_dev) {
		xi_debug(1, "uio dev is null\n");
		return -EINVAL;
	}

	if (len1 > 256 || len2 > 128)
		return -EINVAL;

	memset(&msg, 0, sizeof(msg));

	if (timeout == 0)
		msg.ack.is_ack = false;
	else
		msg.ack.is_ack = true;

	msg.ack.cmd = cmd;
	msg.ack.handle = uio_dev->handle;
	msg.ack.front_chn_index = front_chn_index;
	msg.ack.type = type;
	msg.ack.len1 = len1;
	msg.ack.len2 = len2;

	if (type == MW_NETLINK_TYPE_SET) {
		if (param1)
			memcpy(msg.ack.param1, param1, len1);
		if (param2)
			memcpy(msg.ack.param2, param2, len2);
	} else {
		get_data_ptr1 = param1;
		get_data_ptr2 = param2;
	}

	xi_debug(10, "enter cmd:%d\n", msg.data[0]);
	return mw_netlink_send_msg_with_timeout(uio_dev, &msg, get_data_ptr1, get_data_ptr2, timeout);
}
