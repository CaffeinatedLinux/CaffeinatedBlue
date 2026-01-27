////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#include "mw-stream.h"

#include "supports/osd-bound-rect.h"
#include "supports/math.h"
#include "supports/karray.h"
#include "dma/mw-dma-mem-priv.h"

#include "xi-driver-priv.h"

#include "picopng/picopng.h"

extern int loadPngImage(const char *filename, png_pix_info_t *dinfo);

static bool _is_image_buffer_settings_changed(
        struct mw_stream *stream,
        const struct OSD_IMAGE_SETTINGS *settings
        )
{
    bool changed = false;

    os_mutex_lock(stream->png_mutex);
    changed = (settings->cx != stream->png_dest_settings.cx
            || settings->cy != stream->png_dest_settings.cy
            || settings->colorFormat != stream->png_dest_settings.colorFormat
            || settings->quantRange != stream->png_dest_settings.quantRange
            || settings->satRange != stream->png_dest_settings.satRange
            );
    os_mutex_unlock(stream->png_mutex);

    return changed;
}
static void _get_image_buffer_settings(struct mw_stream *stream,
                                       struct OSD_IMAGE_SETTINGS *settings)
{
    os_spin_lock(stream->settings_lock);
    settings->colorFormat = stream->settings.process_settings.colorFormat;
    settings->quantRange = stream->settings.process_settings.quantRange;
    settings->satRange = stream->settings.process_settings.satRange;
    os_spin_unlock(stream->settings_lock);

    os_spin_lock(stream->format_lock);
    settings->cx = (int)stream->format.cx;
    settings->cy = (int)stream->format.cy;
    if (settings->colorFormat == MWCAP_VIDEO_COLOR_FORMAT_UNKNOWN)
        settings->colorFormat = stream->format.colorFormat;
    if (settings->quantRange == MWCAP_VIDEO_QUANTIZATION_UNKNOWN)
        settings->quantRange = stream->format.quantRange;
    if (settings->satRange == MWCAP_VIDEO_SATURATION_UNKNOWN)
        settings->satRange = stream->format.satRange;
    os_spin_unlock(stream->format_lock);
}

static int _load_png_image(struct mw_stream *stream)
{
    bool ret;
    png_pix_info_t png_info;

    ret = loadPngImage(stream->png_settings.szPNGFilePath, &png_info);
    if (ret != 0 || karray_is_empty(&png_info.arr_image))
        return ret;

    if (png_info.width < 176 || png_info.height < 120) {
        karray_free(&png_info.arr_image);
        return OS_RETURN_INVAL;
    }

    os_memcpy(&stream->png_data.png_info, &png_info, sizeof(stream->png_data.png_info));
    stream->png_data.rect_count = GetOSDBoundRects(
                karray_get_data(&stream->png_data.png_info.arr_image),
                stream->png_data.png_info.width,
                stream->png_data.png_info.height,
                stream->png_data.png_info.width * 4,
                stream->png_data.rects);

    return 0;
}

static void _free_png_image(struct mw_stream *stream)
{
    karray_free(&stream->png_data.png_info.arr_image);

    stream->png_data.png_info.width = 0;
    stream->png_data.png_info.height = 0;
    stream->png_data.rect_count = 0;
}

static int _update_osd_image(struct mw_stream *stream)
{
    int ret = 0;
    bool is_create_imgbuf = false;
    struct OSD_IMAGE_SETTINGS image_settings;
    struct mw_dma_desc *dma;
    int i;

    _get_image_buffer_settings(stream, &image_settings);

    ret = mw_dma_memory_create_desc(
                &dma,
                MWCAP_VIDEO_MEMORY_TYPE_KERNEL,
                (unsigned long)karray_get_data(&stream->png_data.png_info.arr_image),
                karray_get_count(&stream->png_data.png_info.arr_image),
                MW_DMA_TO_DEVICE,
                stream->mw_dev->dma_priv
                );
    if (ret != 0) {
        return ret;
    }

    os_mutex_lock(stream->osd_image.mutex);
    if (!_osd_image_is_valid(&stream->osd_image))
        is_create_imgbuf = true;
    else {
        if (image_settings.cx != stream->png_dest_settings.cx
                || image_settings.cy != stream->png_dest_settings.cy) {
            xi_image_buffer_destroy(&stream->osd_image.imgbuf);
            is_create_imgbuf = true;
        }
    }

    if (is_create_imgbuf) {
        if (!xi_image_buffer_create(
                    &stream->osd_image.imgbuf,
                    xi_driver_get_onboard_heap(stream->driver),
                    (u32)image_settings.cx,
                    (u32)image_settings.cy,
                    (u32)image_settings.cx * 4)) {
            ret = OS_RETURN_NOMEM;
            goto out;
        }
    }

    mw_dma_memory_sync_for_device(dma);
    ret = xi_driver_sg_put_frame(
                stream->driver,
                xi_image_buffer_get_address(&stream->osd_image.imgbuf),
                xi_image_buffer_get_stride(&stream->osd_image.imgbuf),
                0,
                0,
                xi_image_buffer_get_width(&stream->osd_image.imgbuf),
                xi_image_buffer_get_height(&stream->osd_image.imgbuf),
                image_settings.colorFormat,
                image_settings.quantRange,
                image_settings.satRange,
                dma->mwsg_list,
                dma->sglen,
                FALSE,
                stream->png_data.png_info.width * 4,
                stream->png_data.png_info.width,
                stream->png_data.png_info.height,
                TRUE,
                FALSE,
                1000
                );

    if (ret != 0)
        goto out;

    stream->osd_image.rect_count = stream->png_data.rect_count;
    os_memcpy(stream->osd_image.rects, stream->png_data.rects, sizeof(RECT) * stream->osd_image.rect_count);

    for (i = 0; i < stream->osd_image.rect_count; i++) {
        ScaleRect(&stream->osd_image.rects[i],
                  stream->png_data.png_info.width,
                  stream->png_data.png_info.height,
                  image_settings.cx, image_settings.cy);
    }

    os_memcpy(&stream->png_dest_settings, &image_settings, sizeof(stream->png_dest_settings));

out:
    mw_dma_memory_destroy_desc(dma);
    os_mutex_unlock(stream->osd_image.mutex);
    return ret;
}

static void _free_osd_image(struct mw_stream *stream)
{
    os_mutex_lock(stream->osd_image.mutex);
    xi_image_buffer_destroy(&stream->osd_image.imgbuf);
    stream->osd_image.rect_count = 0;
    os_mutex_unlock(stream->osd_image.mutex);
}

