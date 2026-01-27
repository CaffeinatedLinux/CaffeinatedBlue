////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2024 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#ifndef __XI_V4L2_H__
#define __XI_V4L2_H__

#include <media/v4l2-device.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-contig.h>
#include <media/v4l2-common.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-sg.h>

#include "mw-stream.h"
#include "xi-driver.h"
#include "v4l2-sg-buf.h"

struct mw_v4l2_channel {
	void                        *driver; /* low level card driver */

	/* video device */
	struct mutex                vfd_lock;
	struct video_device         *vfd;
	struct device               *pci_dev;

	struct mw_device            mw_dev;
};

struct xi_v4l2_dev {
	struct v4l2_device          v4l2_dev;

	void                        *driver; /* low level card driver */
	void                        *parent_dev;

	char                        driver_name[32];

	struct mw_v4l2_channel      *mw_vchs;
	int                         mw_vchs_count;
};

struct xi_stream_v4l2 {
	struct mutex                v4l2_mutex;
	unsigned long               generating;

	struct vb2_queue            vb2q;
	struct mutex                vb2_mutex;
	struct v4l2_sg_buf_queue    vsq;
	struct list_head            active;
	unsigned int                sequence;
	/* thread for generating video stream*/
	struct task_struct          *kthread;
	long long                   llstart_time;
	xi_timer                    *timer;
	bool                        exit_capture_thread;
	xi_notify_event             *notify;

	/* video info */
	struct xi_fmt               *fmt;
	unsigned int                width, height;
	unsigned int                stride;
	unsigned int                image_size;
	unsigned int                frame_duration;

	int req_cnt;
};
struct xi_stream_pipe {

	struct mw_v4l2_channel      *mw_vch;

	/* sync for close */
	struct rw_semaphore         io_sem;

	struct xi_stream_v4l2       s_v4l2;

	struct mw_stream            s_mw;
};

int xi_v4l2_init(struct xi_v4l2_dev *dev, void *driver, void *parent_dev);

int xi_v4l2_release(struct xi_v4l2_dev *dev);

int start_streaming(struct vb2_queue *vq, unsigned int count);
void stop_streaming(struct vb2_queue *vq);

#ifdef CONFIG_PM
int xi_v4l2_suspend(struct xi_v4l2_dev *dev);

int xi_v4l2_resume(struct xi_v4l2_dev *dev);
#endif

#endif /* __XI_V4L2_H__ */
