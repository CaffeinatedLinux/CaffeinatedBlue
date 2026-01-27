////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "mw-capture-impl.h"
#include "ospi/ospi.h"

#include "mw-procapture-extension.h"
#include "mw-linux.h"
#include "mw-event-dev.h"

#include "supports/shared-image-buffer.h"
#include "dma/mw-dma-mem-priv.h"

#include "xi-driver-priv.h"

struct mw_pin_buffer {
    struct list_head                list_node;

    struct mw_dma_desc              *dma_desc;
    MWCAP_PTR                       address;
    u32                             size;
    bool                            is_busy;
};

struct frame_capture_request {
    struct list_head                list_entry;
    int                             iframe;
    MWCAP_PTR                       context;
    u32                             size;
    u32                             stride;
    bool                            bottomup;

    unsigned short                  cy_partial_notify;
    bool                            frame_completed;
    unsigned short                  cy_completed;
    unsigned short                  cy_completed_prev;
    bool                            notify_user;

    MWCAP_PTR                       user_addr;
    bool                            physical_valid;
    mw_physical_addr_t              phy_addr;

    struct mw_dma_desc              *dma;
    struct mw_pin_buffer            *pin_buf;

    struct shared_image_buf         *osd_image;
    RECT                            osd_rects[MWCAP_VIDEO_MAX_NUM_OSD_RECTS];
    int                             osd_count;

    short                           brightness;
    short                           contrast;
    short                           hue;
    short                           saturation;

    u32                             fourcc;
    short                           width;
    short                           height;
    int                             aspectx;
    int                             aspecty;
    MWCAP_VIDEO_COLOR_FORMAT        cf_output;
    MWCAP_VIDEO_QUANTIZATION_RANGE  quant_range;
    MWCAP_VIDEO_SATURATION_RANGE    sat_range;
    RECT                            rect_src;
    RECT                            rect_target;
    MWCAP_VIDEO_DEINTERLACE_MODE    dei_mode;

    MWCAP_VIDEO_ASPECT_RATIO_CONVERT_MODE
                                    aspect_mode;

    int                             i_real_frame;
};

/*
static bool is_req_list_empty(struct mw_stream_cap *mwcap)
{
    bool ret;

    os_spin_lock_bh(mwcap->req_lock);
    ret = list_empty(&mwcap->req_list);
    os_spin_unlock_bh(mwcap->req_lock);

    return ret;
}
*/


static struct mw_pin_buffer *mw_capture_get_pin_buf(
        struct mw_stream_cap *mwcap, MWCAP_PTR addr)
{
    struct mw_pin_buffer *buf = NULL;

    if (!list_empty(&mwcap->pin_buf_list)) {
        list_for_each_entry(buf, &mwcap->pin_buf_list, list_node) {
            if (buf->address == addr) {
                return buf;
            }
        }
    }

    return NULL;
}

static int mw_capture_buffer_pin(struct mw_stream_cap *mwcap,
                                 int mem_type,
                                 MWCAP_PTR addr, u32 size, int direction)
{
    struct mw_dma_desc *dma;
    struct mw_pin_buffer *buf;
    int ret = 0;

    if (addr == 0 || size == 0)
        return OS_RETURN_INVAL;

    os_spin_lock(mwcap->pin_buf_lock);
    if (mw_capture_get_pin_buf(mwcap, addr) != NULL) {
        os_spin_unlock(mwcap->pin_buf_lock);
        return OS_RETURN_INVAL;
    }
    os_spin_unlock(mwcap->pin_buf_lock);

    buf = os_zalloc(sizeof(*buf));
    if (buf == NULL)
        return OS_RETURN_NOMEM;

    ret = mw_dma_memory_create_desc(
                &dma,
                mem_type,
                addr,
                size,
                direction,
                mwcap->dma_priv);
    if (ret != 0) {
        goto dma_err;
    }

    buf->dma_desc = dma;
    buf->is_busy = false;
    buf->address = addr;
    buf->size = size;

    os_spin_lock(mwcap->pin_buf_lock);
    list_add_tail(&buf->list_node, &mwcap->pin_buf_list);
    os_spin_unlock(mwcap->pin_buf_lock);

    return 0;

dma_err:
    os_free(buf);
    return ret;
}

static int mw_capture_buffer_unpin(struct mw_stream_cap *mwcap,
                          MWCAP_PTR addr)
{
    struct mw_pin_buffer *buf = NULL;
    int ret = 0;

    if (addr == 0)
        return OS_RETURN_INVAL;

    os_spin_lock(mwcap->pin_buf_lock);
    buf = mw_capture_get_pin_buf(mwcap, addr);
    if (buf == NULL) {
        ret = OS_RETURN_INVAL;
        goto exist_err;
    }

    if (buf->is_busy) {
        ret = OS_RETURN_BUSY;
        goto exist_err;
    }
    list_del(&buf->list_node);
    os_spin_unlock(mwcap->pin_buf_lock);

    mw_dma_memory_destroy_desc(buf->dma_desc);
    os_free(buf);

    return 0;

exist_err:
    os_spin_unlock(mwcap->pin_buf_lock);
    return ret;
}