static long mwcap_ioctl_stream(struct mw_stream *stream, os_iocmd_t cmd, void *arg)
{
    struct mw_device *mw_dev = stream->mw_dev;
    long ret = 0;

    switch (cmd) {
    case MWCAP_IOCTL_GET_STREAMS_COUNT:
        *(int *)arg = mw_dev->stream_count;
        break;
    case MWCAP_IOCTL_GET_STREAMS_INFO:
        {
            MWCAP_STREAMS_INFO *info = (MWCAP_STREAMS_INFO *)arg;
            MWCAP_STREAM_INFO *data;
            int count;
            int i;
            struct mw_stream *_stream = NULL;
            struct list_head *pos;

            count = min(mw_dev->stream_count, info->count);
            if (count < 0) {
                ret = OS_RETURN_INVAL;
                break;
            }

            /* os_zalloc may be sleep */
            os_spin_unlock(stream->mw_dev->list_lock);
            data = os_zalloc(sizeof(MWCAP_STREAM_INFO) * count);
            os_spin_lock(stream->mw_dev->list_lock);
            if (data == NULL) {
                ret = OS_RETURN_NOMEM;
                break;
            }

            i = 0;
            if (!list_empty(&mw_dev->stream_list)) {
                list_for_each(pos, &mw_dev->stream_list) {
                    _stream = list_entry(pos, struct mw_stream, list_node);
                    data[i].stream_id = _stream->stream_id;
                    data[i].pid = _stream->pid;
                    os_strlcpy(data[i].comm, _stream->proc_name, MW_MAX_PROCESS_NAME_LEN);
#if defined(__linux__)
                    data[i].streaming = os_local_stream_is_generating(_stream);
#else
                    data[i].streaming = mw_stream_is_local(_stream) && mw_capture_is_generating(&_stream->s_mwcap);
#endif
                    i++;
                    if (i >= count) {
                        break;
                    }
                }
            } else {
                os_free(data);
                ret = OS_RETURN_ERROR;
                break;
            }

            info->count = count;
            if (os_copy_out((os_user_addr_t)(unsigned long)info->infos, data, sizeof(MWCAP_STREAM_INFO) * count) != 0)
                ret = OS_RETURN_ERROR;

            os_free(data);
        }
        break;
    case MWCAP_IOCTL_SET_CTRL_STREAM_ID:
        {
            int ctrl_id = *(int *)arg;
            struct mw_stream *_stream = NULL;
            struct list_head *pos;

            if (ctrl_id == 0 || ctrl_id == stream->stream_id) {
                stream->ctrl_id = stream->stream_id;
                stream->ctrl_stream = NULL;
            }

            ret = OS_RETURN_INVAL;
            /* check if we have the ctrl_id */
            if (!list_empty(&mw_dev->stream_list)) {
                list_for_each(pos, &mw_dev->stream_list) {
                    _stream = list_entry(pos, struct mw_stream, list_node);
                    if (_stream->stream_id == ctrl_id) {
                        stream->ctrl_id = ctrl_id;
                        stream->ctrl_stream = _stream;
                        ret = 0;
                        break;
                    }
                }
            }
        }
        break;
    case MWCAP_IOCTL_GET_CTRL_STREAM_ID:
        *(int *)arg = stream->ctrl_id;
        break;
    case MWCAP_IOCTL_GET_SELF_STREAM_ID:
        *(int *)arg = stream->stream_id;
        break;
    }

    return ret;
}

