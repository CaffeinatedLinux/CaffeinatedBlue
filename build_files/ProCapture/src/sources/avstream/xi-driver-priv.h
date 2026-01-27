////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#ifndef __XI_DRIVER_PRIV_H__
#define __XI_DRIVER_PRIV_H__

#include "mw-procapture-extension.h"
#include "mw-fourcc.h"
#include "supports/xi-timer.h"
#include "supports/xi-notify-event.h"
#include "supports/sw-timer-event-manager.h"
#include "supports/notify-event-manager.h"
#include "supports/image-buffer.h"

#include "front-end/front-end.h"

#include "xi-driver.h"
#include "mw-uio-drv.h"

// Private notify event masks
#define NOTIFY_EVENT_VIDEO_CAPTURE_SETTINGS_CHANGE			0x80000000ULL
#define NOTIFY_EVENT_AUDIO_CAPTURE_SETTINGS_CHANGE			0x40000000ULL

#define NOTIFY_EVENT_VIDEO_CAPTURE_LOST_SYNC				0x20000000ULL
#define NOTIFY_EVENT_VIDEO_CAPTURE_FRAME_CHANGE(i)			(0x00100000ULL << (i))

#define NOTIFY_EVENT_CLOSED_CAPTION_DATA					0x10000000ULL

enum GPIO_ID {
    GPIO_ID_SDI_RX_LOCKED		= 0,
    GPIO_ID_SDI_STANDBY			= 1,
    GPIO_ID_SDI_PROC_EN			= 2,
    GPIO_ID_SDI_AUDIO_EN		= 3,
    GPIO_ID_SDI_ASI				= 4,
    GPIO_ID_SDI_SMPTE_STANDARD	= 5,
    GPIO_ID_SDI_861_EN			= 6,
    GPIO_ID_SDI_RESET_N			= 7,
    GPIO_ID_SDI_LOOPBACK_EN		= 8,
    GPIO_ID_SDI_LOOPBACK_SD		= 9,
    GPIO_ID_SDI_SDO_EN			= 10,
    GPIO_ID_SDI_RECLK_EN		= 11,

    GPIO_ID_ADV_RESET_N			= 16,
    GPIO_ID_ADV_INT_N			= 17,
    GPIO_ID_DVI_HPD				= 18,

    GPIO_ID_HDMI_TX_HPD			= 19,
    GPIO_ID_HDMI_TX_RESET_N		= 20,
    GPIO_ID_HDMI_TX_EQ			= 21,

    GPIO_ID_HDMI_RX_HPD			= 22,
    GPIO_ID_HDMI_RX_RESET_N		= 23,
    GPIO_ID_HDMI_RX_EQ			= 24,
    GPIO_ID_HDMI_RX_DET			= 25,

    GPIO_ID_ADV_LINK2_RESET_N	= 26,
    GPIO_ID_ADV_LINK2_INT_N		= 27,

    GPIO_ID_HDMI2_RX_RESET_N	= 26,
    GPIO_ID_HDMI2_RX_EQ			= 27,
    GPIO_ID_HDMI2_RX_DET		= 28,
    GPIO_ID_HDMI_PORT_SEL		= 29,

    GPIO_ID_HDMI_TX_CLK_EN		= 30,

    GPIO_ID_HDMI_HDCP_EN		= 31
};

enum GPIO_MASK {
    GPIO_MASK_SDI_RX_LOCKED		= (1 << GPIO_ID_SDI_RX_LOCKED),
    GPIO_MASK_SDI_STANDBY		= (1 << GPIO_ID_SDI_STANDBY),
    GPIO_MASK_SDI_PROC_EN		= (1 << GPIO_ID_SDI_PROC_EN),
    GPIO_MASK_SDI_AUDIO_EN		= (1 << GPIO_ID_SDI_AUDIO_EN),
    GPIO_MASK_SDI_ASI			= (1 << GPIO_ID_SDI_ASI),
    GPIO_MASK_SDI_SMPTE_STANDARD= (1 << GPIO_ID_SDI_SMPTE_STANDARD),
    GPIO_MASK_SDI_861_EN		= (1 << GPIO_ID_SDI_861_EN),
    GPIO_MASK_SDI_RESET_N		= (1 << GPIO_ID_SDI_RESET_N),
    GPIO_MASK_SDI_LOOPBACK_EN	= (1 << GPIO_ID_SDI_LOOPBACK_EN),
    GPIO_MASK_SDI_LOOPBACK_SD	= (1 << GPIO_ID_SDI_LOOPBACK_SD),
    GPIO_MASK_SDI_SDO_EN		= (1 << GPIO_ID_SDI_SDO_EN),
    GPIO_MASK_SDI_RECLK_EN		= (1 << GPIO_ID_SDI_RECLK_EN),

    GPIO_MASK_ADV_RESET_N		= (1 << GPIO_ID_ADV_RESET_N),
    GPIO_MASK_ADV_INT_N			= (1 << GPIO_ID_ADV_INT_N),
    GPIO_MASK_DVI_HPD			= (1 << GPIO_ID_DVI_HPD),