int _GetCaptureFrameID(struct mw_stream_cap *mwcap, int iFrame, bool *bStatusImage)
{
    if (iFrame <= MWCAP_VIDEO_FRAME_ID_NEWEST_BUFFERED
        && iFrame >= MWCAP_VIDEO_FRAME_ID_NEXT_BUFFERING) {
        MWCAP_VIDEO_BUFFER_INFO bufferInfo;

        os_event_clear(mwcap->low_latency_notify->event);

        xi_driver_get_video_buffer_info(mwcap->driver, mwcap->m_iChannel, &bufferInfo);

        switch (iFrame) {
        case MWCAP_VIDEO_FRAME_ID_NEWEST_BUFFERED:
            iFrame = mwcap->m_iFrameBuffered = bufferInfo.iNewestBuffered;
            break;
        case MWCAP_VIDEO_FRAME_ID_NEWEST_BUFFERING:
            iFrame = mwcap->m_iFrameBuffering = bufferInfo.iNewestBuffering;
            break;
        case MWCAP_VIDEO_FRAME_ID_NEXT_BUFFERED:
            while (mwcap->m_iFrameBuffered == bufferInfo.iNewestBuffered
                && !mwcap->exit_capture_thread) {
                os_event_wait(mwcap->low_latency_notify->event, -1);
                xi_driver_get_video_buffer_info(mwcap->driver, mwcap->m_iChannel, &bufferInfo);
            }
            iFrame = mwcap->m_iFrameBuffered = bufferInfo.iNewestBuffered;
            break;
        case MWCAP_VIDEO_FRAME_ID_NEXT_BUFFERING:
            while (mwcap->m_iFrameBuffering == bufferInfo.iNewestBuffering
                && !mwcap->exit_capture_thread) {
                os_event_wait(mwcap->low_latency_notify->event, -1);
                xi_driver_get_video_buffer_info(mwcap->driver, mwcap->m_iChannel, &bufferInfo);
            }
            iFrame = mwcap->m_iFrameBuffering = bufferInfo.iNewestBuffering;
            break;
        }

        if (bStatusImage != NULL)
            *bStatusImage = true;
    } else  {
        if (bStatusImage != NULL)
            *bStatusImage = false;
    }

    return iFrame;
}

static void partial_notify_proc(MWCAP_PTR context, BOOLEAN frame_completed,
                                int cy_completed)
{
    struct mw_stream_cap *mwcap = (struct mw_stream_cap *)(unsigned long)context;
    struct frame_capture_request *request = NULL;
    bool notify_user = false;

    os_spin_lock_bh(mwcap->req_lock);
    if (!list_empty(&mwcap->completed_list)) {
        request = list_entry(mwcap->completed_list.next,
                struct frame_capture_request, list_entry);

        if (!request->frame_completed
                && cy_completed > request->cy_completed) {
            request->cy_completed = cy_completed;
            notify_user = (request->cy_partial_notify != 0)
                    && !frame_completed;
            request->notify_user = notify_user;
        }
    }
    os_spin_unlock_bh(mwcap->req_lock);

    if (notify_user)
        os_event_set(mwcap->video_done);
}

static int xi_mwcap_thread_proc(void *data)
{
    struct mw_stream_cap *mwcap = data;

    xi_debug(5, "xi media thread started\n");

#if defined(__linux__)
    os_set_freezable();
#endif

    for (;;) {
        struct frame_capture_request *request = NULL;
        struct mw_dma_desc *dma;
        bool show_status_image = false;

        /* the sleep doesn't take place if condition is ture */
        os_event_wait(mwcap->req_event, -1);
        if (mwcap->exit_capture_thread)
            break;

        os_spin_lock_bh(mwcap->req_lock);
        if (!list_empty(&mwcap->req_list)) {
            request = list_entry(mwcap->req_list.next,
                    struct frame_capture_request, list_entry);
            list_del(&request->list_entry);
        }
        os_spin_unlock_bh(mwcap->req_lock);

        if (request == NULL)
            continue;

        request->frame_completed = false;
        request->cy_completed = 0;
        request->cy_completed_prev = 0;
        request->notify_user = false;
        request->i_real_frame = _GetCaptureFrameID(mwcap, request->iframe, &show_status_image);

        os_spin_lock_bh(mwcap->req_lock);
        list_add_tail(&request->list_entry, &mwcap->completed_list);
        os_spin_unlock_bh(mwcap->req_lock);

        dma = request->dma == NULL ? request->pin_buf->dma_desc : request->dma;
        xi_driver_sg_get_frame(
                    mwcap->driver,
                    mwcap->m_iChannel,
                    dma->mwsg_list,
                    dma->sglen,
                    request->stride,
                    request->i_real_frame,
                    show_status_image,
                    &request->osd_image->image_buf,
                    request->osd_rects,
                    request->osd_count,
                    request->fourcc,
                    request->width,
                    request->height,
                    FOURCC_GetBpp(request->fourcc),
                    request->bottomup,
                    &request->rect_src,
                    &request->rect_target,
                    request->aspectx,
                    request->aspecty,
                    request->cf_output,
                    request->quant_range,
                    request->sat_range,
                    request->contrast,
                    request->brightness,
                    request->saturation,
                    request->hue,
                    request->dei_mode,
                    request->aspect_mode,
                    request->cy_partial_notify,
                    partial_notify_proc,
                    (unsigned long)mwcap,
                    1000
                    );

        os_spin_lock_bh(mwcap->req_lock);
        request->frame_completed = TRUE;
        os_spin_unlock_bh(mwcap->req_lock);

        os_event_set(mwcap->video_done);

#if defined(__linux__)
        os_try_to_freeze();
#endif
    }

    /* sync with kthread_stop */
    os_thread_terminate_self(mwcap->kthread);

    xi_debug(5, "xi media thread: exit\n");
    return 0;
}

static int mwcap_start_generating(struct mw_stream_cap *mwcap,
                                  os_event_t event)
{
    int ret = -1;

    xi_debug(5, "entering function %s\n", __func__);

    if (os_test_and_set_bit(0, &mwcap->generating))
        return OS_RETURN_BUSY;

    mwcap->video_done = event;

    mwcap->m_iFrameBuffered = MWCAP_VIDEO_FRAME_ID_EMPTY;
    mwcap->m_iFrameBuffering = MWCAP_VIDEO_FRAME_ID_EMPTY;

    mwcap->low_latency_notify = xi_driver_new_notify_event(
                mwcap->driver,
                mwcap->m_iChannel,
                MWCAP_NOTIFY_VIDEO_FRAME_BUFFERED
                | MWCAP_NOTIFY_VIDEO_FRAME_BUFFERING,
                NULL
                );
    if (mwcap->low_latency_notify == NULL) {
        ret = OS_RETURN_ERROR;
        goto err;
    }

    mwcap->exit_capture_thread = false;
    mwcap->kthread = os_thread_create_and_run(xi_mwcap_thread_proc, mwcap, "XI-MWCAP");
    if (mwcap->kthread == NULL) {
        xi_debug(0, "Create kernel thread XI-MWCAP Error: %m\n");
        ret = OS_RETURN_NOMEM;
        goto err;
    }

    INIT_LIST_HEAD(&mwcap->req_list);
    INIT_LIST_HEAD(&mwcap->completed_list);

    xi_debug(5, "returning from %s\n", __func__);

    return 0;

err:
    if (mwcap->low_latency_notify != NULL) {
        xi_driver_delete_notify_event(mwcap->driver, mwcap->m_iChannel, mwcap->low_latency_notify);
        mwcap->low_latency_notify = NULL;
    }

    os_clear_bit(0, &mwcap->generating);
    xi_debug(5, "returning from %s\n", __func__);

    return ret;
}