static bool mwcap_ioctl_nolock(struct mw_stream *stream, os_iocmd_t cmd,
            void *arg, long *retval)
{
    long ret = 0;
    bool handled = true;

    switch (cmd) {
    case MWCAP_IOCTL_GET_CHANNEL_INFO:
        xi_driver_get_channel_info(stream->driver, stream->s_mwcap.m_iChannel, (MWCAP_CHANNEL_INFO *)arg);
        break;
    case MWCAP_IOCTL_GET_FAMILY_INFO:
        xi_driver_get_family_info(stream->driver, (MWCAP_PRO_CAPTURE_INFO *)arg);
        break;

    case MWCAP_IOCTL_GET_VIDEO_CAPS:
        xi_driver_get_video_caps(stream->driver, (MWCAP_VIDEO_CAPS *)arg);
        break;
    case MWCAP_IOCTL_GET_AUDIO_CAPS:
        xi_driver_get_audio_caps(stream->driver, (MWCAP_AUDIO_CAPS *)arg);
        break;

    case MWCAP_IOCTL_GET_VIDEO_BUFFER_INFO:
        xi_driver_get_video_buffer_info(stream->driver, stream->s_mwcap.m_iChannel, (MWCAP_VIDEO_BUFFER_INFO *)arg);
        break;
    case MWCAP_IOCTL_GET_VIDEO_FRAME_INFO:
        {
            MWCAP_VIDEO_FRAME_INFO_PAR *par = (MWCAP_VIDEO_FRAME_INFO_PAR *)arg;

            xi_driver_get_video_frame_info(stream->driver, stream->s_mwcap.m_iChannel, (int)par->iframe, &par->info);
        }
        break;

        /* input source*/
    case MWCAP_IOCTL_VIDEO_INPUT_SOURCE_COUNT:
        xi_driver_get_supported_video_input_sources(stream->driver, (int *)arg);
        break;
    case MWCAP_IOCTL_VIDEO_INPUT_SOURCE_ARRAY:
        {
            MWCAP_INPUT_SOURCE_ARRAY *array = (MWCAP_INPUT_SOURCE_ARRAY *)arg;
            const u32 *data;
            int count;

            data = xi_driver_get_supported_video_input_sources(
                    stream->driver, &count);

            if (array->count < count) {
                ret = OS_RETURN_INVAL;
                break;
            }

            if (os_copy_out((os_user_addr_t)(unsigned long)array->data, data, sizeof(unsigned int) * count) != 0) {
                ret = OS_RETURN_ERROR;
                break;
            }

            array->count = count;
        }
        break;
    case MWCAP_IOCTL_AUDIO_INPUT_SOURCE_COUNT:
        xi_driver_get_supported_audio_input_sources(stream->driver, (int *)arg);
        break;
    case MWCAP_IOCTL_AUDIO_INPUT_SOURCE_ARRAY:
        {
            MWCAP_INPUT_SOURCE_ARRAY *array = (MWCAP_INPUT_SOURCE_ARRAY *)arg;
            const u32 *data;
            int count;

            data = xi_driver_get_supported_audio_input_sources(
                    stream->driver, &count);

            if (array->count < count) {
                ret = OS_RETURN_INVAL;
                break;
            }

            if (os_copy_out((os_user_addr_t)(unsigned long)array->data, data, sizeof(unsigned int) * count) != 0) {
                ret = OS_RETURN_ERROR;
                break;
            }

            array->count = count;
        }
        break;

        /* signal status */
    case MWCAP_IOCTL_GET_INPUT_SPECIFIC_STATUS:
        {
            INPUT_SPECIFIC_STATUS status;

            xi_driver_get_input_specific_status(stream->driver, stream->s_mwcap.m_iChannel, &status);

            os_memcpy(arg, &status.pubStatus, sizeof(status.pubStatus));
        }
        break;
    case MWCAP_IOCTL_GET_VIDEO_SIGNAL_STATUS:
        {
            VIDEO_SIGNAL_STATUS status;

            xi_driver_get_video_signal_status(stream->driver, stream->s_mwcap.m_iChannel, &status);

            os_memcpy(arg, &status.pubStatus, sizeof(status.pubStatus));
        }
        break;
    case MWCAP_IOCTL_GET_AUDIO_SIGNAL_STATUS:
        {
            AUDIO_SIGNAL_STATUS status;

            xi_driver_get_audio_signal_status(stream->driver, stream->s_mwcap.m_iChannel, &status);

            os_memcpy(arg, &status.pubStatus, sizeof(status.pubStatus));
        }
        break;

        /* HDMI status */
    case MWCAP_IOCTL_GET_HDMI_INFOFRAME_VALID:
        *(unsigned int *)arg =
                xi_driver_get_hdmi_infoframe_valid_flags(stream->driver);
        break;
    case MWCAP_IOCTL_GET_HDMI_INFOFRAME_PACKET:
        {
            MWCAP_HDMI_INFOFRAME_PACKET *packet = (MWCAP_HDMI_INFOFRAME_PACKET *)arg;

            ret = xi_driver_get_hdmi_infoframe_packet(stream->driver,
                    packet->id, &packet->pkt);
            if (ret != 0)
                break;
        }
        break;

        /* SDI signal */
    case MWCAP_IOCTL_GET_SDI_LINK_STATUS:
        {
            MWCAP_SDI_SINGLE_LINK_STATUS status;
            status.byPortIndex = ((MWCAP_SDI_SINGLE_LINK_STATUS *)arg)->byPortIndex;

            xi_driver_get_sdi_link_status(stream->driver, stream->s_mwcap.m_iChannel, &status);

            os_memcpy(arg, &status, sizeof(status));
        }
        break;
    case MWCAP_IOCTL_GET_SDI_QUAD_LINK_MANUAL_MODE:
        {
            int nMode;

            nMode = xi_driver_get_sdi_quad_link_manual_mode(stream->driver, stream->s_mwcap.m_iChannel);

            *(int *)arg = nMode;
        }
        break;
    case MWCAP_IOCTL_SET_SDI_QUAD_LINK_MANUAL_MODE:
        {
            int nMode = *(int *)arg;

            ret = xi_driver_set_sdi_quad_link_manual_mode(stream->driver, stream->s_mwcap.m_iChannel, nMode);
        }
        break;

        /* device no lock */
    case MWCAP_IOCTL_SET_LED_MODE:
        xi_driver_set_led_mode(stream->driver, *(unsigned int *)arg);
        break;
    case MWCAP_IOCTL_SET_POST_RECONFIG:
        xi_driver_set_post_reconfig_delay(stream->driver, *(unsigned int*)arg);
        break;

    case MWCAP_IOCTL_GET_CORE_TEMPERATURE:
        *(unsigned int *)arg = (unsigned int)xi_driver_get_core_temperature(stream->driver);
        break;

        /* time */
    case MWCAP_IOCTL_GET_TIME:
        *(long long *)arg = xi_driver_get_time(stream->driver);
        break;
    case MWCAP_IOCTL_SET_TIME:
        xi_driver_set_time(stream->driver, *(long long*)arg);
        break;
    case MWCAP_IOCTL_TIME_REGULATION:
        xi_driver_update_time(stream->driver, *(long long*)arg);
        break;


        /* parameter no lock */
    case MWCAP_IOCTL_SET_INPUT_SOURCE_SCAN:
        xi_driver_set_input_source_scan(stream->driver, *(bool *)arg);
        break;
    case MWCAP_IOCTL_GET_INPUT_SOURCE_SCAN:
        *(bool *)arg = xi_driver_get_input_source_scan(stream->driver);
        break;
    case MWCAP_IOCTL_GET_INPUT_SOURCE_SCAN_STATE:
        *(bool *)arg = xi_driver_get_input_source_scan_state(stream->driver);
        break;
    case MWCAP_IOCTL_SET_AV_INPUT_SOURCE_LINK:
        xi_driver_set_av_input_source_link(stream->driver, *(bool *)arg);
        break;
    case MWCAP_IOCTL_GET_AV_INPUT_SOURCE_LINK:
        *(bool *)arg = xi_driver_get_av_input_source_link(stream->driver);
        break;
    case MWCAP_IOCTL_SET_VIDEO_INPUT_SOURCE:
        xi_driver_set_video_input_source(stream->driver, *(unsigned int *)arg);
        break;
    case MWCAP_IOCTL_GET_VIDEO_INPUT_SOURCE:
        *(unsigned int *)arg = xi_driver_get_video_input_source(stream->driver);
        break;
    case MWCAP_IOCTL_SET_AUDIO_INPUT_SOURCE:
        xi_driver_set_audio_input_source(stream->driver, *(unsigned int *)arg);
        break;
    case MWCAP_IOCTL_GET_AUDIO_INPUT_SOURCE:
        *(unsigned int *)arg = xi_driver_get_audio_input_source(stream->driver);
        break;

        /* EDID */
    case MWCAP_IOCTL_GET_EDID_DATA:
        {
            MWCAP_EDID_DATA *edid = (MWCAP_EDID_DATA *)arg;
            int size;
            const u8 *data;

            data = xi_driver_get_edid(stream->driver, &size);

            if (edid->size < size) {
                ret = OS_RETURN_INVAL;
                break;
            }

            if (os_copy_out((os_user_addr_t)(unsigned long)edid->data, data, size) != 0) {
                ret = OS_RETURN_ERROR;
                break;
            }

            edid->size = size;
        }
        break;
    case MWCAP_IOCTL_SET_EDID_DATA:
        {
            MWCAP_EDID_DATA *edid = (MWCAP_EDID_DATA *)arg;
            u8 *data;

            if (edid->size <= 0) {
                ret = OS_RETURN_INVAL;
                break;
            }

            data = os_zalloc(edid->size);
            if (data == NULL) {
                ret = OS_RETURN_NOMEM;
                break;
            }

            if (os_copy_in(data, (os_user_addr_t)(unsigned long)edid->data, edid->size) != 0) {
                ret = OS_RETURN_ERROR;
                os_free(data);
                break;
            }

            ret = xi_driver_set_edid(stream->driver, data, edid->size);

            os_free(data);
        }
        break;

        /* video processing */
    case MWCAP_IOCTL_GET_VIDEO_INPUT_ASPECT_RATIO:
        xi_driver_get_video_input_aspect_ratio(stream->driver, (MWCAP_VIDEO_ASPECT_RATIO *)arg);
        break;
    case MWCAP_IOCTL_SET_VIDEO_INPUT_ASPECT_RATIO:
        xi_driver_set_video_input_aspect_ratio(stream->driver, (MWCAP_VIDEO_ASPECT_RATIO *)arg);
        break;
    case MWCAP_IOCTL_GET_VIDEO_INPUT_COLOR_FORMAT:
        *(MWCAP_VIDEO_COLOR_FORMAT *)arg =
                xi_driver_get_video_input_color_format(stream->driver);
        break;
    case MWCAP_IOCTL_SET_VIDEO_INPUT_COLOR_FORMAT:
        xi_driver_set_video_input_color_format(
                    stream->driver, *(MWCAP_VIDEO_COLOR_FORMAT *)arg);
        break;
    case MWCAP_IOCTL_GET_VIDEO_INPUT_QUANTIZATION_RANGE:
        *(MWCAP_VIDEO_QUANTIZATION_RANGE *)arg =
                xi_driver_get_video_input_quantization_range(stream->driver);
        break;
    case MWCAP_IOCTL_SET_VIDEO_INPUT_QUANTIZATION_RANGE:
        xi_driver_set_video_input_quantization_range(
                    stream->driver, *(MWCAP_VIDEO_QUANTIZATION_RANGE *)arg);
        break;

        /* VGA/Component timings */
    case MWCAP_IOCTL_GET_VIDEO_AUTO_H_ALIGN:
        *(bool *)arg = xi_driver_get_video_auto_h_align(stream->driver);
        break;
    case MWCAP_IOCTL_SET_VIDEO_AUTO_H_ALIGN:
        ret = xi_driver_set_video_auto_h_align(stream->driver, *(bool *)arg);
        break;
    case MWCAP_IOCTL_GET_VIDEO_SAMPLING_PHASE:
        *(u8 *)arg =
                xi_driver_get_video_sampling_phase(stream->driver);
        break;
    case MWCAP_IOCTL_SET_VIDEO_SAMPLING_PHASE:
        ret = xi_driver_set_video_sampling_phase(stream->driver, *(u8 *)arg);
        break;
    case MWCAP_IOCTL_GET_VIDEO_SAMPLING_PHASE_AUTO:
        *(bool *)arg = xi_driver_get_video_sampling_phase_auto(stream->driver);
        break;
    case MWCAP_IOCTL_SET_VIDEO_SAMPLING_PHASE_AUTO:
        ret = xi_driver_set_video_sampling_phase_auto(stream->driver, *(bool *)arg);
        break;
    case MWCAP_IOCTL_SET_VIDEO_TIMING:
        ret = xi_driver_set_video_timing(stream->driver, (MWCAP_VIDEO_TIMING *)arg);
        break;
    case MWCAP_IOCTL_GET_VIDEO_PREFERRED_TIMING_ARRAY:
        {
            MWCAP_VIDEO_TIMING timings[MWCAP_VIDEO_MAX_NUM_PREFERRED_TIMINGS];
            int count = 0;
            MWCAP_VIDEO_TIMING_PAR *par = (MWCAP_VIDEO_TIMING_PAR *)arg;

            count = xi_driver_get_video_preferred_timing_array(stream->driver, timings);
            if (count < 0) {
                ret = count;
                break;
            }

            if (count > 0
                    && os_copy_out((os_user_addr_t)(unsigned long)par->timings, timings,
                                   sizeof(MWCAP_VIDEO_TIMING) * min(count, par->count)) != 0) {
                ret = OS_RETURN_ERROR;
                break;
            }

            par->count = count;
        }
        break;
    case MWCAP_IOCTL_SET_VIDEO_CUSTOM_TIMING:
        ret = xi_driver_set_video_custom_timing(stream->driver, (MWCAP_VIDEO_CUSTOM_TIMING *)arg);
        break;
    case MWCAP_IOCTL_GET_VIDEO_CUSTOM_TIMING_COUNT:
        *(u32 *)arg = xi_driver_get_video_custom_timing_count(stream->driver);
        break;
    case MWCAP_IOCTL_GET_VIDEO_CUSTOM_TIMING_ARRAY:
        {
            MWCAP_VIDEO_CUSTOM_TIMING_PAR *par = (MWCAP_VIDEO_CUSTOM_TIMING_PAR *)arg;
            const MWCAP_VIDEO_CUSTOM_TIMING *timings;
            int count;

            timings = xi_driver_get_video_custom_timing_array(
                        stream->driver, &count);

            count = min(count, par->count);

            if (count > 0 &&
                    os_copy_out((os_user_addr_t)(unsigned long)par->timings, timings,
                                count * sizeof(MWCAP_VIDEO_CUSTOM_TIMING)) != 0) {
                ret = OS_RETURN_ERROR;
                break;
            }

            par->count = count;
        }
        break;
    case MWCAP_IOCTL_SET_VIDEO_CUSTOM_TIMING_ARRAY:
        {
            MWCAP_VIDEO_CUSTOM_TIMING_PAR *par = (MWCAP_VIDEO_CUSTOM_TIMING_PAR *)arg;
            MWCAP_VIDEO_CUSTOM_TIMING *timings;

            if (par->count < 0) {
                ret = OS_RETURN_INVAL;
                break;
            }

            // clear array
            if (par->count == 0) {
                ret = xi_driver_set_video_custom_timing_array(
                            stream->driver, NULL, 0);
                break;
            }

            timings = os_malloc(par->count * sizeof(MWCAP_VIDEO_CUSTOM_TIMING));
            if (timings == NULL) {
                ret = OS_RETURN_NOMEM;
                break;
            }

            if (os_copy_in(timings, (os_user_addr_t)(unsigned long)par->timings,
                           sizeof(MWCAP_VIDEO_CUSTOM_TIMING) * par->count) != 0) {
                ret = OS_RETURN_ERROR;
                os_free(timings);
                break;
            }

            ret = xi_driver_set_video_custom_timing_array(
                        stream->driver, timings, par->count);
            os_free(timings);
        }
        break;
    case MWCAP_IOCTL_GET_VIDEO_CUSTOM_RESOLUTION_COUNT:
        *(u32 *)arg = xi_driver_get_video_custom_resolution_count(stream->driver);
        break;
    case MWCAP_IOCTL_GET_VIDEO_CUSTOM_RESOLUTION_ARRAY:
        {
            MWCAP_VIDEO_CUSTOM_RESOLUTION_PAR *par =
                    (MWCAP_VIDEO_CUSTOM_RESOLUTION_PAR *)arg;
            const MWCAP_SIZE *resolutions;
            int count;

            resolutions = xi_driver_get_video_custom_resolution_array(
                        stream->driver, &count);

            count = min(count, par->count);

            if (count > 0
                    && os_copy_out((os_user_addr_t)(unsigned long)par->resolutions, resolutions,
                                   count * sizeof(MWCAP_SIZE)) != 0) {
                ret = OS_RETURN_ERROR;
                break;
            }

            par->count = count;
        }
        break;
    case MWCAP_IOCTL_SET_VIDEO_CUSTOM_RESOLUTION_ARRAY:
        {
            MWCAP_VIDEO_CUSTOM_RESOLUTION_PAR *par =
                    (MWCAP_VIDEO_CUSTOM_RESOLUTION_PAR *)arg;
            MWCAP_SIZE *resolutions;

            if (par->count < 0) {
                ret = OS_RETURN_INVAL;
                break;
            }

            // clear array
            if (par->count == 0) {
                ret = xi_driver_set_video_custom_resolution_array(
                            stream->driver, NULL, 0);
                break;
            }

            resolutions = os_malloc(par->count * sizeof(MWCAP_SIZE));
            if (resolutions == NULL) {
                ret = OS_RETURN_NOMEM;
                break;
            }

            if (os_copy_in(resolutions, (os_user_addr_t)(unsigned long)par->resolutions,
                           sizeof(MWCAP_SIZE) * par->count) != 0) {
                ret = OS_RETURN_ERROR;
                break;
            }

            ret = xi_driver_set_video_custom_resolution_array(
                        stream->driver, resolutions, par->count);
            os_free(resolutions);
        }
        break;

    case MWCAP_IOCTL_SETTINGS_SAVE_AS_PRESET:
        os_spin_lock(stream->mw_dev->settings_lock);
        os_memcpy(&stream->mw_dev->settings.process_settings,
               &stream->settings.process_settings,
               sizeof(stream->settings.process_settings));
        os_memcpy(&stream->mw_dev->png_settings, &stream->png_settings,
               sizeof(stream->png_settings));

        stream->mw_dev->settings.contrast = stream->settings.contrast;
        stream->mw_dev->settings.brightness = stream->settings.brightness;
        stream->mw_dev->settings.saturation = stream->settings.saturation;
        stream->mw_dev->settings.hue = stream->settings.hue;
        os_spin_unlock(stream->mw_dev->settings_lock);
        break;
    case MWCAP_IOCTL_SETTINGS_RELOAD_PRESET:
        os_spin_lock(stream->mw_dev->settings_lock);
        os_memcpy(&stream->settings.process_settings,
               &stream->mw_dev->settings.process_settings,
               sizeof(stream->settings.process_settings));
        os_memcpy(&stream->png_settings, &stream->mw_dev->png_settings,
               sizeof(stream->png_settings));

        stream->settings.contrast = stream->mw_dev->settings.contrast;
        stream->settings.brightness = stream->mw_dev->settings.brightness;
        stream->settings.saturation = stream->mw_dev->settings.saturation;
        stream->settings.hue = stream->mw_dev->settings.hue;
        os_spin_unlock(stream->mw_dev->settings_lock);
        break;


    case MWCAP_IOCTL_SET_SDI_ANC_TYPE:
        xi_driver_set_sdianc_type(stream->driver, (MWCAP_SDI_ANC_TYPE *)arg);
        break;
    case MWCAP_IOCTL_GET_SDI_ANC_PACKET:
        xi_driver_get_sdianc_packet(stream->driver, (MWCAP_SDI_ANC_PACKET *)arg);
        break;

    case MWCAP_IOCTL_GET_V4L2_FRAME_SDI_ANC_PACKETS:
    {
        MWCAP_SDI_ANC_PACKETS_PAR *par = (MWCAP_SDI_ANC_PACKETS_PAR *)arg;
        MWCAP_SDI_ANC_PACKET *anc_packets;
        int anc_packet_count;

        if (par->count < 0) {
            ret = OS_RETURN_INVAL;
            break;
        }

        anc_packet_count = xi_v4l2_get_last_frame_sdianc_data(stream,
                                                              &anc_packets);

        if (anc_packet_count <= 0) {
            ret = OS_RETURN_NODATA;
            break;
        }

        anc_packet_count = (anc_packet_count < par->count) ?
                               anc_packet_count : par->count;
        if (os_copy_out((os_user_addr_t)(unsigned long)par->packets,
            anc_packets, sizeof(MWCAP_SDI_ANC_PACKET) * anc_packet_count)) {
            ret = OS_RETURN_ERROR;
            break;
        }

        par->count = anc_packet_count;
    }
        break;
    case MWCAP_IOCTL_GET_ENABLE_V4L2_FRAME_SDI_ANC:
        *((BOOLEAN *)arg) = xi_driver_GetEnableSystemSDIANC(stream->driver);
        break;
    case MWCAP_IOCTL_SET_ENABLE_V4L2_FRAME_SDI_ANC:
        xi_driver_SetEnableSystemSDIANC(stream->driver, *((BOOLEAN *)arg));
        break;

    default:
        handled = false;
        break;
    }

    if (retval != NULL)
        *retval = ret;

    return handled;
}

