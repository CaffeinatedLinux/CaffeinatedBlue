////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2024 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include <ospi/ospi.h>

#include "v4l2-sg-buf.h"

#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/dma-mapping.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>

#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-fh.h>
#include <media/videobuf2-dma-sg.h>
#include <media/v4l2-common.h>
#include <media/v4l2-event.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-contig.h>
#include <media/v4l2-common.h>
#include <media/videobuf2-core.h>

#include "v4l2.h"

void v4l2_sg_queue_init(struct v4l2_sg_buf_queue *queue, enum v4l2_field field)
{
	queue->field = field;

	spin_lock_init(&queue->active_lock);
	INIT_LIST_HEAD(&queue->queued_list);
	INIT_LIST_HEAD(&queue->active_list);

	init_waitqueue_head(&queue->active_wait);
}


static int queue_setup(struct vb2_queue *q,
			   unsigned int *nbuffers, unsigned int *nplanes,
			   unsigned int sizes[], struct device *alloc_devs[])
{
	struct xi_stream_pipe *pipe = q->drv_priv;
	struct xi_stream_v4l2 *s_v4l2 = &pipe->s_v4l2;

	xi_debug(5, "nbuffers:%d,type:%d,io_modes:%d\n", *nbuffers, q->type, q->io_modes);

	if (*nbuffers > VIDEO_MAX_FRAME)
		*nbuffers = VIDEO_MAX_FRAME;
	*nplanes = 1;
	sizes[0] = s_v4l2->image_size;

	return 0;
}

static int buffer_init(struct vb2_buffer *vb)
{
	struct xi_stream_pipe *pipe = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *v4l2_buf = to_vb2_v4l2_buffer(vb);
	struct v4l2_sg_buf *vbuf = container_of(v4l2_buf, struct v4l2_sg_buf, vb2);
	struct xi_stream_v4l2 *s_v4l2 = &pipe->s_v4l2;
	unsigned long size = s_v4l2->image_size;
	int num_pages = size >> PAGE_SHIFT;

	xi_debug(5, "num_pages:%d,type:%d\n", num_pages, vb->type);

	vbuf->mwsg_list = os_malloc(sizeof(*vbuf->mwsg_list) * num_pages);
	vbuf->queue = &s_v4l2->vsq;

	return 0;
}

static int buffer_prepare(struct vb2_buffer *vb)
{
	struct xi_stream_pipe *pipe = vb2_get_drv_priv(vb->vb2_queue);
	struct xi_stream_v4l2 *s_v4l2 = &pipe->s_v4l2;
	unsigned long size = s_v4l2->image_size;

	xi_debug(20, "\n");

	if (vb2_plane_size(vb, 0) < size) {
		xi_debug(0, "buffer too small (%lu < %lu)\n",
				vb2_plane_size(vb, 0), size);
		return -EINVAL;
	}
		xi_debug(20, "size:%d\n", size);
	vb2_set_plane_payload(vb, 0, size);
	return 0;
}

static void buffer_queue(struct vb2_buffer *vb)
{
	struct xi_stream_pipe *pipe = vb2_get_drv_priv(vb->vb2_queue);
	struct xi_stream_v4l2 *s_v4l2 = &pipe->s_v4l2;
	struct v4l2_sg_buf_queue *vsq = &s_v4l2->vsq;
	struct vb2_v4l2_buffer *v4l2_buf = to_vb2_v4l2_buffer(vb);
	struct v4l2_sg_buf *vbuf = container_of(v4l2_buf, struct v4l2_sg_buf, vb2);

	xi_debug(20, "\n");

	list_add_tail(&vbuf->queued_node, &vsq->queued_list);

	if (vb->vb2_queue->streaming) {
		spin_lock_bh(&vsq->active_lock);
		list_add_tail(&vbuf->active_node, &vsq->active_list);

		wake_up_interruptible_sync(&vsq->active_wait);
		spin_unlock_bh(&vsq->active_lock);
	}
}

static void buffer_finish(struct vb2_buffer *vb)
{
	struct xi_stream_pipe *pipe = vb2_get_drv_priv(vb->vb2_queue);
	struct xi_stream_v4l2 *s_v4l2 = &pipe->s_v4l2;
	struct vb2_v4l2_buffer *v4l2_buf = to_vb2_v4l2_buffer(vb);
	struct v4l2_sg_buf *vbuf = container_of(v4l2_buf, struct v4l2_sg_buf, vb2);

	xi_debug(20, "vbuf:%p\n", vbuf);
	s_v4l2->vsq.last_dqueue_vbuf = vbuf;
}