static void mwcap_stop_generating(struct mw_stream_cap *mwcap)
{
    struct frame_capture_request *request = NULL;

    xi_debug(5, "entering function %s\n", __func__);

    if (!os_test_and_clear_bit(0, &mwcap->generating))
        return;

    mwcap->exit_capture_thread = true;
    os_event_set(mwcap->req_event);
    /* shutdown control thread */
    if (mwcap->kthread) {
        os_thread_wait_and_free(mwcap->kthread);
        mwcap->kthread = NULL;
    }

    /* thread stopped, no need to lock req_lock,
        protected by video mutex */
    while (!list_empty(&mwcap->req_list)) {
        request = list_entry(mwcap->req_list.next,
                struct frame_capture_request, list_entry);
        list_del(&request->list_entry);

        os_spin_lock(mwcap->pin_buf_lock);
        if (request->pin_buf != NULL)
            request->pin_buf->is_busy = false;
        os_spin_unlock(mwcap->pin_buf_lock);

        if (request->dma != NULL)
            mw_dma_memory_destroy_desc(request->dma);
        os_free(request);
    }
    while (!list_empty(&mwcap->completed_list)) {
        request = list_entry(mwcap->completed_list.next,
                struct frame_capture_request, list_entry);
        list_del(&request->list_entry);

        os_spin_lock(mwcap->pin_buf_lock);
        if (request->pin_buf != NULL)
            request->pin_buf->is_busy = false;
        os_spin_unlock(mwcap->pin_buf_lock);

        if (request->dma != NULL)
            mw_dma_memory_destroy_desc(request->dma);
        os_free(request);
    }

    if (mwcap->low_latency_notify != NULL) {
        xi_driver_delete_notify_event(mwcap->driver, mwcap->m_iChannel, mwcap->low_latency_notify);
        mwcap->low_latency_notify = NULL;
    }
}

static int mwcap_put_video_frame(struct mw_stream_cap *mwcap,
        MWCAP_VIDEO_CAPTURE_FRAME *pframe)
{
    struct frame_capture_request *request = NULL;
    int ret = 0;

    request = os_zalloc(sizeof(struct frame_capture_request));
    if (request == NULL) {
        ret = OS_RETURN_NOMEM;
        return ret;
    }

    request->iframe                 = (signed char)pframe->iSrcFrame;
    request->physical_valid         = pframe->bPhysicalAddress;
    request->context                = pframe->pvContext;
    request->bottomup               = pframe->bBottomUp;
    request->size                   = pframe->cbFrame;
    request->stride                 = pframe->cbStride;

    request->cy_partial_notify      = pframe->cyPartialNotify;

    request->osd_image              = (struct shared_image_buf *)(unsigned long)pframe->pOSDImage;
    request->osd_count              = pframe->cOSDRects;
    if (request->osd_image != NULL && request->osd_count != 0) {
        os_memcpy(request->osd_rects, pframe->aOSDRects,
                sizeof(RECT) * min(request->osd_count, MWCAP_VIDEO_MAX_NUM_OSD_RECTS));
    }

    request->contrast               = pframe->sContrast;
    request->brightness             = pframe->sBrightness;
    request->saturation             = pframe->sSaturation;
    request->hue                    = pframe->sHue;

    request->fourcc                 = pframe->dwFOURCC;
    request->width                  = pframe->cx;
    request->height                 = pframe->cy;
    request->aspectx                = pframe->nAspectX;
    request->aspecty                = pframe->nAspectY;
    request->cf_output              = pframe->colorFormat;
    request->quant_range            = pframe->quantRange;
    request->sat_range              = pframe->satRange;
    request->rect_src               = pframe->rectSource;
    request->rect_target            = pframe->rectTarget;
    request->dei_mode               = pframe->deinterlaceMode;
    request->aspect_mode            = pframe->aspectRatioConvertMode;

    request->dma = NULL;
    if (request->physical_valid) {
        request->phy_addr = (mw_physical_addr_t)(unsigned long)pframe->liPhysicalAddress.QuadPart;
        ret = mw_dma_memory_create_desc(
                    &request->dma,
                    MWCAP_VIDEO_MEMORY_TYPE_PHYSICAL,
                    (unsigned long)request->phy_addr,
                    request->size,
                    MW_DMA_FROM_DEVICE,
                    NULL
                    );
    } else {
        request->user_addr = pframe->pvFrame;

        os_spin_lock(mwcap->pin_buf_lock);
        request->pin_buf = mw_capture_get_pin_buf(mwcap, request->user_addr);
        if (request->pin_buf != NULL)
            request->pin_buf->is_busy = true;
        os_spin_unlock(mwcap->pin_buf_lock);

        if (request->pin_buf == NULL)
            ret = mw_dma_memory_create_desc(
                        &request->dma,
                        MWCAP_VIDEO_MEMORY_TYPE_USER,
                        (unsigned long)request->user_addr,
                        request->size,
                        MW_DMA_FROM_DEVICE,
                        mwcap->dma_priv
                        );
    }

    if (ret != 0) {
        os_free(request);
        return ret;
    }

    os_spin_lock_bh(mwcap->req_lock);
    list_add_tail(&request->list_entry, &mwcap->req_list);
    os_spin_unlock_bh(mwcap->req_lock);

    os_event_set(mwcap->req_event);

    return ret;
}