static long mwcap_ioctl_settings(struct frame_settings *settings, os_iocmd_t cmd, void *arg)
{
    long ret = 0;

    switch (cmd) {
    case MWCAP_IOCTL_GET_BRIGHTNESS:
        *(int *)arg = settings->brightness;
        break;
    case MWCAP_IOCTL_SET_BRIGHTNESS:
        settings->brightness = *(int *)arg;
        break;

    case MWCAP_IOCTL_GET_CONTRAST:
        *(int *)arg = settings->contrast;
        break;
    case MWCAP_IOCTL_SET_CONTRAST:
        settings->contrast = *(int *)arg;
        break;

    case MWCAP_IOCTL_GET_HUE:
        *(int *)arg = settings->hue;
        break;
    case MWCAP_IOCTL_SET_HUE:
        settings->hue = *(int *)arg;
        break;

    case MWCAP_IOCTL_GET_SATURATION:
        *(int *)arg = settings->saturation;
        break;
    case MWCAP_IOCTL_SET_SATURATION:
        settings->saturation = *(int *)arg;
        break;
    }

    return ret;
}

static long mwcap_ioctl_osd_process(struct mw_stream *stream, os_iocmd_t cmd, void *arg)
{
    long ret = 0;

    switch (cmd) {
        /* process settings & osd, for v4l2 */
    case MWCAP_IOCTL_GET_VIDEO_CONNECTION_FORMAT:
        os_spin_lock(stream->format_lock);
        os_memcpy(arg, &stream->format, sizeof(MWCAP_VIDEO_CONNECTION_FORMAT));
        os_spin_unlock(stream->format_lock);
        break;

    case MWCAP_IOCTL_GET_VIDEO_PROCESS_SETTINGS:
        os_spin_lock(stream->settings_lock);
        os_memcpy(arg, &stream->settings.process_settings, sizeof(MWCAP_VIDEO_PROCESS_SETTINGS));
        os_spin_unlock(stream->settings_lock);
        break;
    case MWCAP_IOCTL_SET_VIDEO_PROCESS_SETTINGS:
        {
            MWCAP_VIDEO_PROCESS_SETTINGS *par = (MWCAP_VIDEO_PROCESS_SETTINGS *)arg;

            os_mutex_lock(stream->png_mutex);

            os_spin_lock(stream->settings_lock);
            os_memcpy(&stream->settings.process_settings, par, sizeof(*par));
            os_spin_unlock(stream->settings_lock);

            if (_osd_image_is_valid(&stream->osd_image)) {
                struct OSD_IMAGE_SETTINGS image_settings;

                _get_image_buffer_settings(stream, &image_settings);
                if (_is_image_buffer_settings_changed(stream, &image_settings))
                    _update_osd_image(stream);
            }

            os_mutex_unlock(stream->png_mutex);
        }
        break;

    case MWCAP_IOCTL_GET_VIDEO_OSD_SETTINGS:
        {
            os_mutex_lock(stream->png_mutex);
            os_memcpy(arg, &stream->png_settings, sizeof(MWCAP_VIDEO_OSD_SETTINGS));
            os_mutex_unlock(stream->png_mutex);
        }
        break;
    case MWCAP_IOCTL_SET_VIDEO_OSD_SETTINGS:
        {
            MWCAP_VIDEO_OSD_SETTINGS *par = (MWCAP_VIDEO_OSD_SETTINGS *)arg;

            os_mutex_lock(stream->png_mutex);
            os_memcpy(&stream->png_settings, par, sizeof(MWCAP_VIDEO_OSD_SETTINGS));
            _free_png_image(stream);
            if (stream->png_settings.bEnable) {
                ret = _load_png_image(stream);

                if (ret == 0)
                    ret = _update_osd_image(stream);
            }

            if (!stream->png_settings.bEnable || ret != 0) {
                _free_png_image(stream);
                _free_osd_image(stream);
            }
            os_mutex_unlock(stream->png_mutex);
        }
        break;

    case MWCAP_IOCTL_GET_VIDEO_OSD_IMAGE:
        {
            os_mutex_lock(stream->osd_image.mutex);
            os_memcpy(arg, &stream->osd_image.user_image, sizeof(MWCAP_VIDEO_OSD_IMAGE));
            os_mutex_unlock(stream->osd_image.mutex);
        }
        break;
    case MWCAP_IOCTL_SET_VIDEO_OSD_IMAGE:
        {
            MWCAP_VIDEO_OSD_IMAGE *par = (MWCAP_VIDEO_OSD_IMAGE *)arg;

            if (par->pvOSDImage != 0
                    && !shared_image_buf_open(
                        xi_driver_get_imgbuf_manager(stream->driver),
                        (struct shared_image_buf *)(unsigned long)par->pvOSDImage,
                        NULL)) {
                ret = OS_RETURN_INVAL;
                break;
            }

            os_mutex_lock(stream->osd_image.mutex);
            if (stream->osd_image.user_image.pvOSDImage)
                shared_image_buf_close(
                            xi_driver_get_imgbuf_manager(stream->driver),
                            (struct shared_image_buf *)(unsigned long)stream->osd_image.user_image.pvOSDImage,
                            NULL);
            os_memcpy(&stream->osd_image.user_image, par, sizeof(MWCAP_VIDEO_OSD_IMAGE));
            os_mutex_unlock(stream->osd_image.mutex);
        }
        break;

    default:
        break;
    }

    return ret;
}