    GPIO_MASK_HDMI_TX_HPD		= (1 << GPIO_ID_HDMI_TX_HPD),
    GPIO_MASK_HDMI_TX_RESET_N	= (1 << GPIO_ID_HDMI_TX_RESET_N),
    GPIO_MASK_HDMI_TX_EQ		= (1 << GPIO_ID_HDMI_TX_EQ),

    GPIO_MASK_HDMI_RX_HPD		= (1 << GPIO_ID_HDMI_RX_HPD),
    GPIO_MASK_HDMI_RX_RESET_N	= (1 << GPIO_ID_HDMI_RX_RESET_N),
    GPIO_MASK_HDMI_RX_EQ		= (1 << GPIO_ID_HDMI_RX_EQ),
    GPIO_MASK_HDMI_RX_DET		= (1 << GPIO_ID_HDMI_RX_DET),

    GPIO_MASK_ADV_LINK2_RESET_N = (1 << GPIO_ID_ADV_LINK2_RESET_N),
    GPIO_MASK_ADV_LINK2_INT_N	= (1 << GPIO_ID_ADV_LINK2_INT_N),

    GPIO_MASK_HDMI2_RX_RESET_N	= (1 << GPIO_ID_HDMI2_RX_RESET_N),
    GPIO_MASK_HDMI2_RX_EQ		= (1 << GPIO_ID_HDMI2_RX_EQ),
    GPIO_MASK_HDMI2_RX_DET		= (1 << GPIO_ID_HDMI2_RX_DET),
    GPIO_MASK_HDMI_PORT_SEL		= (1 << GPIO_ID_HDMI_PORT_SEL),

    GPIO_MASK_HDMI_TX_CLK_EN	= (1 << GPIO_ID_HDMI_TX_CLK_EN),
    GPIO_MASK_HDMI_HDCP_EN		= (1 << GPIO_ID_HDMI_HDCP_EN)
};

enum AUDIO_CTRL {
    AUDIO_CTRL_HDMI             = 0x00,
    AUDIO_CTRL_HDMI_ADV7611     = 0x01,
    AUDIO_CTRL_SDI              = 0x01,
    AUDIO_CTRL_ANALOG           = 0x02
};

enum VIDEO_CTRL {
    VIDEO_CTRL_ENABLE			= 0x01,
    VIDEO_CTRL_ADV_RECEIVER		= 0x02,
    VIDEO_CTRL_ADV_48BIT		= 0x04,
    VIDEO_CTRL_ADV_48BIT_422	= 0x08
};

struct xi_gpio *xi_driver_get_gpio_master(struct xi_driver *pobj);

struct xi_i2c_master *xi_driver_get_i2c_master(struct xi_driver *pobj);

xi_notify_event_manager *xi_driver_get_notify_event_manager(struct xi_driver *pobj, int iChannel);

xi_sw_timer_event_manager *xi_driver_get_sw_timer_event_manager(struct xi_driver *pobj);

struct xi_spi_master *xi_driver_get_gspi_master(struct xi_driver *pobj);

struct shared_image_buf_manager *xi_driver_get_imgbuf_manager(struct xi_driver *pobj);

struct xi_heap *xi_driver_get_onboard_heap(struct xi_driver *pobj);

volatile void __iomem *xi_driver_get_sync_meter_addr(struct xi_driver *pobj);

volatile void __iomem *xi_driver_get_video_activity_detector_addr(struct xi_driver *pobj);

volatile void __iomem *xi_driver_get_sdi_address(struct xi_driver *pobj);

volatile void __iomem *xi_driver_get_hdmi_phy_gpio_addr(struct xi_driver *pobj);

volatile void __iomem *xi_driver_get_hdmi_phy_clkdet_addr(struct xi_driver *pobj);

volatile void __iomem *xi_driver_get_hdmi_phy_drpm_addr(struct xi_driver *pobj);

volatile void __iomem *xi_driver_get_hdmi_rx_base_addr(struct xi_driver *pobj);

volatile void __iomem *xi_driver_get_sdi_rx_base_addr(struct xi_driver *pobj);

volatile void __iomem *xi_driver_get_sdianc_base_addr(struct xi_driver *pobj);


/* Magewell Capture Extensions */
int xi_driver_get_firmware_storage_info(struct xi_driver *pobj,
        MWCAP_FIRMWARE_STORAGE *pStorage);
int xi_driver_set_firmware_erase(struct xi_driver *pobj, MWCAP_FIRMWARE_ERASE *pErase);
int xi_driver_firmware_data_write(struct xi_driver *pobj, u32 offset, u8 *data, u32 count);
int xi_driver_firmware_data_read(struct xi_driver *pobj, u32 offset, u8 *data, u32 count);