static int mwcap_get_video_status(struct mw_stream_cap *mwcap,
        MWCAP_VIDEO_CAPTURE_STATUS *pstatus)
{
    struct frame_capture_request *request = NULL;
    bool is_unlocked = false;

    os_spin_lock_bh(mwcap->req_lock);
    if (!list_empty(&mwcap->completed_list)) {
        request = list_entry(mwcap->completed_list.next,
                struct frame_capture_request, list_entry);
    }

    if (request == NULL) {
        os_memset(pstatus, 0, sizeof(MWCAP_VIDEO_CAPTURE_STATUS));
        pstatus->iFrame = MWCAP_VIDEO_FRAME_ID_EMPTY;
    } else {
        pstatus->pvContext          = request->context;
        pstatus->bPhysicalAddress   = request->physical_valid;
        if (request->physical_valid) {
            pstatus->liPhysicalAddress.QuadPart = request->phy_addr;
        } else {
            pstatus->pvFrame = request->user_addr;
        }

        pstatus->bFrameCompleted    = request->frame_completed;
        pstatus->cyCompleted        = request->cy_completed;
        pstatus->cyCompletedPrev    = request->cy_completed_prev;
        pstatus->iFrame             = request->i_real_frame;

        request->cy_completed_prev  = request->cy_completed;

        if (pstatus->bFrameCompleted) {
            list_del(&request->list_entry);
        }
        os_spin_unlock_bh(mwcap->req_lock);
        is_unlocked = true;

#if defined(__arm__) || defined(__aarch64__)
        if (request->pin_buf != NULL) {
            if (pstatus->cyCompleted > pstatus->cyCompletedPrev)
                mw_dma_memory_sync_for_cpu_ex(
                            request->pin_buf->dma_desc,
                            pstatus->cyCompletedPrev * request->stride,
                            (pstatus->cyCompleted - pstatus->cyCompletedPrev) * request->stride
                            );
        }
        else if (!pstatus->bFrameCompleted) {
            if (pstatus->cyCompleted > pstatus->cyCompletedPrev)
                mw_dma_memory_sync_for_cpu_ex(
                            request->dma,
                            pstatus->cyCompletedPrev * request->stride,
                            (pstatus->cyCompleted - pstatus->cyCompletedPrev) * request->stride
                            );
        }
#endif

        if (pstatus->bFrameCompleted) {
            os_spin_lock(mwcap->pin_buf_lock);
            if (request->pin_buf != NULL)
                request->pin_buf->is_busy = false;
            os_spin_unlock(mwcap->pin_buf_lock);

            if (request->dma != NULL) {
                //mw_dma_memory_sync_for_cpu(request->dma);
                mw_dma_memory_destroy_desc(request->dma);
            }

            os_free(request);
        }
    }

    if (!is_unlocked)
        os_spin_unlock_bh(mwcap->req_lock);

    return 0;
}


static void mwcap_audio_capture_close(struct mw_stream_cap *mwcap)
{
    if (mwcap->audio_notify != NULL) {
        xi_driver_delete_notify_event(mwcap->driver, mwcap->m_iChannel, mwcap->audio_notify);
        mwcap->audio_notify = NULL;
    }

    if (mwcap->m_bAudioAcquired) {
        xi_driver_release_audio_capture(mwcap->driver);
        mwcap->m_bAudioAcquired = false;
    }

    os_clear_bit(0, &mwcap->audio_open);
}

static int mwcap_audio_capture_open(struct mw_stream_cap *mwcap)
{
    int ret = 0;

    if (os_test_and_set_bit(0, &mwcap->audio_open))
        return OS_RETURN_BUSY;

    xi_driver_acquire_audio_capture(mwcap->driver);
    mwcap->m_bAudioAcquired = true;

    mwcap->prev_audio_block_id = -1;
    mwcap->audio_notify = xi_driver_new_notify_event(mwcap->driver,
            mwcap->m_iChannel, MWCAP_NOTIFY_AUDIO_INPUT_RESET, NULL);
    if (mwcap->audio_notify == NULL) {
        ret = OS_RETURN_ERROR;
        mwcap_audio_capture_close(mwcap);
    }

    return ret;
}

static int mwcap_get_audio_frame(struct mw_stream_cap *mwcap,
        MWCAP_AUDIO_CAPTURE_FRAME * pframe)
{
    if (!os_test_bit(0, &mwcap->audio_open))
        return OS_RETURN_ERROR;

    if (os_event_try_wait(mwcap->audio_notify->event)) {
        mwcap->prev_audio_block_id = -1;
    }

    return xi_driver_get_normalized_audio_capture_frame(mwcap->driver,
            mwcap->m_iChannel, &mwcap->prev_audio_block_id, pframe);
}

static long mw_capture_audio_process(struct mw_stream_cap *mwcap, os_iocmd_t cmd, void * arg)
{
    long ret = 0;

    switch (cmd) {
    case MWCAP_IOCTL_AUDIO_CAPTURE_OPEN:
        ret = mwcap_audio_capture_open(mwcap);
        break;
    case MWCAP_IOCTL_AUDIO_CAPTURE_FRAME:
        ret = mwcap_get_audio_frame(mwcap, (MWCAP_AUDIO_CAPTURE_FRAME *)arg);
        break;
    case MWCAP_IOCTL_AUDIO_CAPTURE_CLOSE:
        mwcap_audio_capture_close(mwcap);
        break;
    }

    return ret;
}