static long mwcap_ioctl_devlock(struct mw_stream *stream, os_iocmd_t cmd, void *arg)
{
    void *driver = stream->driver;
    long ret = 0;

    switch (cmd) {
        /* firmware lock */
    case MWCAP_IOCTL_GET_FIRMWARE_STORAGE:
        ret = xi_driver_get_firmware_storage_info(driver, (MWCAP_FIRMWARE_STORAGE *)arg);
        break;
    case MWCAP_IOCTL_SET_FIRMWARE_ERASE:
        ret = xi_driver_set_firmware_erase(driver, (MWCAP_FIRMWARE_ERASE *)arg);
        break;
    case MWCAP_IOCTL_SET_FIRMWARE_DATA:
        {
            MWCAP_FIRMWARE_DATA *info = (MWCAP_FIRMWARE_DATA *)arg;
            u8 *data;

            if (info->size <= 0) {
                ret = OS_RETURN_INVAL;
                break;
            }

            data = os_malloc(info->size);
            if (data == NULL) {
                ret = OS_RETURN_NOMEM;
                break;
            }

            if (os_copy_in(data, (os_user_addr_t)(unsigned long)info->data, info->size) != 0) {
                ret = OS_RETURN_ERROR;
                os_free(data);
                break;
            }

            ret = xi_driver_firmware_data_write(driver, info->offset, data, info->size);
            if (ret != info->size)
                ret = OS_RETURN_ERROR;
            else
                ret = OS_RETURN_SUCCESS;

            os_free(data);
        }
        break;
    case MWCAP_IOCTL_GET_FIRMWARE_DATA:
        {
            MWCAP_FIRMWARE_DATA *info = (MWCAP_FIRMWARE_DATA *)arg;
            u8 *data;

            if (info->size <= 0) {
                ret = OS_RETURN_INVAL;
                break;
            }

            data = os_malloc(info->size);
            if (data == NULL) {
                ret = OS_RETURN_NOMEM;
                break;
            }

            ret = xi_driver_firmware_data_read(driver, info->offset, data, info->size);
            if (ret > 0) {
                info->size = ret;
                if (os_copy_out((os_user_addr_t)(unsigned long)info->data, data, ret) != 0) {
                    ret = OS_RETURN_ERROR;
                    os_free(data);
                    break;
                }

                ret = OS_RETURN_SUCCESS;
            }

            os_free(data);
        }
        break;
    }

    return ret;
}