/**
 * called once before the buffer is freed.
 * free mwsg list that used by private buffer
 */
static void buffer_cleanup(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *v4l2_buf = to_vb2_v4l2_buffer(vb);
	struct v4l2_sg_buf *vbuf = container_of(v4l2_buf, struct v4l2_sg_buf, vb2);

	xi_debug(20, "buffer cleanup\n");

	if (vbuf->mwsg_list != NULL)
		os_free(vbuf->mwsg_list);
}

int v4l2_sg_queue_streamon(struct v4l2_sg_buf_queue *queue)
{
	struct v4l2_sg_buf *vbuf = NULL;

	if (queue->streaming) {
		xi_debug(0, "already streaming\n");
		return -EBUSY;
	}

	xi_debug(20, "stream on\n");

	spin_lock_bh(&queue->active_lock);
	list_for_each_entry(vbuf, &queue->queued_list, queued_node)
		list_add_tail(&vbuf->active_node, &queue->active_list);
	spin_unlock_bh(&queue->active_lock);

	queue->streaming = 1;

	return 0;
}

int v4l2_sg_queue_streamoff(struct v4l2_sg_buf_queue *queue)
{
	if (!queue->streaming) {
		xi_debug(5, "not streaming\n");
		return -EINVAL;
	}

	xi_debug(20, "stream stop\n");

	queue->streaming = 0;

	INIT_LIST_HEAD(&queue->queued_list);
	INIT_LIST_HEAD(&queue->active_list);

	return 0;
}

static const struct vb2_ops vb2_qops = {
	.queue_setup = queue_setup,
	.buf_init    = buffer_init,
	.buf_prepare = buffer_prepare,
	.buf_queue   = buffer_queue,
	.buf_finish  = buffer_finish,
	.buf_cleanup = buffer_cleanup,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish  = vb2_ops_wait_finish,
	.start_streaming = start_streaming,
	.stop_streaming = stop_streaming,
};

int v4l2_vb2_queue_init(struct vb2_queue *q, struct mutex *ext_mutex, struct device *parent_dev, void *priv)
{
	int ret;

	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_DMABUF | VB2_READ | VB2_USERPTR;
	q->drv_priv = priv;
	q->ops = &vb2_qops;
	q->gfp_flags = GFP_DMA32;
	q->mem_ops = &vb2_dma_sg_memops;
	q->buf_struct_size = sizeof(struct v4l2_sg_buf);//use default struct vb2_v4l2_buffer vb2;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->lock = ext_mutex;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 8, 0))
	q->dev = parent_dev;
#endif

	ret = vb2_queue_init(q);
	xi_debug(5, "ret:%d\n", ret);

	return ret;
}

void v4l2_sg_queue_deinit(struct v4l2_sg_buf_queue *queue)
{
	v4l2_sg_queue_streamoff(queue);
}

struct v4l2_sg_buf *v4l2_sg_queue_get_activebuf(struct v4l2_sg_buf_queue *queue)
{
	struct v4l2_sg_buf *vbuf = NULL;

	spin_lock_bh(&queue->active_lock);

	if (list_empty(&queue->active_list))
		goto out;

	vbuf = list_entry(queue->active_list.next,
					struct v4l2_sg_buf, active_node);
	list_del(&vbuf->active_node);

out:
	spin_unlock_bh(&queue->active_lock);

	return vbuf;
}

void v4l2_sg_queue_put_donebuf(struct v4l2_sg_buf_queue *queue, struct v4l2_sg_buf *vbuf)
{
	xi_debug(20, "done\n");
	vb2_buffer_done(&vbuf->vb2.vb2_buf, VB2_BUF_STATE_DONE);
}

int v4l2_sg_get_last_frame_sdianc_data(struct v4l2_sg_buf_queue *queue,
									   MWCAP_SDI_ANC_PACKET **packets)
{
	struct v4l2_sg_buf *vbuf = NULL;

	if (packets == NULL || queue == NULL)
		return OS_RETURN_INVAL;

	vbuf = queue->last_dqueue_vbuf;

	if (vbuf == NULL)
		return OS_RETURN_NODATA;

	if (vbuf->anc_packet_count <= 0)
		return OS_RETURN_NODATA;

	*packets = vbuf->anc_packets;

	return vbuf->anc_packet_count;
}