static long mw_capture_video_process(struct mw_stream_cap *mwcap, os_iocmd_t cmd, void * arg)
{
    long ret = 0;

    switch (cmd) {
    case MWCAP_IOCTL_VIDEO_CAPTURE_OPEN:
        {
            MWCAP_VIDEO_CAPTURE_OPEN *par = (MWCAP_VIDEO_CAPTURE_OPEN *)arg;

            if (!mw_event_is_event_valid((os_event_t)(unsigned long)par->hEvent)) {
                ret = OS_RETURN_INVAL;
                break;
            }

            ret = mwcap_start_generating(mwcap, (os_event_t)(unsigned long)par->hEvent);
        }
        break;
    case MWCAP_IOCTL_VIDEO_CAPTURE_FRAME:
        {
            MWCAP_VIDEO_CAPTURE_FRAME *vframe = (MWCAP_VIDEO_CAPTURE_FRAME *)arg;

            if (!mw_capture_is_generating(mwcap)) {
                ret = OS_RETURN_ERROR;
                break;
            }

            ret = mwcap_put_video_frame(mwcap, vframe);
        }
        break;
    case MWCAP_IOCTL_VIDEO_CAPTURE_STATUS:
        {
            if (!mw_capture_is_generating(mwcap)) {
                ret = OS_RETURN_ERROR;
                break;
            }

            ret = mwcap_get_video_status(mwcap, (MWCAP_VIDEO_CAPTURE_STATUS *)arg);
            if (ret != 0)
                break;
        }
        break;
    case MWCAP_IOCTL_VIDEO_CAPTURE_CLOSE:
        mwcap_stop_generating(mwcap);
        break;
    }

    return ret;
}


static bool _is_timer_valid(struct mw_stream_cap *mwcap, xi_timer *timer)
{
    xi_timer *_timer;
    bool found = false;

    if (!list_empty(&mwcap->timer_list)) {
        list_for_each_entry(_timer, &mwcap->timer_list, user_node) {
            if (_timer == timer) {
                found = true;
                break;
            }
        }
    }

    return found;
}

static long mw_capture_timer_process(struct mw_stream_cap *mwcap, os_iocmd_t cmd, void * arg)
{
    long ret = 0;

    switch(cmd) {
    case MWCAP_IOCTL_TIMER_REGISTRATION:
        {
            xi_timer *timer;
            MWCAP_TIMER_REGISTRATION_S *treg = (MWCAP_TIMER_REGISTRATION_S *)arg;

            if (!mw_event_is_event_valid((os_event_t)(unsigned long)treg->pvEvent)) {
                ret = OS_RETURN_INVAL;
                break;
            }

            os_spin_unlock(mwcap->timer_lock); // malloc maybe sleep
            timer = xi_driver_new_timer(mwcap->driver, (os_event_t)(unsigned long)treg->pvEvent);
            if (timer == NULL) {
                ret = OS_RETURN_NOMEM;
                os_spin_lock(mwcap->timer_lock);
                break;
            }
            os_spin_lock(mwcap->timer_lock);

            treg->pvTimer = (MWCAP_PTR)(unsigned long)timer;
            list_add_tail(&timer->user_node, &mwcap->timer_list);
        }
        break;
    case MWCAP_IOCTL_TIMER_DEREGISTRATION:
        {
            xi_timer *timer = *(xi_timer **)arg;

            if (_is_timer_valid(mwcap, timer)) {
                list_del(&timer->user_node);
                xi_driver_delete_timer(mwcap->driver, timer);
            } else {
                ret = OS_RETURN_INVAL;
                break;
            }
        }
        break;
    case MWCAP_IOCTL_TIMER_EXPIRE_TIME:
        {
            MWCAP_TIMER_EXPIRE_TIME *exp = (MWCAP_TIMER_EXPIRE_TIME *)arg;
            xi_timer *timer;

            timer = (xi_timer *)(unsigned long)exp->pvTimer;

            if (!_is_timer_valid(mwcap, timer)) {
                ret = OS_RETURN_INVAL;
                break;
            }

            if (!xi_driver_schedual_timer(mwcap->driver, exp->llExpireTime, timer)) {
                ret = OS_RETURN_ERROR;
                break;
            }
        }
        break;
    default:
        break;
    }

    return ret;
}

static bool _is_notify_valid(struct mw_stream_cap *mwcap,
                             xi_notify_event *notify)
{
    xi_notify_event *_notify;
    bool found = false;

    if (!list_empty(&mwcap->notify_list)) {
        list_for_each_entry(_notify, &mwcap->notify_list, user_node) {
            if (_notify == notify) {
                found = true;
                break;
            }
        }
    }

    return found;
}

static long mw_capture_notify_process(struct mw_stream_cap *mwcap, os_iocmd_t cmd, void * arg)
{
    long ret = 0;

    switch(cmd) {
    case MWCAP_IOCTL_NOTIFY_REGISTRATION:
        {
            xi_notify_event *notify;
            MWCAP_NOTIFY_REGISTRATION_S *handler = (MWCAP_NOTIFY_REGISTRATION_S *)arg;

            if (!mw_event_is_event_valid((os_event_t)(unsigned long)handler->pvEvent)) {
                ret = OS_RETURN_INVAL;
                break;
            }

            os_spin_unlock(mwcap->notify_lock); // malloc maybe sleep
            notify = xi_driver_new_notify_event(mwcap->driver, mwcap->m_iChannel, handler->ullEnableBits,
                                                (os_event_t)(unsigned long)handler->pvEvent);
            if (notify == NULL) {
                ret = OS_RETURN_NOMEM;
                os_spin_lock(mwcap->notify_lock);
                break;
            }
            os_spin_lock(mwcap->notify_lock);

            handler->pvNotify = (MWCAP_PTR)(unsigned long)notify;
            list_add_tail(&notify->user_node, &mwcap->notify_list);
        }
        break;
    case MWCAP_IOCTL_NOTIFY_DEREGISTRATION:
        {
            xi_notify_event *notify = *(xi_notify_event **)arg;

            if (!_is_notify_valid(mwcap, notify)) {
                ret = OS_RETURN_INVAL;
                break;
            }

            list_del(&notify->user_node);
            xi_driver_delete_notify_event(mwcap->driver, mwcap->m_iChannel, notify);
        }
        break;
    case MWCAP_IOCTL_NOTIFY_STATUS:
        {
            MWCAP_NOTIFY_STATUS *status = (MWCAP_NOTIFY_STATUS *)arg;
            xi_notify_event *notify;

            notify = (xi_notify_event *)(unsigned long)status->pvNotify;

            if (!_is_notify_valid(mwcap, notify)) {
                ret = OS_RETURN_INVAL;
                break;
            }

            status->ullStatusBits = xi_driver_notify_event_get_status_bits(
                        mwcap->driver, mwcap->m_iChannel, notify);
        }
        break;
    case MWCAP_IOCTL_NOTIFY_ENABLE:
        {
            xi_notify_event *notify;
            MWCAP_NOTIFY_ENABLE *par = (MWCAP_NOTIFY_ENABLE *)arg;

            notify = (xi_notify_event *)(unsigned long)par->pvNotify;

            if (!_is_notify_valid(mwcap, notify)) {
                ret = OS_RETURN_INVAL;
                break;
            }

            xi_driver_notify_event_set_enable_bits(
                        mwcap->driver, mwcap->m_iChannel, notify, par->ullEnableBits);
        }
        break;
    default:
        break;
    }

    return ret;
}


