////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2024 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#ifndef __V4L2_SG_BUFFER_H__
#define __V4L2_SG_BUFFER_H__

#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/videodev2.h>
#include <linux/scatterlist.h>

#include "mw-procapture-extension.h"

#include "mw-sg.h"
#include <media/videobuf2-v4l2.h>

#define V4L2_SG_BUF_MAX_FRAME_SIZE	(64*1024*1024)

enum v4l2_sg_buf_state {
	V4L2_SG_BUF_STATE_DEQUEUED = 0,
	V4L2_SG_BUF_STATE_PREPARED,
	V4L2_SG_BUF_STATE_QUEUED,
	V4L2_SG_BUF_STATE_ACTIVE,
	V4L2_SG_BUF_STATE_DONE,
	V4L2_SG_BUF_STATE_ERROR,
};

struct v4l2_sg_buf_queue;

struct v4l2_sg_buf_dma_desc {
	void                        *vaddr;
	struct page                 **pages;
	int                         write;
	int                         offset;
	unsigned long               size;
	unsigned int                num_pages;
	struct scatterlist          *sglist;
	atomic_t                    mmap_refcount;
};

struct v4l2_sg_buf {
	struct vb2_v4l2_buffer      vb2;

	struct list_head            queued_node;
	struct list_head            active_node;

	mw_scatterlist_t            *mwsg_list;
	unsigned int                mwsg_len;

	struct v4l2_sg_buf_queue    *queue;

	int                         anc_packet_count;
	MWCAP_SDI_ANC_PACKET        anc_packets[MWCAP_MAX_ANC_PACKETS_PER_FRAME];
};

struct v4l2_sg_buf_queue {
	enum v4l2_field             field;
	unsigned int                streaming:1;
	struct list_head            queued_list;
	spinlock_t                  active_lock;
	struct list_head            active_list;
	wait_queue_head_t           active_wait;

	struct v4l2_sg_buf          *last_dqueue_vbuf;
};

void v4l2_sg_queue_init(struct v4l2_sg_buf_queue *queue, enum v4l2_field field);
void v4l2_sg_queue_deinit(struct v4l2_sg_buf_queue *queue);
int v4l2_sg_queue_streamon(struct v4l2_sg_buf_queue *queue);
int v4l2_sg_queue_streamoff(struct v4l2_sg_buf_queue *queue);

struct v4l2_sg_buf *v4l2_sg_queue_get_activebuf(struct v4l2_sg_buf_queue *queue);
void v4l2_sg_queue_put_donebuf(struct v4l2_sg_buf_queue *queue, struct v4l2_sg_buf *vbuf);
int v4l2_sg_get_last_frame_sdianc_data(struct v4l2_sg_buf_queue *queue,
									   MWCAP_SDI_ANC_PACKET **packets);


int v4l2_vb2_queue_init(struct vb2_queue *q, struct mutex *ext_mutex, struct device *parent_dev, void *priv);
#endif /* __V4L2_SG_BUFFER_H__ */