static long mwcap_private_ioctl(struct mw_stream *stream, os_iocmd_t cmd, void *arg)
{
    long ret = 0;
    void *driver = stream->driver;

    switch (cmd) {
        // HDCP
    case MWCAP_IOCTL_GET_HDCP_SUPPORT:
        *(bool *)arg = xi_driver_get_hdcp_support(driver);
        break;
    case MWCAP_IOCTL_SET_HDCP_SUPPORT:
        xi_driver_set_hdcp_support(driver, *(bool *)arg);
        break;

    case MWCAP_IOCTL_GET_TOP_FIELD_FIRST:
        *(bool *)arg = parameter_GetTopFieldFirst(
                    xi_driver_get_parameter_manager(driver));
        break;
    case MWCAP_IOCTL_SET_TOP_FIELD_FIRST:
        parameter_SetTopFieldFirst(
                    xi_driver_get_parameter_manager(driver),
                    *(bool *)arg);
        break;
    case MWCAP_IOCTL_GET_ENABLE_HD_SYNC_METER:
        *(bool *)arg = parameter_GetEnableHPSyncMeter(
                    xi_driver_get_parameter_manager(driver));
        break;
    case MWCAP_IOCTL_SET_ENABLE_HD_SYNC_METER:
        parameter_SetEnableHPSyncMeter(
                    xi_driver_get_parameter_manager(driver),
                    *(bool *)arg);
        break;
    case MWCAP_IOCTL_GET_FRAME_DURATION_CHANGE_THRESHOLD:
        *(DWORD *)arg = parameter_GetFrameDurationChangeThreshold(
                    xi_driver_get_parameter_manager(driver));
        break;
    case MWCAP_IOCTL_SET_FRAME_DURATION_CHANGE_THRESHOLD:
        parameter_SetFrameDurationChangeThreshold(
                    xi_driver_get_parameter_manager(driver),
                    *(DWORD *)arg);
        break;
    case MWCAP_IOCTL_GET_VIDEO_SYNC_SLICE_LEVEL:
        *(DWORD *)arg = parameter_GetVideoSyncSliceLevel(
                    xi_driver_get_parameter_manager(driver));
        break;
    case MWCAP_IOCTL_SET_VIDEO_SYNC_SLICE_LEVEL:
        parameter_SetVideoSyncSliceLevel(
                    xi_driver_get_parameter_manager(driver),
                    *(DWORD *)arg);
        break;
    case MWCAP_IOCTL_GET_INPUT_SOUCE_SCAN_PERIOD:
        *(DWORD *)arg = parameter_GetScanPeriod(
                           xi_driver_get_parameter_manager(driver));
        break;
    case MWCAP_IOCTL_SET_INPUT_SOUCE_SCAN_PERIOD:
        parameter_SetScanPeriod(
                    xi_driver_get_parameter_manager(driver),
                    *(DWORD *)arg);
        break;
    case MWCAP_IOCTL_GET_INPUT_SOUCE_SCAN_KEEP_DURATION:
        *(DWORD *)arg = parameter_GetScanKeepDuration(
                           xi_driver_get_parameter_manager(driver));
        break;
    case MWCAP_IOCTL_SET_INPUT_SOUCE_SCAN_KEEP_DURATION:
        parameter_SetScanKeepDuration(
                    xi_driver_get_parameter_manager(driver),
                    *(DWORD *)arg);
        break;
    case MWCAP_IOCTL_GET_SCAN_INPUT_SOURCES_COUNT:
        *(DWORD *)arg = parameter_GetScanInputSourceCount(
                    xi_driver_get_parameter_manager(driver));
        break;
    case MWCAP_IOCTL_GET_SCAN_INPUT_SOURCES_ARRAY:
        {
            MWCAP_INPUT_SOURCE_ARRAY *array = (MWCAP_INPUT_SOURCE_ARRAY *)arg;
            const u32 *data;
            int count;

            count = parameter_GetScanInputSourceCount(
                        xi_driver_get_parameter_manager(driver));
            data = parameter_GetScanInputSources(
                        xi_driver_get_parameter_manager(driver));

            if (array->count < count) {
                ret = OS_RETURN_INVAL;
                break;
            }

            if (os_copy_out((os_user_addr_t)(unsigned long)array->data, data, sizeof(u32) * count) != 0) {
                ret = OS_RETURN_ERROR;
                break;
            }
        }
        break;
    case MWCAP_IOCTL_SET_SCAN_INPUT_SOURCES_ARRAY:
        {
            MWCAP_INPUT_SOURCE_ARRAY *array = (MWCAP_INPUT_SOURCE_ARRAY *)arg;
            u32 *data;

            if (array->count <= 0) {
                ret = OS_RETURN_INVAL;
                break;
            }

            data = os_malloc(sizeof(u32) * array->count);
            if (data == NULL) {
                ret = OS_RETURN_NOMEM;
                break;
            }

            if (os_copy_in(data, (os_user_addr_t)(unsigned long)array->data, sizeof(u32) * array->count) != 0) {
                ret = OS_RETURN_ERROR;
                os_free(data);
                break;
            }

            if (!parameter_SetScanInputSources(
                        xi_driver_get_parameter_manager(driver),
                        data,
                        array->count)) {
                ret = OS_RETURN_NOMEM;
            }

            os_free(data);
        }
        break;
    case MWCAP_IOCTL_GET_VAD_THRESHOLD:
        *(DWORD *)arg = parameter_GetVADThreshold(
                    xi_driver_get_parameter_manager(driver));
        break;
    case MWCAP_IOCTL_SET_VAD_THRESHOLD:
        parameter_SetVADThreshold(
                    xi_driver_get_parameter_manager(driver),
                    *(DWORD *)arg);
        break;
    case MWCAP_IOCTL_GET_LOW_LATENCY_STRIPE_HEIGHT:
        *(int *)arg = parameter_GetLowLatencyStripeHeight(xi_driver_get_parameter_manager(driver));
        break;
    case MWCAP_IOCTL_SET_LOW_LATENCY_STRIPE_HEIGHT:
        parameter_SetLowLatencyStripeHeight(xi_driver_get_parameter_manager(driver),
                                            *(int *)arg);
        break;

        // Diagnoise
    case MWCAP_IOCTL_GET_I2C_REGISTER_ID:
        {
            MWCAP_DEVICE_I2C_S *par = (MWCAP_DEVICE_I2C_S *)arg;
            void *data;
            short ret_size;

            if (par->iSize <= 0) {
                ret = OS_RETURN_INVAL;
                break;
            }

            data = os_malloc(par->iSize);
            if (data == NULL) {
                ret = OS_RETURN_NOMEM;
                break;
            }

            ret_size = xi_driver_i2c_read_regs(
                        driver, par->iChannel, par->byDevAddr,
                        par->byRegAddr, data, par->iSize);

            if (ret_size <= 0 ||
                    os_copy_out(par->pData, data, par->iSize) != 0)
                ret = OS_RETURN_ERROR;

            os_free(data);
        }
        break;
    case MWCAP_IOCTL_SET_I2C_REGISTER_ID:
        {
            MWCAP_DEVICE_I2C_S *par = (MWCAP_DEVICE_I2C_S *)arg;
            void *data;

            if (par->iSize <= 0) {
                ret = OS_RETURN_INVAL;
                break;
            }

            data = os_malloc(par->iSize);
            if (data == NULL) {
                ret = OS_RETURN_NOMEM;
                break;
            }

            if (os_copy_in(data, par->pData, par->iSize) != 0) {
                ret = OS_RETURN_ERROR;
                os_free(data);
                break;
            }

            xi_driver_i2c_write_regs(
                        driver, par->iChannel, par->byDevAddr,
                        par->byRegAddr, data, par->iSize);

            os_free(data);
        }
        break;
    case MWCAP_IOCTL_GET_GSPI_REGISTER_ID:
        {
            MWCAP_DEVICE_GSPI_S *par = (MWCAP_DEVICE_GSPI_S *)arg;

            par->wData = xi_driver_gspi_read_reg(driver, par);
        }
        break;
    case MWCAP_IOCTL_SET_GSPI_REGISTER_ID:
        xi_driver_gspi_write_reg(driver, (MWCAP_DEVICE_GSPI_S *)arg);
        break;
    default:
        break;
    }

    return ret;
}