static struct shared_image_user_node *_upload_imgbuf_find_user(
        struct list_head *head, struct shared_image_buf *pimage)
{
    struct shared_image_user_node *_image = NULL;

    if (pimage == NULL)
        return false;

    if (!list_empty(head)) {
        list_for_each_entry(_image, head, user_node) {
            if (_image->shared_buf == pimage) {
                return _image;
                break;
            }
        }
    }

    return NULL;
}

static long _upload_image(struct mw_stream_cap *mwcap, MWCAP_VIDEO_UPLOAD_IMAGE *upload_img)
{
    struct shared_image_buf *_imgbuf = (struct shared_image_buf *)(unsigned long)upload_img->pvDestImage;
    struct mw_dma_desc *dma = NULL;
    struct mw_pin_buffer *pin_buf = NULL;
    long ret = 0;

    if (!shared_image_buf_is_valid(xi_driver_get_imgbuf_manager(mwcap->driver), _imgbuf))
        return OS_RETURN_INVAL;

    if (upload_img->bSrcPhysicalAddress) {
        ret = mw_dma_memory_create_desc(
                    &dma,
                    MWCAP_VIDEO_MEMORY_TYPE_PHYSICAL,
                    (unsigned long)upload_img->liSrcPhysicalAddress.QuadPart,
                    upload_img->cbSrcFrame,
                    MW_DMA_TO_DEVICE,
                    NULL
                    );
    } else {
        os_spin_lock(mwcap->pin_buf_lock);
        pin_buf = mw_capture_get_pin_buf(mwcap, upload_img->pvSrcFrame);
        if (pin_buf != NULL)
            pin_buf->is_busy = true;
        os_spin_unlock(mwcap->pin_buf_lock);

        if (pin_buf == NULL)
            ret = mw_dma_memory_create_desc(
                        &dma,
                        MWCAP_VIDEO_MEMORY_TYPE_USER,
                        (unsigned long)upload_img->pvSrcFrame,
                        upload_img->cbSrcFrame,
                        MW_DMA_TO_DEVICE,
                        mwcap->dma_priv
                        );
        else
            dma = pin_buf->dma_desc;
    }
    if (ret != 0) {
        return ret;
    }

    mw_dma_memory_sync_for_device(dma);
    ret = xi_driver_sg_put_frame(
                mwcap->driver,
                xi_image_buffer_get_address(&_imgbuf->image_buf),
                xi_image_buffer_get_stride(&_imgbuf->image_buf),
                upload_img->xDest,
                upload_img->yDest,
                upload_img->cxDest,
                upload_img->cyDest,
                upload_img->cfDest,
                upload_img->quantRangeDest,
                upload_img->satRangeDest,
                dma->mwsg_list,
                dma->sglen,
                upload_img->bSrcBottomUp,
                upload_img->cbSrcStride,
                upload_img->cxSrc,
                upload_img->cySrc,
                upload_img->bSrcPixelAlpha,
                upload_img->bSrcPixelXBGR,
                1000
                );

    if (pin_buf != NULL) {
        os_spin_lock(mwcap->pin_buf_lock);
        pin_buf->is_busy = false;
        os_spin_unlock(mwcap->pin_buf_lock);
    } else
        mw_dma_memory_destroy_desc(dma);

    return ret;
}

static long mw_capture_upload_process(struct mw_stream_cap *mwcap, os_iocmd_t cmd, void * arg)
{
    struct shared_image_buf_manager *imgbuf_manager =
                xi_driver_get_imgbuf_manager(mwcap->driver);
    long ret = 0;

    switch (cmd) {
    case MWCAP_IOCTL_VIDEO_CREATE_IMAGE:
        {
            MWCAP_VIDEO_CREATE_IMAGE *par = (MWCAP_VIDEO_CREATE_IMAGE *)arg;
            struct shared_image_buf *pimage = NULL;
            struct shared_image_user_node *img_user = NULL;

            img_user = os_malloc(sizeof(struct shared_image_user_node));
            if (img_user == NULL) {
                ret = OS_RETURN_NOMEM;
                break;
            }

            pimage = shared_image_buf_create(imgbuf_manager, par->cx, par->cy);
            if (pimage == NULL) {
                ret = OS_RETURN_NOMEM;
                os_free(img_user);
                break;
            }

            img_user->shared_buf = pimage;

            par->pvImage = (MWCAP_PTR)(unsigned long)pimage;
            list_add_tail(&img_user->user_node, &mwcap->upload_buf_list);
        }
        break;

    case MWCAP_IOCTL_VIDEO_OPEN_IMAGE:
        {
            MWCAP_VIDEO_IMAGE_REF *par = (MWCAP_VIDEO_IMAGE_REF *)arg;
            struct shared_image_buf *pimage = NULL;
            struct shared_image_user_node *img_user = NULL;
            long ref_count = 0;

            /* this fd(task) has the image, need't open */
            pimage = (struct shared_image_buf *)(unsigned long)par->pvImage;
            if (_upload_imgbuf_find_user(&mwcap->upload_buf_list, pimage) != NULL) {
                ret = OS_RETURN_INVAL;
                break;
            }

            img_user = os_malloc(sizeof(struct shared_image_user_node));
            if (img_user == NULL) {
                ret = OS_RETURN_NOMEM;
                break;
            }

            if (!shared_image_buf_open(imgbuf_manager, pimage, &ref_count)) {
                ret = OS_RETURN_INVAL;
                os_free(img_user);
                break;
            }
            img_user->shared_buf = pimage;

            par->nRefCount = ref_count;
            list_add_tail(&img_user->user_node, &mwcap->upload_buf_list);
        }
        break;

    case MWCAP_IOCTL_VIDEO_CLOSE_IMAGE:
        {
            MWCAP_VIDEO_IMAGE_REF *par = (MWCAP_VIDEO_IMAGE_REF *)arg;
            struct shared_image_buf *pimage = NULL;
            struct shared_image_user_node *img_user = NULL;
            long ref_count = 0;

            pimage = (struct shared_image_buf *)(unsigned long)par->pvImage;
            img_user = _upload_imgbuf_find_user(&mwcap->upload_buf_list, pimage);
            if (img_user == NULL) {
                ret = OS_RETURN_INVAL;
                break;
            }

            list_del(&img_user->user_node);
            os_free(img_user);

            if (!shared_image_buf_close(imgbuf_manager, pimage, &ref_count)) {
                ret = OS_RETURN_INVAL;
                /* shared_image_buf_close should always sucess */
                os_print_err("BUG: shared_image_buf_close failed!\n");
            }

            par->nRefCount = ref_count;
        }
        break;

    case MWCAP_IOCTL_VIDEO_UPLOAD_IMAGE:
        ret = _upload_image(mwcap, (MWCAP_VIDEO_UPLOAD_IMAGE *)arg);
        break;

    default:
        break;
    }

    return ret;
}

