////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2024 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/list.h>

#include "ospi/ospi.h"

#include "mw-fe-uapi.h"

/**
 * struct device_mem - description of a UIO memory region
 * @name: name of the memory region for identification
 * @addr: address of the device's memory rounded to page
 *        size (phys_addr is used since addr can be
 *        logical, virtual, or physical & phys_addr_t
 *        should always be large enough to handle any of
 *        the address types)
 * @offs: offset of device memory within the page
 * @size: size of IO (multiple of page size)
 * @internal_addr: ioremap-ped version of addr, for driver internal use
 */
struct device_mem {
	const char          *name;
	phys_addr_t         addr;
	unsigned long       offs;
	resource_size_t     size;
	void __iomem        *internal_addr;
};

#define MW_DEV_NAME "mw-uio-dev"

struct uio_device {
	struct device           dev;
	struct cap_device_info  *info;
	struct mutex            info_lock;
	struct list_head        node;
	bool                    is_busy;
	int                     handle;

	/*netlink*/
	struct sock             *fe_sock;
	int                     user_pid; /*user->kernel*/
	int                     netlink_id; /*kernel->user*/

	os_mutex_t              ack_mutex;
	os_event_t              ack_complete;
	struct mw_netlink_msg   ack_msg;
	os_atomic_t             netlink_waiting_cmd;
};

/**
 * struct cap_device_info - UIO device capabilities
 * @uio_dev: the UIO device this info belongs to
 * @version: device driver version
 * @mem: list of mappable memory regions, size==0 for end of list
 * @priv: optional private data
 */
struct cap_device_info {
	struct uio_device *uio_dev;
	const char *version;
	struct device_mem mem;
	void *priv; /*capture ctx*/
};

int mw_uio_dev_register(struct device *parent, struct cap_device_info *info);

void mw_uio_dev_unregister(struct cap_device_info *info);

long mw_uio_dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg);

int mw_uio_dev_manager_create(void);

int mw_uio_dev_manager_destory(void);

struct sock *mw_uio_dev_manager_get_sock(void);

struct uio_device *mw_uio_dev_manager_find_dev(int handle);