int mw_stream_init(struct mw_stream *stream,
                   void *driver,
                   struct mw_device *mw_dev,
                   os_pid_t pid,
                   char *proc_name)
{
    int ret = 0;

    stream->mw_dev = mw_dev;
    stream->driver = driver;

    stream->settings_lock = os_spin_lock_alloc();
    stream->png_mutex = os_mutex_alloc();
    stream->osd_image.mutex = os_mutex_alloc();
    stream->format_lock = os_spin_lock_alloc();
    if (stream->settings_lock == NULL
            || stream->png_mutex == NULL
            || stream->osd_image.mutex == NULL
            || stream->format_lock == NULL) {
        ret = OS_RETURN_NOMEM;
        goto lock_err;
    }

    /* load default */
    os_spin_lock(stream->mw_dev->settings_lock);
    os_memcpy(&stream->settings.process_settings,
           &stream->mw_dev->settings.process_settings,
           sizeof(stream->settings.process_settings));
    os_memcpy(&stream->png_settings, &stream->mw_dev->png_settings,
           sizeof(stream->png_settings));
    os_spin_unlock(stream->mw_dev->settings_lock);

    stream->settings.contrast = stream->mw_dev->settings.contrast;
    stream->settings.brightness = stream->mw_dev->settings.brightness;
    stream->settings.saturation = stream->mw_dev->settings.saturation;
    stream->settings.hue = stream->mw_dev->settings.hue;
    stream->settings.process_settings.deinterlaceMode =
            MWCAP_VIDEO_DEINTERLACE_BLEND;

    /* update osd */
    if (stream->png_settings.bEnable) {
        ret = _load_png_image(stream);

        if (ret == 0)
            ret = _update_osd_image(stream);
    }


    ret = mw_capture_init(&stream->s_mwcap, driver, mw_dev->m_iChannel, mw_dev->dma_priv);
    if (ret < 0) {
        goto cap_init_err;
    }

    /* alloc stream id */
    os_spin_lock(stream->mw_dev->list_lock);
    stream->mw_dev->max_stream_id++;
    stream->stream_id = stream->mw_dev->max_stream_id;
    list_add_tail(&stream->list_node, &stream->mw_dev->stream_list);
    stream->mw_dev->stream_count++;
    os_spin_unlock(stream->mw_dev->list_lock);

    stream->pid = pid;
    os_strlcpy(stream->proc_name, proc_name, MW_MAX_PROCESS_NAME_LEN);
    stream->ctrl_id = stream->stream_id;
    stream->ctrl_stream = NULL;

    return 0;

cap_init_err:
lock_err:
    if (stream->settings_lock != NULL) {
        os_spin_lock_free(stream->settings_lock);
        stream->settings_lock = NULL;
    }
    if (stream->png_mutex != NULL) {
        os_mutex_free(stream->png_mutex);
        stream->png_mutex = NULL;
    }
    if (stream->osd_image.mutex != NULL) {
        os_mutex_free(stream->osd_image.mutex);
        stream->osd_image.mutex = NULL;
    }
    if (stream->format_lock != NULL) {
        os_spin_lock_free(stream->format_lock);
        stream->format_lock = NULL;
    }
    return ret;
}