bool mw_capture_ioctl(struct mw_stream_cap *mwcap, os_iocmd_t cmd,
                      void *arg, long *ret_val)
{
    long ret = 0;
    bool handled = true;

    switch (cmd) {

        /* timer */
    case MWCAP_IOCTL_TIMER_REGISTRATION:
    case MWCAP_IOCTL_TIMER_DEREGISTRATION:
    case MWCAP_IOCTL_TIMER_EXPIRE_TIME:
        os_spin_lock(mwcap->timer_lock);
        ret = mw_capture_timer_process(mwcap, cmd, arg);
        os_spin_unlock(mwcap->timer_lock);
        break;

        /* notify */
    case MWCAP_IOCTL_NOTIFY_REGISTRATION:
    case MWCAP_IOCTL_NOTIFY_DEREGISTRATION:
    case MWCAP_IOCTL_NOTIFY_STATUS:
    case MWCAP_IOCTL_NOTIFY_ENABLE:
        os_spin_lock(mwcap->notify_lock);
        ret = mw_capture_notify_process(mwcap, cmd, arg);
        os_spin_unlock(mwcap->notify_lock);
        break;

        /* video capture */
    case MWCAP_IOCTL_VIDEO_CAPTURE_OPEN:
    case MWCAP_IOCTL_VIDEO_CAPTURE_FRAME:
    case MWCAP_IOCTL_VIDEO_CAPTURE_STATUS:
    case MWCAP_IOCTL_VIDEO_CAPTURE_CLOSE:
        os_mutex_lock(mwcap->video_mutex);
        ret = mw_capture_video_process(mwcap, cmd, arg);
        os_mutex_unlock(mwcap->video_mutex);
        break;

        /* audio capture */
    case MWCAP_IOCTL_AUDIO_CAPTURE_OPEN:
    case MWCAP_IOCTL_AUDIO_CAPTURE_FRAME:
    case MWCAP_IOCTL_AUDIO_CAPTURE_CLOSE:
        os_mutex_lock(mwcap->audio_mutex);
        ret = mw_capture_audio_process(mwcap, cmd, arg);
        os_mutex_unlock(mwcap->audio_mutex);
        break;

    case MWCAP_IOCTL_VIDEO_CREATE_IMAGE:
    case MWCAP_IOCTL_VIDEO_OPEN_IMAGE:
    case MWCAP_IOCTL_VIDEO_CLOSE_IMAGE:
    case MWCAP_IOCTL_VIDEO_UPLOAD_IMAGE:
        os_mutex_lock(mwcap->upload_mutex);
        ret = mw_capture_upload_process(mwcap, cmd, arg);
        os_mutex_unlock(mwcap->upload_mutex);
        break;

    case MWCAP_IOCTL_VIDEO_PIN_BUFFER:
        {
            MWCAP_VIDEO_PIN_BUFFER *par = (MWCAP_VIDEO_PIN_BUFFER *)arg;

            os_mutex_lock(mwcap->video_mutex);
            ret = mw_capture_buffer_pin(mwcap,
                                        par->mem_type,
                                        par->pvBuffer, par->cbBuffer,
                                        MW_DMA_BIDIRECTIONAL);
            os_mutex_unlock(mwcap->video_mutex);
        }
        break;
    case MWCAP_IOCTL_VIDEO_UNPIN_BUFFER:
        os_mutex_lock(mwcap->video_mutex);
        ret = mw_capture_buffer_unpin(mwcap, *(MWCAP_PTR *)arg);
        os_mutex_unlock(mwcap->video_mutex);
        break;

    default:
        handled = false;
        break;
    }

    if (ret_val != NULL)
        *ret_val = ret;

    return handled;
}

