////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2024 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <linux/idr.h>
#include <linux/ioctl.h>
#include <linux/io.h>
#include <linux/eventfd.h>
#include <linux/version.h>
#include <linux/anon_inodes.h>
#include <linux/fdtable.h>
#include <linux/file.h>
#include <linux/mutex.h>

#include "mw-uio-drv.h"
#include "mw-netlink.h"

#define MW_MAX_DEVICES     (4096)
#define DEV(idev) (&((idev)->dev))
static const char this_driver_name[] = "uio-driver";

typedef struct _mw_uio_dev_manager {
	struct sock *fe_sock;
	int dev_major;
	struct cdev *dev_cdev;
	struct idr dev_idr;
	spinlock_t idr_lock;
} mw_uio_dev_manager_t;

static mw_uio_dev_manager_t g_mw_uio_dev_manager;

static struct attribute *mw_uio_dev_attrs[] = {
	NULL,
};
ATTRIBUTE_GROUPS(mw_uio_dev);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,2,0)
static char *mw_devnode(struct device *dev, umode_t *mode)
#else
static char *mw_devnode(const struct device *dev, umode_t *mode)
#endif
{
	if (mode)
		*mode = 0666;//S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH
	return NULL;
}

static struct class mw_uio_dev_class = {
	.name = MW_DEV_NAME,
	.dev_groups = mw_uio_dev_groups,
	.devnode = mw_devnode,
};


static int mw_uio_dev_open(struct inode *inode, struct file *filep)
{
	struct uio_device *idev = NULL;
	struct device *device;
	int handle = iminor(inode);

	idev = mw_uio_dev_manager_find_dev(handle);
	xi_debug(5, "idev:%p\n", idev);
	if (!idev)
		return -ENODEV;

	device = DEV(idev);

	if (idev->is_busy)
		return -EBUSY;

	get_device(device);

	idev->is_busy = TRUE;
	filep->private_data = idev;

	return 0;
}

static int mw_uio_dev_release(struct inode *inode, struct file *filep)
{
	int ret = 0;
	struct uio_device *idev = filep->private_data;
	struct device *device;

	if (!idev)
		return -EINVAL;

	device = DEV(idev);
	idev->is_busy = FALSE;
	put_device(device);

	filep->private_data = NULL;

	return ret;
}

static const struct vm_operations_struct uio_physical_vm_ops = {
#ifdef CONFIG_HAVE_IOREMAP_PROT
	.access = generic_access_phys,
#endif
};

static int uio_mmap_physical(struct vm_area_struct *vma)
{
	struct uio_device *idev = vma->vm_private_data;
	struct device_mem *mem;

	mem = &idev->info->mem;

	if (mem->addr & ~PAGE_MASK)
		return -ENODEV;
	if (vma->vm_end - vma->vm_start > mem->size)
		return -EINVAL;

	vma->vm_ops = &uio_physical_vm_ops;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	/*
	 * We cannot use the vm_iomap_memory() helper here,
	 * because vma->vm_pgoff is the map index we looked
	 * up above in uio_find_mem_index(), rather than an
	 * actual page offset into the mmap.
	 *
	 * So we just do the physical mmap without a page
	 * offset.
	 */
	return remap_pfn_range(vma,
			vma->vm_start,
			mem->addr >> PAGE_SHIFT,
			vma->vm_end - vma->vm_start,
			vma->vm_page_prot);
}

static int mw_uio_dev_mmap(struct file *filep, struct vm_area_struct *vma)
{
	struct uio_device *idev = filep->private_data;
	unsigned long requested_pages, actual_pages;
	int ret = 0;

	if (vma->vm_end < vma->vm_start)
		return -EINVAL;

	vma->vm_private_data = idev;

	if (!idev->info) {
		ret = -EINVAL;
		goto out;
	}

	requested_pages = vma_pages(vma);
	actual_pages = ((idev->info->mem.addr & ~PAGE_MASK)
			+ idev->info->mem.size + PAGE_SIZE - 1) >> PAGE_SHIFT;
	if (requested_pages > actual_pages) {
		ret = -EINVAL;
		goto out;
	}

	ret = uio_mmap_physical(vma);

 out:
	return ret;
}