void mw_stream_deinit(struct mw_stream *stream)
{
    struct mw_device *mw_dev = stream->mw_dev;

    if (stream->settings_lock == NULL
            || stream->png_mutex == NULL
            || stream->osd_image.mutex == NULL
            || stream->format_lock == NULL)
        return;

    mw_capture_deinit(&stream->s_mwcap);

    os_mutex_lock(stream->osd_image.mutex);
    if (stream->osd_image.user_image.pvOSDImage)
        shared_image_buf_close(
                    xi_driver_get_imgbuf_manager(stream->driver),
                    (struct shared_image_buf *)(unsigned long)stream->osd_image.user_image.pvOSDImage,
                    NULL);
    os_mutex_unlock(stream->osd_image.mutex);

    os_mutex_lock(stream->png_mutex);
    _free_osd_image(stream);
    _free_png_image(stream);
    os_mutex_unlock(stream->png_mutex);

    /* free stream id */
    {
        struct mw_stream *_stream = NULL;
        struct list_head *pos = NULL;

        os_spin_lock(mw_dev->list_lock);

        list_del(&stream->list_node);
        mw_dev->stream_count--;
        if (mw_dev->stream_count < 0)
            mw_dev->stream_count = 0;

        /* find last stream, the stream id is max */
        if (!list_empty(&mw_dev->stream_list))
            _stream = list_entry(mw_dev->stream_list.prev, struct mw_stream, list_node);
        mw_dev->max_stream_id = 0;
        if (_stream != NULL)
            mw_dev->max_stream_id = _stream->stream_id;

        /* check if someone ctrl this pipe */
        if (!list_empty(&mw_dev->stream_list)) {
            list_for_each(pos, &mw_dev->stream_list) {
                _stream = list_entry(pos, struct mw_stream, list_node);
                if (_stream->ctrl_stream == stream) {
                    _stream->ctrl_stream = NULL;
                }
            }
        }

        os_spin_unlock(mw_dev->list_lock);
    }

    os_spin_lock_free(stream->settings_lock);
    stream->settings_lock = NULL;
    os_mutex_free(stream->png_mutex);
    stream->png_mutex = NULL;
    os_mutex_free(stream->osd_image.mutex);
    stream->osd_image.mutex = NULL;
    os_spin_lock_free(stream->format_lock);
    stream->format_lock = NULL;
}

long mw_stream_ioctl(struct mw_stream *stream, os_iocmd_t cmd, void *arg)
{
    long ret = 0;

    if (_IOC_TYPE(cmd) == _IOC_TYPE(MWCAP_IOCTL_GET_HDCP_SUPPORT)) {
        ret = mwcap_private_ioctl(stream, cmd, arg);
        return ret;
    }

    /* pipe ctrl */
    if (_IOC_NR(cmd) <= GET_SELF_STREAM_ID_NUM) {
        os_spin_lock(stream->mw_dev->list_lock);
        ret = mwcap_ioctl_stream(stream, cmd, arg);
        os_spin_unlock(stream->mw_dev->list_lock);
        return ret;
    }

    /* mw capture */
    if (mw_capture_ioctl(&stream->s_mwcap, cmd, arg, &ret)) {
        return ret;
    }

    /* setting for os local stream */
    if (_IOC_NR(cmd) >= GET_VIDEO_CONNECTION_FORMAT_NUM &&
        _IOC_NR(cmd) <= SET_SATURATION_NUM) {
        struct mw_stream *_stream = stream;

        if (stream->ctrl_stream == NULL && stream->ctrl_id != stream->stream_id) {
            return OS_RETURN_ERROR;
        }
        if (stream->ctrl_stream != NULL)
            _stream = stream->ctrl_stream;

        if (_IOC_NR(cmd) >= GET_BRIGHTNESS_NUM) {
            os_spin_lock(_stream->settings_lock);
            ret = mwcap_ioctl_settings(&_stream->settings, cmd, arg);
            os_spin_unlock(_stream->settings_lock);
        } else {
            ret = mwcap_ioctl_osd_process(_stream, cmd, arg);
        }
        return ret;
    }

    /* no need locked */
    if (mwcap_ioctl_nolock(stream, cmd, arg, &ret)) {
        return ret;
    }

    /* device locked */
    os_mutex_lock(stream->mw_dev->dev_mutex);
    ret = mwcap_ioctl_devlock(stream, cmd, arg);
    os_mutex_unlock(stream->mw_dev->dev_mutex);

    return ret;
}


bool mw_stream_acquire_osd_image(struct mw_stream *stream, struct xi_image_buffer **pimage,
                           RECT *rects, int *rect_count)
{
    struct shared_image_buf *shared_imgbuf;

    if (pimage == NULL || rects == NULL || rect_count == NULL)
        return false;

    *pimage = NULL;
    *rect_count = 0;

    os_mutex_lock(stream->osd_image.mutex);
    shared_imgbuf = (struct shared_image_buf *)(unsigned long)stream->osd_image.user_image.pvOSDImage;
    if (shared_imgbuf != NULL
            && shared_image_buf_is_valid(
                xi_driver_get_imgbuf_manager(stream->driver),
                shared_imgbuf)
            && xi_image_buffer_isvalid(&shared_imgbuf->image_buf)
            && stream->osd_image.user_image.cOSDRects != 0) {
        *pimage = &shared_imgbuf->image_buf;
        *rect_count = stream->osd_image.user_image.cOSDRects;
        os_memcpy(rects, stream->osd_image.user_image.aOSDRects, *rect_count * sizeof(RECT));
        return true;
    }

    if (!_osd_image_is_valid(&stream->osd_image)) {
        os_mutex_unlock(stream->osd_image.mutex);
        return false;
    }

    *pimage = &stream->osd_image.imgbuf;
    *rect_count = stream->osd_image.rect_count;
    os_memcpy(rects, stream->osd_image.rects, *rect_count * sizeof(RECT));
    return true;
}

void mw_stream_release_osd_image(struct mw_stream *stream)
{
    os_mutex_unlock(stream->osd_image.mutex);
}

void mw_stream_update_osd_image(struct mw_stream *stream)
{
    os_mutex_lock(stream->png_mutex);
    if (_osd_image_is_valid(&stream->osd_image)) {
        _update_osd_image(stream);
    }
    os_mutex_unlock(stream->png_mutex);
}

void mw_stream_update_connection_format(struct mw_stream *stream,
                                      bool update_osd,
                                      bool connected,
                                      int width,
                                      int height,
                                      u32 fourcc,
                                      u32 frame_duration)
{
    int ngcd;

    os_spin_lock(stream->format_lock);
    stream->format.bConnected = connected;
    stream->format.cx = width;
    stream->format.cy = height;
    stream->format.dwFOURCC = fourcc;
    stream->format.dwFrameDuration = frame_duration;

    ngcd = GCD((int)stream->format.cx, (int)stream->format.cy);
    stream->format.nAspectX = (int)stream->format.cx / ngcd;
    stream->format.nAspectY = (int)stream->format.cy / ngcd;

    if (FOURCC_IsRGB(stream->format.dwFOURCC)) {
        stream->format.colorFormat = MWCAP_VIDEO_COLOR_FORMAT_RGB;
        stream->format.quantRange = MWCAP_VIDEO_QUANTIZATION_FULL;
        stream->format.satRange = MWCAP_VIDEO_SATURATION_FULL;
    } else {
        stream->format.colorFormat = stream->format.cx < 720 ?
                    MWCAP_VIDEO_COLOR_FORMAT_YUV601 : MWCAP_VIDEO_COLOR_FORMAT_YUV709;
        stream->format.quantRange = MWCAP_VIDEO_QUANTIZATION_LIMITED;
        stream->format.satRange = MWCAP_VIDEO_SATURATION_LIMITED;
    }
    os_spin_unlock(stream->format_lock);

    if (update_osd) {
        os_mutex_lock(stream->png_mutex);
        if (_osd_image_is_valid(&stream->osd_image)) {
            struct OSD_IMAGE_SETTINGS image_settings;

            _get_image_buffer_settings(stream, &image_settings);
            if (_is_image_buffer_settings_changed(stream, &image_settings))
                _update_osd_image(stream);
        }
        os_mutex_unlock(stream->png_mutex);
    }
}