int mw_capture_init(struct mw_stream_cap *mwcap, void *driver, int iChannel, os_dma_par_t dma_priv)
{
    int ret = 0;

    mwcap->m_iChannel = iChannel;

    /* timer & notify */
    mwcap->timer_lock = os_spin_lock_alloc();
    mwcap->notify_lock = os_spin_lock_alloc();
    if (mwcap->timer_lock == NULL || mwcap->notify_lock == NULL) {
        ret = OS_RETURN_NOMEM;
        goto tn_err;
    }
    INIT_LIST_HEAD(&mwcap->timer_list);
    INIT_LIST_HEAD(&mwcap->notify_list);

    /* pinned buffers */
    mwcap->pin_buf_lock = os_spin_lock_alloc();
    if (mwcap->pin_buf_lock == NULL) {
        ret = OS_RETURN_NOMEM;
        goto pin_err;
    }
    INIT_LIST_HEAD(&mwcap->pin_buf_list);

    /* video capture */
    mwcap->req_lock = os_spin_lock_alloc();
    mwcap->video_mutex = os_mutex_alloc();
    mwcap->req_event = os_event_alloc();
    if (mwcap->req_lock == NULL
            || mwcap->video_mutex == NULL
            || mwcap->req_event == NULL) {
        ret = OS_RETURN_NOMEM;
        goto video_err;
    }
    INIT_LIST_HEAD(&mwcap->req_list);
    INIT_LIST_HEAD(&mwcap->completed_list);

    /* audio capture */
    mwcap->audio_mutex = os_mutex_alloc();
    if (mwcap->audio_mutex == NULL) {
        ret = OS_RETURN_NOMEM;
        goto audio_err;
    }
    mwcap->audio_open = 0;

    /* video upload */
    mwcap->upload_mutex = os_mutex_alloc();
    if (mwcap->upload_mutex == NULL) {
        ret = OS_RETURN_NOMEM;
        goto upload_err;
    }
    INIT_LIST_HEAD(&mwcap->upload_buf_list);

    mwcap->driver = driver;
    mwcap->dma_priv = dma_priv;

    return 0;

upload_err:
    if (mwcap->upload_mutex != NULL) {
        os_mutex_free(mwcap->upload_mutex);
        mwcap->upload_mutex = NULL;
    }
audio_err:
    if (mwcap->audio_mutex != NULL) {
        os_mutex_free(mwcap->audio_mutex);
        mwcap->audio_mutex = NULL;
    }
video_err:
    if (mwcap->req_lock != NULL) {
        os_spin_lock_free(mwcap->req_lock);
        mwcap->req_lock = NULL;
    }
    if (mwcap->video_mutex != NULL) {
        os_mutex_free(mwcap->video_mutex);
        mwcap->video_mutex = NULL;
    }
    if (mwcap->req_event != NULL) {
        os_event_free(mwcap->req_event);
        mwcap->req_event = NULL;
    }
pin_err:
    if (mwcap->pin_buf_lock != NULL) {
        os_spin_lock_free(mwcap->pin_buf_lock);
        mwcap->pin_buf_lock = NULL;
    }
tn_err:
    if (mwcap->timer_lock != NULL) {
        os_spin_lock_free(mwcap->timer_lock);
        mwcap->timer_lock = NULL;
    }
    if (mwcap->notify_lock != NULL) {
        os_spin_lock_free(mwcap->notify_lock);
        mwcap->notify_lock = NULL;
    }
    return ret;
}

void mw_capture_deinit(struct mw_stream_cap *mwcap)
{
    xi_timer *timer;
    xi_notify_event *notify;
    struct shared_image_user_node *img_user = NULL;

    if (mwcap->upload_mutex == NULL
            || mwcap->audio_mutex == NULL
            || mwcap->req_lock == NULL
            || mwcap->video_mutex == NULL
            || mwcap->pin_buf_lock == NULL
            || mwcap->timer_lock == NULL
            || mwcap->notify_lock == NULL
            || mwcap->req_event == NULL) {
        return;
    }

    os_mutex_lock(mwcap->video_mutex);
    mwcap_stop_generating(mwcap);
    os_mutex_unlock(mwcap->video_mutex);

    os_spin_lock(mwcap->timer_lock);
    while (!list_empty(&mwcap->timer_list)) {
        timer = list_entry(mwcap->timer_list.next, xi_timer, user_node);
        if (timer != NULL) {
            list_del(&timer->user_node);
            xi_driver_delete_timer(mwcap->driver, timer);
        }
    }
    os_spin_unlock(mwcap->timer_lock);

    os_spin_lock(mwcap->notify_lock);
    while (!list_empty(&mwcap->notify_list)) {
        notify = list_entry(mwcap->notify_list.next, xi_notify_event, user_node);
        if (notify != NULL) {
            list_del(&notify->user_node);
            xi_driver_delete_notify_event(mwcap->driver, mwcap->m_iChannel, notify);
        }
    }
    os_spin_unlock(mwcap->notify_lock);

    os_mutex_lock(mwcap->upload_mutex);
    while (!list_empty(&mwcap->upload_buf_list)) {
        img_user = list_entry(mwcap->upload_buf_list.next, struct shared_image_user_node, user_node);
        if (img_user != NULL) {
            list_del(&img_user->user_node);
            shared_image_buf_close(
                        xi_driver_get_imgbuf_manager(mwcap->driver),
                        img_user->shared_buf, NULL);
            os_free(img_user);
        }
    }
    os_mutex_unlock(mwcap->upload_mutex);

    os_mutex_lock(mwcap->audio_mutex);
    mwcap_audio_capture_close(mwcap);
    os_mutex_unlock(mwcap->audio_mutex);

    /* unpin buffers */
    {
        struct mw_pin_buffer *pinbuf = NULL;

        os_spin_lock(mwcap->pin_buf_lock);
        while (!list_empty(&mwcap->pin_buf_list)) {
            pinbuf = list_entry(mwcap->pin_buf_list.next,
                                struct mw_pin_buffer, list_node);
            list_del(&pinbuf->list_node);

            os_spin_unlock(mwcap->pin_buf_lock);
            mw_dma_memory_destroy_desc(pinbuf->dma_desc);
            os_free(pinbuf);
            os_spin_lock(mwcap->pin_buf_lock);
        }
        os_spin_unlock(mwcap->pin_buf_lock);
    }

    os_mutex_free(mwcap->upload_mutex);
    mwcap->upload_mutex = NULL;
    os_mutex_free(mwcap->audio_mutex);
    mwcap->audio_mutex = NULL;
    os_spin_lock_free(mwcap->req_lock);
    mwcap->req_lock = NULL;
    os_mutex_free(mwcap->video_mutex);
    mwcap->video_mutex = NULL;
    os_event_free(mwcap->req_event);
    mwcap->req_event = NULL;
    os_spin_lock_free(mwcap->pin_buf_lock);
    mwcap->pin_buf_lock = NULL;
    os_spin_lock_free(mwcap->timer_lock);
    mwcap->timer_lock = NULL;
    os_spin_lock_free(mwcap->notify_lock);
    mwcap->notify_lock = NULL;
}