static const struct file_operations mw_uio_dev_fops = {
	.owner      = THIS_MODULE,
	.open       = mw_uio_dev_open,
	.release    = mw_uio_dev_release,
	.llseek     = noop_llseek,
	.unlocked_ioctl = mw_uio_dev_ioctl,
	.mmap       = mw_uio_dev_mmap,
};

static int mw_uio_dev_major_init(void)
{
	static const char name[] = MW_DEV_NAME;
	struct cdev *cdev = NULL;
	dev_t uio_device = 0;
	int result;

	result = alloc_chrdev_region(&uio_device, 0, MW_MAX_DEVICES, name);
	if (result)
		goto out;

	result = -ENOMEM;
	cdev = cdev_alloc();
	if (!cdev)
		goto out_unregister;

	cdev->owner = THIS_MODULE;
	cdev->ops = &mw_uio_dev_fops;
	kobject_set_name(&cdev->kobj, "%s", name);

	result = cdev_add(cdev, uio_device, MW_MAX_DEVICES);
	if (result)
		goto out_put;

	g_mw_uio_dev_manager.dev_major = MAJOR(uio_device);
	g_mw_uio_dev_manager.dev_cdev = cdev;

	return 0;
out_put:
	kobject_put(&cdev->kobj);
out_unregister:
	unregister_chrdev_region(uio_device, MW_MAX_DEVICES);
out:
	return result;
}

static void mw_uio_dev_major_cleanup(void)
{
	unregister_chrdev_region(MKDEV(g_mw_uio_dev_manager.dev_major, 0), MW_MAX_DEVICES);
	cdev_del(g_mw_uio_dev_manager.dev_cdev);
}

int mw_uio_dev_manager_create(void)
{
	mw_uio_dev_manager_t *mgr = &g_mw_uio_dev_manager;
	int ret;

	if (!mgr)
		return -EINVAL;

	mgr->fe_sock = netlink_init(UIO_NETLINK_ID, mw_netlink_recv_callback);
	if (mgr->fe_sock == NULL)
		return -EINVAL;

	/* This is the first time in here, set everything up properly */
	ret = mw_uio_dev_major_init();
	if (ret)
		goto exit;

	ret = class_register(&mw_uio_dev_class);
	if (ret) {
		xi_debug(1, "class_register failed for uio\n");
		goto err_class_register;
	}

	idr_init(&g_mw_uio_dev_manager.dev_idr);
	spin_lock_init(&g_mw_uio_dev_manager.idr_lock);

	return 0;

err_class_register:
	mw_uio_dev_major_cleanup();
exit:
	if (mgr->fe_sock)
		mw_netlink_destroy(mgr->fe_sock);
	mgr->fe_sock = NULL;

	return ret;
}

int mw_uio_dev_manager_destory(void)
{
	mw_uio_dev_manager_t *mgr = &g_mw_uio_dev_manager;

	if (!mgr)
		return -EINVAL;

	if (mgr->fe_sock)
		mw_netlink_destroy(mgr->fe_sock);
	mgr->fe_sock = NULL;

	class_unregister(&mw_uio_dev_class);
	mw_uio_dev_major_cleanup();

	idr_destroy(&mgr->dev_idr);

	return 0;
}

struct sock *mw_uio_dev_manager_get_sock(void)
{
	return g_mw_uio_dev_manager.fe_sock;
}

struct uio_device *mw_uio_dev_manager_find_dev(int handle)
{
	struct uio_device *idev = NULL;