void xi_driver_set_led_mode(struct xi_driver *pobj, unsigned int mode);
void xi_driver_set_post_reconfig_delay(struct xi_driver *pobj, u32 delay);
long xi_driver_get_core_temperature(struct xi_driver *pobj);

int xi_driver_set_edid(struct xi_driver *pobj, u8 *data, int size);
const u8 *xi_driver_get_edid(struct xi_driver *pobj, int *size);
u32 xi_driver_get_hdmi_infoframe_valid_flags(struct xi_driver *pobj);
int xi_driver_get_hdmi_infoframe_packet(struct xi_driver *pobj,
        MWCAP_HDMI_INFOFRAME_ID id, HDMI_INFOFRAME_PACKET *pInfoFramePacket);

void xi_driver_set_video_input_aspect_ratio(struct xi_driver *pobj,
        MWCAP_VIDEO_ASPECT_RATIO *pAspectRatio);
bool xi_driver_get_video_input_aspect_ratio(struct xi_driver *pobj,
        MWCAP_VIDEO_ASPECT_RATIO *pAspectRatio);
void xi_driver_set_video_input_color_format(struct xi_driver *pobj,
        MWCAP_VIDEO_COLOR_FORMAT colorFormat);
MWCAP_VIDEO_COLOR_FORMAT
xi_driver_get_video_input_color_format(struct xi_driver *pobj);
void xi_driver_set_video_input_quantization_range(struct xi_driver *pobj,
        MWCAP_VIDEO_QUANTIZATION_RANGE range);
MWCAP_VIDEO_QUANTIZATION_RANGE
xi_driver_get_video_input_quantization_range(struct xi_driver *pobj);

// VGA/Component timings
bool xi_driver_get_video_auto_h_align(struct xi_driver *pobj);
int xi_driver_set_video_auto_h_align(struct xi_driver *pobj, bool enable);
u8 xi_driver_get_video_sampling_phase(struct xi_driver *pobj);
int xi_driver_set_video_sampling_phase(struct xi_driver *pobj, u8 sampling_phase);
bool xi_driver_get_video_sampling_phase_auto(struct xi_driver *pobj);
int xi_driver_set_video_sampling_phase_auto(struct xi_driver *pobj, bool enable);
int xi_driver_set_video_timing(struct xi_driver *pobj, const MWCAP_VIDEO_TIMING *timing);
int xi_driver_set_video_custom_timing(struct xi_driver *pobj, const MWCAP_VIDEO_CUSTOM_TIMING *timing);
/* @ret; <0 error; >=0 timing count */
int xi_driver_get_video_preferred_timing_array(struct xi_driver *pobj, MWCAP_VIDEO_TIMING *timing);
int xi_driver_get_video_custom_timing_count(struct xi_driver *pobj);
const MWCAP_VIDEO_CUSTOM_TIMING * xi_driver_get_video_custom_timing_array(
        struct xi_driver *pobj, int *count);
int xi_driver_set_video_custom_timing_array(struct xi_driver *pobj,
            MWCAP_VIDEO_CUSTOM_TIMING *timings, int count);
int xi_driver_get_video_custom_resolution_count(struct xi_driver *pobj);
const MWCAP_SIZE * xi_driver_get_video_custom_resolution_array(
        struct xi_driver *pobj, int *count);
int xi_driver_set_video_custom_resolution_array(struct xi_driver *pobj,
            MWCAP_SIZE *resolutions, int count);

/* private */
int xi_driver_set_hdcp_support(struct xi_driver *pobj, bool enable);
bool xi_driver_get_hdcp_support(struct xi_driver *pobj);

short xi_driver_i2c_read_regs(
        struct xi_driver *pobj,
        int ch, uint8_t devaddr, uint8_t regaddr, uint8_t *data, short size);
short xi_driver_i2c_write_regs(
        struct xi_driver *pobj,
        int ch, uint8_t devaddr, uint8_t regaddr, uint8_t *data, short size);
WORD xi_driver_gspi_read_reg(struct xi_driver *pobj,
                              MWCAP_DEVICE_GSPI_S *gpi_s);
void xi_driver_gspi_write_reg(struct xi_driver *pobj,
                              MWCAP_DEVICE_GSPI_S *gpi_s);

/* SDI ANC */
void xi_driver_set_sdianc_type(struct xi_driver *pobj,
                               MWCAP_SDI_ANC_TYPE *pANCType);
void xi_driver_get_sdianc_packet(struct xi_driver *pobj,
                                 MWCAP_SDI_ANC_PACKET *pPacket);

BOOLEAN xi_driver_GetEnableSystemSDIANC(struct xi_driver *pobj);
void xi_driver_SetEnableSystemSDIANC(struct xi_driver *pobj, BOOLEAN bEnable);

// QuadSDIForceConvMode2SI is use for fixing QuadLink signal in china TV.
int xi_driver_SetQuadSDIForceConvMode2SI(struct xi_driver *pobj, BOOLEAN bEnable);

#endif /* __XI_DRIVER_PRIV_H__ */