	spin_lock_bh(&g_mw_uio_dev_manager.idr_lock);
	idev = idr_find(&g_mw_uio_dev_manager.dev_idr, handle);
	if (!idev) {
		spin_unlock_bh(&g_mw_uio_dev_manager.idr_lock);
		xi_debug(1, "no device find\n");
		return NULL;
	}
	spin_unlock_bh(&g_mw_uio_dev_manager.idr_lock);

	return idev;
}

static int mw_manager_alloc_dev_minor(struct uio_device *idev)
{
	int retval;

	spin_lock_bh(&g_mw_uio_dev_manager.idr_lock);
	retval = idr_alloc_cyclic(&g_mw_uio_dev_manager.dev_idr, idev, 0, MW_MAX_DEVICES, GFP_KERNEL);
	if (retval >= 0) {
		idev->handle = retval;
		retval = 0;
	} else if (retval == -ENOSPC) {
		xi_debug(1, "minor index over limit %d\n", MW_MAX_DEVICES);
	}
	spin_unlock_bh(&g_mw_uio_dev_manager.idr_lock);

	return retval;
}

/**
 * mw_uio_dev_free_minor - free mwio3 device minor
 *
 * As I do not want to allocate an ID that has been allocated before,
 * I will not perform the release operation for unregister.
 */
static int mw_uio_dev_free_minor(struct uio_device *idev, uint32_t mintor)
{
	spin_lock_bh(&g_mw_uio_dev_manager.idr_lock);
	idr_remove(&g_mw_uio_dev_manager.dev_idr, mintor);
	spin_unlock_bh(&g_mw_uio_dev_manager.idr_lock);

	return 0;
}

static void mw_uio_dev_device_release(struct device *dev)
{
	/*do nothing*/
}

int mw_uio_dev_register(struct device *parent, struct cap_device_info *info)
{
	struct uio_device *idev;
	int ret = 0;

	if (!parent || !info)
		return -EINVAL;

	idev = kzalloc(sizeof(struct uio_device), GFP_KERNEL);
	if (!idev)
		return -ENOMEM;

	idev->info = info;

	ret = mw_manager_alloc_dev_minor(idev);
	if (ret != 0)
		return ret;

	device_initialize(&idev->dev);
	idev->dev.devt = MKDEV(g_mw_uio_dev_manager.dev_major, idev->handle);
	idev->dev.class = &mw_uio_dev_class;
	idev->dev.parent = parent;
	idev->dev.release = mw_uio_dev_device_release;
	idev->fe_sock = g_mw_uio_dev_manager.fe_sock;
	idev->ack_complete = os_event_alloc();
	idev->ack_mutex = os_mutex_alloc();

	os_atomic_set(&idev->netlink_waiting_cmd, 0);

	dev_set_drvdata(&idev->dev, idev);

	ret = dev_set_name(&idev->dev, "mw-uio-%d", idev->handle);
	if (ret) {
		mw_uio_dev_free_minor(idev, idev->handle);
		put_device(&idev->dev);
		return ret;
	}

	ret = device_add(&idev->dev);
	if (ret) {
		mw_uio_dev_free_minor(idev, idev->handle);
		put_device(&idev->dev);
		return ret;
	}

	xi_debug(5, "handle:%d, idev:%p\n", idev->handle, idev);

	info->uio_dev = idev;

	return 0;
}

void mw_uio_dev_unregister(struct cap_device_info *info)
{
	struct uio_device *idev;

	if (!info)
		return;

	idev = info->uio_dev;
	if (!idev)
		return;

	os_mutex_free(idev->ack_mutex);
	idev->ack_mutex = NULL;

	os_event_free(idev->ack_complete);
	idev->ack_complete = NULL;
	os_atomic_set(&idev->netlink_waiting_cmd, 0);

	idev->info = NULL;

	device_unregister(&idev->dev);

	mw_uio_dev_free_minor(idev, idev->handle);

	kfree(idev);
	info->uio_dev = NULL;
}
