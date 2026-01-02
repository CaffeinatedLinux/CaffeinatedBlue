////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#include "ospi/ospi.h"
#include "win-types.h"
#include "xi-version.h"
#include "mw-edid.h"

#include "xi-driver-priv.h"
#include "drivers/gpio-master.h"
#include "drivers/i2c-master.h"
#include "drivers/device-info.h"
#include "drivers/irq-control.h"
#include "drivers/pcie-dma-controller.h"
#include "drivers/pcie-dma-desc-chain.h"
#include "drivers/pcie-ring-dma-controller.h"
#include "drivers/pcie-memory-writer.h"
#include "drivers/spi-master.h"
#include "drivers/qspiflash-micron.h"
#include "drivers/audio-capture.h"
#include "drivers/video-capture.h"
#include "drivers/timestamp.h"
#include "drivers/pi7c9x2gxxx.h"
#include "drivers/pwm-fan-control.h"
#include "drivers/sdianc.h"
#include "supports/math.h"
#include "supports/timer-event-manager.h"
#include "supports/notify-event-manager.h"
#include "supports/resource-pool.h"
#include "supports/heap.h"
#include "supports/image-buffer.h"
#include "supports/utils.h"
#include "supports/shared-image-buffer.h"
#include "supports/karray.h"
#include "picopng/picopng.h"
#include "dma/mw-dma-mem-priv.h"

#include "front-end/front-end.h"
#include "mw-netlink.h"

enum DEV_ADDR {
    DEV_INFO_BASE_ADDR			= 0x000 * 4,
    IRQ_BASE_ADDR				= 0x010 * 4,
    OW_BASE_ADDR				= 0x020 * 4,
    I2C_BASE_ADDR				= 0x030 * 4,
    GSPI_BASE_ADDR				= 0x040 * 4,
    GPIO_BASE_ADDR				= 0x050 * 4,
    DNA_BASE_ADDR				= 0x060 * 4,
    SPI_BASE_ADDR				= 0x070 * 4,
    SYNC_METER_BASE_ADDR		= 0x080 * 4,
    AUD_CAPTURE_BASE_ADDR		= 0x090 * 4,
    AUD_DMA_BASE_ADDR			= 0x0A0 * 4,
    VAD_BASE_ADDR				= 0x0B0 * 4,
    TIMESTAMP_BASE_ADDR			= 0x0C0 * 4,
    VPP1_DMA_BASE_ADDR			= 0x0D0 * 4,
    VPP2_DMA_BASE_ADDR			= 0x0E0 * 4,
    UPLOAD_DMA_BASE_ADDR		= 0x0F0 * 4,
    VPP1_MWR_DMA_BASE_ADDR		= 0x100 * 4,
    VPP2_MWR_DMA_BASE_ADDR		= 0x110 * 4,
    SDI_INPUT_BASE_ADDR			= 0x120 * 4,
    PWM_FAN_BASE_ADDR			= 0x130 * 4,
    ANC1_BASE_ADDR				= 0x140 * 4,
    ANC2_BASE_ADDR				= 0x150 * 4,
    VID_CAPTURE_BASE_ADDR		= 0x800 * 4,
    HDMI_PHY_BASE_ADDR			= 0x200 * 4,
    HDMI_PHY_DRPM_ADDR			= HDMI_PHY_BASE_ADDR + 0x00 * 4,
    HDMI_PHY_GPIO_ADDR			= HDMI_PHY_BASE_ADDR + 0x10 * 4,
    HDMI_PHY_NIDRU_ADDR			= HDMI_PHY_BASE_ADDR + 0x20 * 4,
    HDMI_PHY_CLKDET_ADDR		= HDMI_PHY_BASE_ADDR + 0x30 * 4,
    HDMI_RX_BASE_ADDR			= 0x300 * 4,
    SDI_RX_BASE_ADDR			= 0x400 * 4,
    DP_PHY_BASE_ADDR			= 0x200 * 4,
    DP_RX_BASE_ADDR				= 0x300 * 4,
};

enum IRQ_MASK {
    IRQ_MASK_I2C				= 0x00000001,
    IRQ_MASK_GPIO				= 0x00000002,
    IRQ_MASK_UART				= 0x00000004,
    IRQ_MASK_SYNC_METER			= 0x00000008,
    IRQ_MASK_AUD_CAPTURE		= 0x00000010,
    IRQ_MASK_AUD_DMA			= 0x00000020,
    IRQ_MASK_VID_CAPTURE		= 0x00000040,
    IRQ_MASK_VPP1_DMA			= 0x00000080,
    IRQ_MASK_VPP2_DMA			= 0x00000100,
    IRQ_MASK_TIMESTAMP			= 0x00000200,
    IRQ_MASK_UPLOAD_DMA			= 0x00000400,
    IRQ_MASK_VPP1_MWR_DMA		= 0x00000800,
    IRQ_MASK_VPP2_MWR_DMA		= 0x00001000,
    IRQ_MASK_SDI_INPUT			= 0x00002000,
    IRQ_MASK_VAD                = 0x00004000,
    IRQ_MASK_HDMI_PHY_CLKDET	= 0x00008000,
    IRQ_MASK_HDMI_PHY_GPIO		= 0x00010000,
    IRQ_MASK_HDMI_RX			= 0x00020000,
    IRQ_MASK_ANC1				= 0x00040000,
    IRQ_MASK_ANC2				= 0x00080000
};

enum SYS_FREQ {
    SYS_FREQ_SPI				= 20000000,
    SYS_FREQ_GSPI				= 6250000,
    SYS_FREQ_I2C				= 100000,
    SYS_FREQ_HDMI				= 100000000
};

enum MEM_INFO {
    MAX_FIRMWARE_HEADER_SIZE    = 65536,

    FRAME_BUFFER_BASE			= 0,
    FRAME_BUFFER_SIZE			= 256 * 1024 * 1024
};

enum FRONT_END_TYPE {
    FRONT_END_AIO,
    FRONT_END_DVI,
    FRONT_END_HDMI,
    FRONT_END_DUAL_DVI,
    FRONT_END_GV7601,
    FRONT_END_ADV761X,
    FRONT_END_ADV7842,
    FRONT_END_XILINX_3G_SDI,
    FRONT_END_XILINX_HDMI,
    FRONT_END_AIO_4K_PLUS,
    FRONT_END_DUAL_ADV761X,
    FRONT_END_AIO_4K,
    FRONT_END_QUAD_LINK_SDI,
    FRONT_END_AIO_LITE,
    FRONT_END_DP,
    FRONT_END_ADV7180
};

enum THREAD_REQUEST {
    REQUEST_EXIT_THREAD,
    REQUEST_SET_INPUT_SORUCE_SCAN,
    REQUEST_GET_INPUT_SOURCE_SCAN_STATE,
    REQUEST_SET_AV_INPUT_SOURCE_LINK,
    REQUEST_SET_VIDEO_INPUT_SOURCE,
    REQUEST_GET_VIDEO_INPUT_SOURCE,
    REQUEST_SET_AUDIO_INPUT_SOURCE,
    REQUEST_GET_AUDIO_INPUT_SOURCE,
    REQUEST_SET_EDID,
    REQUEST_SET_ENABLE_HDCP,
    REQUEST_SET_AUTO_H_ALIGN,
    REQUEST_SET_SAMPLING_PHASE,
    REQUEST_GET_PREFERRED_TIMING_ARRAY,
    REQUEST_SET_VIDEO_CUSTOM_TIMING_ARRAY,
    REQUEST_SET_VIDEO_CUSTOM_RESOLUTION_ARRAY
};

enum FRAME_SOURCE {
    SOURCE_FULL_FRAME,
    SOURCE_QUARTER_FRAME,
    SOURCE_TOP_FIELD,
    SOURCE_BOTTOM_FIELD
};

struct xi_driver_private {
    struct xi_irq_control              irq_ctrl;
    struct pcie_ring_dma_controller    aud_dma_ctrl;
    struct pcie_dma_controller         vpp_dma_ctrl[2];
    struct pcie_memory_writer          vpp_memory_writer[2];
    struct pcie_dma_controller         upload_dma_ctrl;

    struct xi_timestamp                timestamp;
    struct xi_device                   dev_info;
    struct xi_gpio                     gpio;
    struct xi_spi_master               spi;
    struct qspiflash_micron            spi_flash;
    struct xi_spi_master               gspi;
    struct xi_i2c_master               i2c;
    struct audio_capture               audio_cap;
    struct video_capture               video_cap[MAX_CHANNELS_PER_DEVICE];
    struct mw_pi7c9x2gxxx              pi7c9x2gxxx;
    struct pwm_fan_ctrl                 m_pwmFanCtrl;
    struct sdianc                       m_sdiANC;

    xi_timer_event_manager             timer_event_manager;
    xi_sw_timer_event_manager          sw_timer_manager;
    os_event_t                         sw_timer_event;
    xi_notify_event_manager            notify_event_manager[MAX_CHANNELS_PER_DEVICE];
    os_event_t                         m_eventSignal;
    xi_notify_event                   *signal_notify_event[MAX_CHANNELS_PER_DEVICE];

    struct parameter_t                 parameter_manager;
    struct xi_heap                     board_heap;
    struct xi_image_buffer             imgbuf_nosignal;
    struct xi_image_buffer             imgbuf_unsupported;
    struct xi_image_buffer             imgbuf_locking;
    struct shared_image_buf_manager    imgbuf_manager;

    /* device info */
    u32                                hardware_ver;
    u16                                product_id;
    u8                                 board_index;
    u8                                 channel_index;
    u8                                 pci_bus_id;
    u8                                 pci_dev_id;
    char                               product_name[MW_PRODUCT_NAME_LEN];
    u32                                firmware_ver;
    struct uio_device	               *uio_dev;
    CHAR                               szBoardSerialNo[MW_SERIAL_NO_LEN];

    /*front end*/
    int                                front_end_type;
    struct xi_front_end                *front_end[MAX_CHANNELS_PER_DEVICE];

    bool                               has_i2c;
    bool                               has_gspi;
    BOOLEAN                            m_bHasPI7C9X2GX08;
    BOOLEAN                            m_bHasPWMFan;
    BOOLEAN                            m_bHasLoopthrough;
    DWORD                              m_dwGPIOIntrMasks;
    bool                               multi_ch;
    u32                                ref_clk_freq;
    PARAMETER_EDID_TYPE                 m_bEDIDType;
    int                                 m_nPixelsPerCycle;
    DWORD                               m_cbFirmwareStorageLimit;
    BOOLEAN                             m_bMultiChannelFPGA;
    int                                 m_cChannlesPerDevice;
    BOOLEAN                             m_bSupportLowLatency;
    BOOLEAN                             m_b16ChannelAudio;
    BOOLEAN                             m_b16To2AudioMux;

    u32                                post_reconfig_delay;

    /* house keeping */
    os_thread_t                        house_kthread;
    struct xi_thread_request_ack       house_thread_request;

    xi_notify_event                   *m_apNotifyEventFrame[2 * MAX_CHANNELS_PER_DEVICE];

    /* video */
    struct pcie_dma_desc_chain         vpp_dma_chain[2];
    struct resource_pool               vpp_pool;
    os_event_t                         vpp1_dma_done;
    os_event_t                         vpp2_dma_done;
    os_event_t                         vpp_dma_done[2];
    os_event_t                         upload_fbwr_done;
    os_event_t                         upload_dma_done;
    u32                                video_cap_enabled_int[MAX_CHANNELS_PER_DEVICE];
    os_clock_kick_t                    video_strip_last_enable_time[MAX_CHANNELS_PER_DEVICE];
    os_spinlock_t                      video_strip_enable_lock;

    // Device caps
    DWORD                               m_dwVideoCaptureCaps;
    DWORD                               m_dwVideoCaptureStatus[MAX_CHANNELS_PER_DEVICE];
    LONGLONG                            m_llCurrTime;
    os_atomic_t                         m_dwVPP1CPLDTag;
    os_atomic_t                         m_dwVPP2CPLDTag;

    LPFN_PARTIAL_NOTIFY                m_alpfnPartialNotify[2];
    unsigned long                      m_apvPartialNotifyContext[2];
    os_event_t                         m_eventVideoDMAFrameComplete[2];
    os_event_t                         m_eventVPPFBRDDone[2];

    /* audio */
    os_contig_dma_desc_t               audio_dma_buf;   /* audio dma buffer */
    os_event_t                         audio_cap_done;
    os_atomic_t                        audio_completed_block_id;

    /* audio ref count */
    os_mutex_t                          audio_ref_lock;
    int                                 audio_ref_count;

    /* events */
    os_event_t                          gpio_irq_done;
    os_event_t                          sync_meter_done;
    os_event_t                          sdi_input_done;
    os_event_t                          vad_done;
    os_event_t                          m_eventHDMIGPIO;
    os_event_t                          m_eventHDMIClkDet;
    os_event_t                          m_eventHDMIRX;
    os_event_t                          m_eventSDIANC;

    /* irq */
    os_atomic_t                        bits_val;
    os_atomic_t                        intr_enabled;

    /* soft timer sim irq */
    os_timer                            m_simIsrTimer;
    bool                                m_bSimISRMode;

    /* suspend save */
    long long                          suspend_ts;

    volatile void __iomem             *reg_base;
    os_dma_par_t                       dma_priv;  /* os contigous priv */
    void                              *driver;     /* xi_driver */


    // Limitations
    int                                 m_cxMaxInput;
    int                                 m_cyMaxInput;
    int                                 m_cxMaxOutput;
    int                                 m_cyMaxOutput;

    BOOLEAN                             m_bEnableSystemSDIANC;
};

static void _sim_irq_process(void *param);

static void _video_capture_interrupt_handler(struct xi_driver_private *priv);
static int house_keeping_thread_proc(void *data);
static bool _process_request(struct xi_driver_private *priv);

static void _ResetVideoCapture(struct xi_driver_private *priv, int iChannel);
static void _StopVideoCapture(struct xi_driver_private *priv, int iChannel);
static void _StartAudioCapture(struct xi_driver_private *priv);
static void _StopAudioCapture(struct xi_driver_private *priv);
static void _ResetAudioCapture(struct xi_driver_private *priv);
static void _GetBlackColorValue(
        unsigned short *r_v,
        unsigned short *g_y,
        unsigned short *b_u,
        MWCAP_VIDEO_COLOR_FORMAT colorFormat,
        MWCAP_VIDEO_QUANTIZATION_RANGE quantRange,
        SHORT sRVBUScale,
        SHORT sRVBUOffset
        );
static int _WaitForStripeDone(struct xi_driver *pobj, int iChannel, int iVPP, enum FRAME_SOURCE frameSource,
                              int iFrame, int ySrc, int cySrc, DWORD dwTimeout);
static void _EnableStripeInterrupts(struct xi_driver_private *priv, int iChannel);
static void _UpdateStripeInterrupts(struct xi_driver_private *priv, int iChannel);

static void _UpdateEDID(struct xi_driver_private *priv);
static void _AdjustFanDuty(struct xi_driver_private *priv);

struct xi_gpio *xi_driver_get_gpio_master(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return &priv->gpio;
}

struct xi_i2c_master *xi_driver_get_i2c_master(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return &priv->i2c;
}

xi_sw_timer_event_manager *xi_driver_get_sw_timer_event_manager(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return &priv->sw_timer_manager;
}

xi_notify_event_manager *xi_driver_get_notify_event_manager(struct xi_driver *pobj, int iChannel)
{
    struct xi_driver_private *priv = pobj->priv;

    return &priv->notify_event_manager[iChannel];
}

struct parameter_t *xi_driver_get_parameter_manager(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return &priv->parameter_manager;
}

struct xi_spi_master *xi_driver_get_gspi_master(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return &priv->gspi;
}

struct shared_image_buf_manager *xi_driver_get_imgbuf_manager(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return &priv->imgbuf_manager;
}

struct xi_heap *xi_driver_get_onboard_heap(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return &priv->board_heap;
}

volatile void __iomem *xi_driver_get_sync_meter_addr(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return priv->reg_base + SYNC_METER_BASE_ADDR;
}

volatile void __iomem *xi_driver_get_video_activity_detector_addr(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return priv->reg_base + VAD_BASE_ADDR;
}

volatile void __iomem *xi_driver_get_sdi_address(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return priv->reg_base + SDI_INPUT_BASE_ADDR;
}

volatile void __iomem *xi_driver_get_hdmi_phy_gpio_addr(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return priv->reg_base + HDMI_PHY_GPIO_ADDR;
}

volatile void __iomem *xi_driver_get_hdmi_phy_clkdet_addr(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return priv->reg_base + HDMI_PHY_CLKDET_ADDR;
}

volatile void __iomem *xi_driver_get_hdmi_phy_drpm_addr(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return priv->reg_base + HDMI_PHY_DRPM_ADDR;
}

volatile void __iomem *xi_driver_get_hdmi_rx_base_addr(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return priv->reg_base + HDMI_RX_BASE_ADDR;
}

volatile void __iomem *xi_driver_get_sdi_rx_base_addr(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return priv->reg_base + SDI_RX_BASE_ADDR;
}

volatile void __iomem *xi_driver_get_sdianc_base_addr(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return priv->reg_base + ANC1_BASE_ADDR;
}

u32 xi_driver_get_ref_clock_freq(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return priv->ref_clk_freq;
}

int xi_driver_get_family_name(struct xi_driver *pobj, char *name, size_t size)
{
    struct xi_driver_private *priv = pobj->priv;
    int ret = 0;

    if (NULL == priv || NULL == name || 0 == size)
        return -1;

    switch (priv->product_id) {
    case MWCAP_PRODUCT_ID_PRO_CAPTURE_EZ100A:
        os_strlcpy(name, "EZ Capture", size);
        break;

    default:
        os_strlcpy(name, "Pro Capture", size);
        break;
    }

    return ret;
}

int xi_driver_get_card_name(struct xi_driver *pobj, int iChannel, void *name, size_t size)
{
    struct xi_driver_private *priv = pobj->priv;
    int ch_index;
    int ret;

    if (NULL == priv || NULL == name || 0 == size)
        return -1;

    ch_index = priv->m_bMultiChannelFPGA ? iChannel : priv->channel_index;
    if (priv->multi_ch || priv->m_bMultiChannelFPGA) {
        ret = snprintf(name, size, "%02d-%02d %s", priv->board_index, ch_index, priv->product_name);
    } else {
        ret = snprintf(name, size, "%02d %s", priv->board_index, priv->product_name);
    }

    return ret;
}

int xi_driver_get_channle_count(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return priv->m_cChannlesPerDevice;
}


/************ init functions   *************/
static int _get_device_caps(struct xi_driver_private *priv)
{
    volatile void __iomem *reg_base = priv->reg_base;
    u32 channel_id = 0;

    xi_device_init(&priv->dev_info, reg_base+DEV_INFO_BASE_ADDR);
    priv->product_id = xi_device_get_product_id(&priv->dev_info);
    priv->ref_clk_freq = xi_device_get_ref_clk_freq(&priv->dev_info);
    if (priv->ref_clk_freq == 0)
        priv->ref_clk_freq = 125000000;

    priv->hardware_ver = xi_device_get_hardware_ver(&priv->dev_info);
    priv->firmware_ver = xi_device_get_firmware_ver(&priv->dev_info);

    channel_id = xi_device_get_channel_id(&priv->dev_info);
    priv->board_index = (channel_id >> 4) & 0x0F;
    priv->channel_index = channel_id & 0x0F;

    priv->pci_bus_id = (xi_device_get_pcie_device_addr(&priv->dev_info) >> 8)
            & 0xFF;
    priv->pci_dev_id = xi_device_get_pcie_device_addr(&priv->dev_info) & 0x1F;


    xi_debug(1, "ProductID: %04X, BoardIndex: %02X, Channel: %02X\n",
            priv->product_id, priv->board_index, priv->channel_index);

    priv->front_end_type = FRONT_END_AIO;
    priv->has_i2c = false;
    priv->has_gspi = false;
    priv->m_bHasPWMFan = false;
    priv->multi_ch = false;
    priv->m_bHasPI7C9X2GX08 = false;
    priv->m_dwGPIOIntrMasks = 0;
    priv->m_bEDIDType = PARAMETER_EDID_2K;
    priv->m_cbFirmwareStorageLimit = 4 * 1024 * 1024;
    priv->m_bMultiChannelFPGA = FALSE;
    priv->m_cChannlesPerDevice = 1;
    priv->m_b16ChannelAudio = FALSE;
    priv->m_b16To2AudioMux = FALSE;
    priv->m_bHasLoopthrough = FALSE;
    priv->m_bSupportLowLatency = TRUE;

    priv->m_nPixelsPerCycle = 1;
    priv->m_cxMaxInput = priv->m_cxMaxOutput = 2048;
    priv->m_cyMaxInput = priv->m_cyMaxOutput = 2160;

    os_strlcpy(priv->product_name, "Pro Capture ", MW_PRODUCT_NAME_LEN);

    if (xi_device_get_vfs_full_frame_size(&priv->dev_info) >= 0x4000000) {
        priv->m_cyMaxInput = 4096;
        priv->m_cyMaxOutput = 4096;
    }

    switch (priv->product_id) {
        case MWCAP_PRODUCT_ID_PRO_CAPTURE_AIO:
        {
            os_strlcat(priv->product_name, "AIO", MW_PRODUCT_NAME_LEN);

            priv->front_end_type = FRONT_END_AIO;
            priv->has_i2c = true;

            if (priv->hardware_ver <= 0x00020000) {
                priv->has_gspi = true;
                priv->m_dwGPIOIntrMasks = GPIO_MASK_SDI_RX_LOCKED | GPIO_MASK_ADV_INT_N;
            } else {
                priv->m_dwGPIOIntrMasks = GPIO_MASK_ADV_INT_N;
            }

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_EZ100A:
        {
            os_strlcpy(priv->product_name, "EZ Capture EZ100A", MW_PRODUCT_NAME_LEN);

            priv->front_end_type = FRONT_END_AIO;
            priv->has_i2c = TRUE;

            priv->m_dwGPIOIntrMasks = GPIO_MASK_ADV_INT_N;

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_DVI:
        {
            os_strlcat(priv->product_name, "DVI", MW_PRODUCT_NAME_LEN);

            priv->front_end_type = FRONT_END_DVI;
            priv->has_i2c = true;
            priv->m_dwGPIOIntrMasks = GPIO_MASK_ADV_INT_N;

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_HDMI:
        {
            os_strlcat(priv->product_name, "HDMI", MW_PRODUCT_NAME_LEN);

            priv->front_end_type = FRONT_END_HDMI;
            priv->has_i2c = true;
            priv->m_dwGPIOIntrMasks = GPIO_MASK_ADV_INT_N;

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_SDI:
        {
            os_strlcat(priv->product_name, "SDI", MW_PRODUCT_NAME_LEN);

            priv->m_cyMaxInput = 1080;

            if (priv->hardware_ver <= 0x00010000) {
                priv->has_gspi = true;
                priv->m_dwGPIOIntrMasks = GPIO_MASK_SDI_RX_LOCKED;
                priv->front_end_type = FRONT_END_GV7601;
            } else {
                priv->front_end_type = FRONT_END_XILINX_3G_SDI;
            }

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_DUAL_SDI:
        {
            os_strlcat(priv->product_name, "Dual SDI", MW_PRODUCT_NAME_LEN);

            priv->multi_ch = true;
            priv->m_bHasPI7C9X2GX08 = true;
            priv->m_cyMaxInput = 1080;

            if (priv->hardware_ver <= 0x00010000) {
                priv->front_end_type = FRONT_END_GV7601;
                priv->has_gspi = true;
                priv->m_dwGPIOIntrMasks = GPIO_MASK_SDI_RX_LOCKED;
            } else {
                priv->front_end_type = FRONT_END_XILINX_3G_SDI;
            }

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_DUAL_DVI:
        {
            os_strlcat(priv->product_name, "Dual DVI", MW_PRODUCT_NAME_LEN);

            priv->front_end_type = FRONT_END_DUAL_DVI;
            priv->has_i2c = true;
            priv->multi_ch = true;
            priv->m_bHasPI7C9X2GX08 = true;
            priv->m_dwGPIOIntrMasks = GPIO_MASK_ADV_INT_N;

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_DUAL_HDMI:
        {
            os_strlcat(priv->product_name, "Dual HDMI", MW_PRODUCT_NAME_LEN);

            priv->front_end_type = FRONT_END_ADV761X;
            priv->has_i2c = true;
            priv->multi_ch = true;
            priv->m_bHasPI7C9X2GX08 = true;
            priv->m_dwGPIOIntrMasks = GPIO_MASK_ADV_INT_N;

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_QUAD_SDI:
        {
            os_strlcat(priv->product_name, "Quad SDI", MW_PRODUCT_NAME_LEN);

            priv->m_bHasPWMFan = (priv->hardware_ver >= 0x00030000);
            priv->multi_ch = true;
            priv->m_bHasPI7C9X2GX08 = true;
            priv->m_cyMaxInput = 1080;

            if (priv->hardware_ver <= 0x00020000) {
                priv->front_end_type = FRONT_END_GV7601;
                priv->has_gspi = true;
                priv->m_dwGPIOIntrMasks = GPIO_MASK_SDI_RX_LOCKED;
            } else {
                priv->front_end_type = FRONT_END_XILINX_3G_SDI;
            }

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_QUAD_HDMI:
        {

            os_strlcat(priv->product_name, "Quad HDMI", MW_PRODUCT_NAME_LEN);

            priv->m_bHasPWMFan = (priv->hardware_ver >= 0x00020000);
            priv->front_end_type = FRONT_END_ADV761X;
            priv->has_i2c = true;
            priv->multi_ch = true;
            priv->m_bHasPI7C9X2GX08 = true;
            priv->m_dwGPIOIntrMasks = GPIO_MASK_ADV_INT_N;

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_MINI_HDMI:
        {
            os_strlcat(priv->product_name, "Mini HDMI", MW_PRODUCT_NAME_LEN);

            priv->front_end_type = FRONT_END_ADV761X;
            priv->has_i2c = true;
            priv->m_dwGPIOIntrMasks = GPIO_MASK_ADV_INT_N;

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_HDMI_4K:
        {
            os_strlcat(priv->product_name, "HDMI 4K", MW_PRODUCT_NAME_LEN);

            priv->front_end_type = FRONT_END_ADV761X;
            priv->has_i2c = true;
            priv->m_dwGPIOIntrMasks = GPIO_MASK_ADV_INT_N;

            priv->m_bEDIDType = PARAMETER_EDID_4K;
            priv->m_cxMaxInput = priv->m_cxMaxOutput = 4096;
            priv->m_nPixelsPerCycle = 2;

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_MINI_SDI:
        {
            os_strlcat(priv->product_name, "Mini SDI", MW_PRODUCT_NAME_LEN);

            if (priv->firmware_ver <= 0x00010022)
                priv->front_end_type = FRONT_END_XILINX_3G_SDI;
            else
                priv->front_end_type = FRONT_END_QUAD_LINK_SDI;
            priv->has_i2c = false;
            priv->m_cyMaxInput = 1080;

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_AIO_4K_PLUS:
        {
            os_strlcat(priv->product_name, "AIO 4K+", MW_PRODUCT_NAME_LEN);

            priv->front_end_type = FRONT_END_AIO_4K_PLUS;
            priv->has_i2c = true;

            priv->m_bEDIDType = PARAMETER_EDID_4Kp;
            priv->m_cxMaxInput = priv->m_cxMaxOutput = 4096;
            priv->m_nPixelsPerCycle = 4;
            priv->m_cbFirmwareStorageLimit = 8 * 1024 * 1024;

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_HDMI_4K_PLUS:
        case MWCAP_PRODUCT_ID_PRO_CAPTURE_HDMI_4K_PLUS_TBT:
        {
            os_strlcat(priv->product_name, "HDMI 4K+", MW_PRODUCT_NAME_LEN);

            priv->m_bHasPWMFan = (priv->hardware_ver == 0x00010000);
            priv->front_end_type = FRONT_END_XILINX_HDMI;
            priv->has_i2c = true;
            priv->m_bEDIDType = PARAMETER_EDID_4Kp;

            priv->m_dwGPIOIntrMasks = (priv->hardware_ver == 0x00010000) ? GPIO_MASK_HDMI_TX_HPD : 0;
            priv->m_bHasLoopthrough = (priv->hardware_ver == 0x00010000);

            priv->m_cxMaxInput = priv->m_cxMaxOutput = 4096;
            priv->m_nPixelsPerCycle = 4;
            priv->m_cbFirmwareStorageLimit = 8 * 1024 * 1024;

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_SDI_4K_PLUS:
        {
            os_strlcat(priv->product_name, "SDI 4K+", MW_PRODUCT_NAME_LEN);

            priv->front_end_type = FRONT_END_QUAD_LINK_SDI;
            priv->has_gspi = TRUE;

            priv->m_bHasPWMFan = (priv->hardware_ver >= 0x00040000);
            priv->m_cxMaxInput = priv->m_cxMaxOutput = 4096;
            priv->m_nPixelsPerCycle = 4;
            priv->m_cbFirmwareStorageLimit = 16 * 1024 * 1024;

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_DUAL_HDMI_4K_PLUS:
        {
            os_strlcat(priv->product_name, "Dual HDMI 4K+", MW_PRODUCT_NAME_LEN);

            priv->m_bHasPWMFan = TRUE;
            priv->front_end_type = FRONT_END_XILINX_HDMI;
            priv->has_i2c = true;
            priv->m_bEDIDType = PARAMETER_EDID_4Kp;
            priv->multi_ch = true;

            priv->m_dwGPIOIntrMasks = GPIO_MASK_HDMI_TX_HPD;
            priv->m_bHasLoopthrough = TRUE;

            priv->m_cxMaxInput = priv->m_cxMaxOutput = 4096;
            priv->m_nPixelsPerCycle = 4;
            priv->m_cbFirmwareStorageLimit = 8 * 1024 * 1024;

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_DUAL_SDI_4K_PLUS:
        {
            os_strlcat(priv->product_name, "Dual SDI 4K+", MW_PRODUCT_NAME_LEN);

            priv->front_end_type = FRONT_END_QUAD_LINK_SDI;
            priv->has_gspi = TRUE;
            priv->multi_ch = true;

            priv->m_bHasPWMFan = (priv->hardware_ver >= 0x00040000);
            priv->m_cxMaxInput = priv->m_cxMaxOutput = 4096;
            priv->m_nPixelsPerCycle = 4;
            priv->m_cbFirmwareStorageLimit = 16 * 1024 * 1024;

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_DVI_4K:
        {
            os_strlcat(priv->product_name, "DVI 4K", MW_PRODUCT_NAME_LEN);

            priv->front_end_type = FRONT_END_DUAL_ADV761X;
            priv->has_i2c = true;
            priv->m_bEDIDType = PARAMETER_EDID_DVI_4K;

            priv->m_dwGPIOIntrMasks = GPIO_MASK_ADV_INT_N;

            priv->m_cxMaxInput = priv->m_cxMaxOutput = 4096;
            priv->m_nPixelsPerCycle = 2;

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_AIO_4K:
        {
            os_strlcat(priv->product_name, "AIO 4K", MW_PRODUCT_NAME_LEN);

            priv->front_end_type = FRONT_END_AIO_4K;
            priv->has_i2c = true;
            priv->m_bEDIDType = PARAMETER_EDID_4K;

            priv->m_dwGPIOIntrMasks = GPIO_MASK_ADV_INT_N;

            priv->m_cxMaxInput = priv->m_cxMaxOutput = 4096;
            priv->m_nPixelsPerCycle = 2;

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_DUAL_AIO_BETA:
        case MWCAP_PRODUCT_ID_PRO_CAPTURE_DUAL_AIO:
        {
            os_strlcat(priv->product_name, "Dual AIO", MW_PRODUCT_NAME_LEN);

            priv->has_i2c = true;
            priv->multi_ch = true;

            priv->m_dwGPIOIntrMasks = GPIO_MASK_ADV_INT_N;
            priv->front_end_type = FRONT_END_AIO_LITE;

            return 0;
        }

        case MWCAP_PRODUCT_ID_PRO_CAPTURE_HEXA_CVBS:
        {
            os_strlcat(priv->product_name, "Hexa CVBS", MW_PRODUCT_NAME_LEN);

            priv->front_end_type = FRONT_END_ADV7180;
            priv->has_i2c = TRUE;
            priv->m_bSupportLowLatency = FALSE;

            priv->m_cxMaxInput = 720;
            priv->m_cyMaxInput = 576;

            priv->m_bMultiChannelFPGA = TRUE;
            priv->m_cChannlesPerDevice = 6;
            priv->m_b16ChannelAudio = TRUE;
            priv->m_b16To2AudioMux = TRUE;

            return 0;
        }

        default:
            return -1;
    }

    return -1;
}

static int _init_parameters(struct xi_driver_private *priv)
{
    return parameter_manager_init(&priv->parameter_manager, priv->driver, priv->m_bEDIDType);
}

static void _deinit_parameters(struct xi_driver_private *priv)
{
    parameter_manager_deinit(&priv->parameter_manager);
}

static struct xi_front_end *_mw_front_end_create(void *driver,
    struct xi_front_end_notify_sink *notify_sink, int cxMax, int cyMax)
{
    struct xi_front_end *front;
    int ret;

    front = os_zalloc(sizeof(*front));
    if (front == NULL)
        return NULL;

    ret = xi_front_end_init(front, driver, NULL, cxMax, cyMax);
    if (ret != 0)
        os_free(front);

    return front;
}

static int mw_front_end_create(struct xi_driver_private *priv)
{
    int i, ret;

    switch (priv->front_end_type) {
    case FRONT_END_AIO:
    case FRONT_END_DVI:
    case FRONT_END_HDMI:
    case FRONT_END_DUAL_DVI:
    case FRONT_END_GV7601:
    case FRONT_END_ADV761X:
    case FRONT_END_ADV7842:
    case FRONT_END_XILINX_3G_SDI:
    case FRONT_END_XILINX_HDMI:
    case FRONT_END_AIO_4K_PLUS:
    case FRONT_END_DUAL_ADV761X:
    case FRONT_END_AIO_4K:
    case FRONT_END_QUAD_LINK_SDI:
    case FRONT_END_AIO_LITE:
    case FRONT_END_ADV7180:
        for (i = 0; i < priv->m_cChannlesPerDevice; i++) {
            priv->front_end[i] = _mw_front_end_create(priv->driver, NULL, priv->m_cxMaxInput, priv->m_cyMaxInput);
            priv->front_end[i]->chn_index = i;
        }
        break;
    default:
        xi_debug(0, "Front End<%d> Not Supported!\n", priv->front_end_type);
        priv->front_end[0] = NULL;
        break;
    }

    for (i = 0; i < priv->m_cChannlesPerDevice; i++) {
        if (priv->front_end[i] == NULL) {
            xi_debug(0, "create front end[%d] failed\n", i);
            ret = OS_RETURN_NOMEM;
            return  ret;
        }
    }

    return 0;
}

static int _init_devices(struct xi_driver_private *priv)
{
    volatile void __iomem *reg_base = priv->reg_base;
    u8 mem_init_done = 0;
    u8 mem_error = 0;
    int ret = 0;
    int i;

    // FAN control
    if (priv->m_bHasPWMFan) {
        xi_debug(0, "Initialize PWM FAN\n");
        pwm_fan_ctrl_init(&priv->m_pwmFanCtrl, reg_base+PWM_FAN_BASE_ADDR, priv->ref_clk_freq, 25000);
        pwm_fan_ctrl_Enable(&priv->m_pwmFanCtrl, true);

        _AdjustFanDuty(priv);
    }

    xi_device_get_device_status(&priv->dev_info, NULL, &mem_init_done, &mem_error);
    if (mem_error || !mem_init_done) {
        xi_debug(0, "Memory failed, Done: %d, Error: %d\n", mem_init_done, mem_error);
        return OS_RETURN_ERROR;
    }

    // irq init
    xi_irq_init(&priv->irq_ctrl, reg_base+IRQ_BASE_ADDR);
    xi_irq_set_control(&priv->irq_ctrl, 1);
    xi_irq_set_enable_bits_value(&priv->irq_ctrl, 0); // enable irq when item ready
    os_atomic_set(&priv->intr_enabled, 1);

    /* enable sim isr */
    os_timer_schedule_relative(priv->m_simIsrTimer, 100);

    xi_timestamp_init(&priv->timestamp, reg_base+TIMESTAMP_BASE_ADDR);
    xi_timestamp_set_time(&priv->timestamp, 0);
    xi_timestamp_set_alarm_time(&priv->timestamp, TIMESTAMP_INFINITE);
    xi_timestamp_enable_alarm_int(&priv->timestamp, true);
    xi_irq_set_enable_bits(&priv->irq_ctrl, IRQ_MASK_TIMESTAMP);

    /* spi */
    xi_spi_init(&priv->spi, reg_base+SPI_BASE_ADDR,
            priv->ref_clk_freq, SYS_FREQ_SPI, SPI_BITS_WIDTH_8);
    // Fix Xilinx STARTUPE2 clock problem (Lost first 2 pulse after configuration)
    xi_spi_set_chip_select(&priv->spi, 0x00);
    xi_spi_write(&priv->spi, 0);

    qspiflash_micron_init(&priv->spi_flash, &priv->spi, 0x01);
    xi_debug(1, "SPI Signature: %04X\n",
            qspiflash_micron_ReadJedecID(&priv->spi_flash));

    /* gpio */
    xi_gpio_init(&priv->gpio, reg_base+GPIO_BASE_ADDR);
    xi_gpio_set_intr_trigger_mode(&priv->gpio,
                                  (GPIO_INTR_TRIG_MODE_CHANGE << GPIO_ID_HDMI_TX_HPD)
                                  | (GPIO_INTR_TRIG_MODE_CHANGE << GPIO_ID_SDI_RX_LOCKED)
                                  | (GPIO_INTR_TRIG_MODE_EVENT << GPIO_ID_ADV_INT_N));
    xi_gpio_set_intr_trigger_type(&priv->gpio, (GPIO_INTR_TRIG_TYPE_LEVEL << GPIO_ID_ADV_INT_N));
    xi_gpio_set_intr_trigger_value(&priv->gpio, (GPIO_INTR_TRIG_VALUE_LOW << GPIO_ID_ADV_INT_N));
    xi_gpio_set_intr_enable_bits_value(&priv->gpio, priv->m_dwGPIOIntrMasks);
    xi_irq_set_enable_bits(&priv->irq_ctrl, IRQ_MASK_GPIO);

    // Initialize A/V front-end devices
    /* i2c */
    if (priv->has_i2c) {
        xi_debug(2, "i2c init\n");
        ret = xi_i2c_master_init(&priv->i2c, reg_base+I2C_BASE_ADDR, priv->ref_clk_freq, SYS_FREQ_I2C);
        if (ret != OS_RETURN_SUCCESS)
            return ret;
        xi_debug(0, "i2c master:%p\n", &priv->i2c);
        xi_irq_set_enable_bits(&priv->irq_ctrl, IRQ_MASK_I2C);
    }
    if (priv->has_gspi) {
        xi_spi_init(&priv->gspi, reg_base+GSPI_BASE_ADDR,
                priv->ref_clk_freq, SYS_FREQ_GSPI, SPI_BITS_WIDTH_16);
    }

    ret = mw_front_end_create(priv);
    if (ret < 0)
        goto front_end_err;

    /* sdi anc */
    sdianc_init(&priv->m_sdiANC, reg_base+ANC2_BASE_ADDR);
    sdianc_SetControl(&priv->m_sdiANC, TRUE);

    xi_pcie_ring_dma_controller_init(&priv->aud_dma_ctrl, reg_base+AUD_DMA_BASE_ADDR);
    xi_pcie_dma_controller_init(&priv->vpp_dma_ctrl[0], reg_base+VPP1_DMA_BASE_ADDR);
    xi_pcie_dma_controller_init(&priv->vpp_dma_ctrl[1], reg_base+VPP2_DMA_BASE_ADDR);
    xi_pcie_dma_controller_init(&priv->upload_dma_ctrl, reg_base+UPLOAD_DMA_BASE_ADDR);
    xi_pcie_memory_writer_init(&priv->vpp_memory_writer[0], reg_base+VPP1_MWR_DMA_BASE_ADDR);
    xi_pcie_memory_writer_init(&priv->vpp_memory_writer[1], reg_base+VPP2_MWR_DMA_BASE_ADDR);
    xi_irq_set_enable_bits(&priv->irq_ctrl,
                           IRQ_MASK_SYNC_METER |
                           IRQ_MASK_SDI_INPUT |
                           IRQ_MASK_VAD |
                           IRQ_MASK_AUD_DMA |
                           IRQ_MASK_VPP1_DMA |
                           IRQ_MASK_VPP2_DMA |
                           IRQ_MASK_UPLOAD_DMA |
                           IRQ_MASK_VPP1_MWR_DMA |
                           IRQ_MASK_VPP2_MWR_DMA |
                           IRQ_MASK_HDMI_PHY_CLKDET |
                           IRQ_MASK_HDMI_PHY_GPIO |
                           IRQ_MASK_HDMI_RX |
                           IRQ_MASK_ANC1 |
                           IRQ_MASK_ANC2
                           );

    /* audio input */
    audio_capture_init(&priv->audio_cap, reg_base + AUD_CAPTURE_BASE_ADDR);
    audio_capture_set_int_enables(&priv->audio_cap,
            AUD_INT_MASK_INPUT_OVERFLOW | AUD_INT_MASK_INPUT_LOST_SYNC);
    xi_irq_set_enable_bits(&priv->irq_ctrl, IRQ_MASK_AUD_CAPTURE);

    /* video input */
    xi_debug(2, "video capture init\n");
    for (i = 0; i < priv->m_cChannlesPerDevice; i++) {
        ret = video_capture_Initialize(
                    &priv->video_cap[i],
                    reg_base+VID_CAPTURE_BASE_ADDR,
                    i,
                    xi_device_get_vfs_frame_count(&priv->dev_info),
                    xi_device_get_vfs_full_buffer_address(&priv->dev_info),
                    xi_device_get_vfs_full_frame_size(&priv->dev_info),
                    xi_device_get_vfs_quarter_buffer_address(&priv->dev_info),
                    xi_device_get_vfs_quarter_frame_size(&priv->dev_info),
                    priv->m_nPixelsPerCycle
                    );
        if (ret != OS_RETURN_SUCCESS)
            goto video_cap_err;

        video_capture_SetIntEnables(&priv->video_cap[i], priv->video_cap_enabled_int[i]);
    }
    priv->m_dwVideoCaptureCaps = video_capture_GetCaps(&priv->video_cap[0]);

    xi_irq_set_enable_bits(&priv->irq_ctrl, IRQ_MASK_VID_CAPTURE);

    xi_debug(1, "Product ID: %08X\n", priv->product_id);
    xi_debug(1, "mem_init_done=%d mem_error=%d\n", mem_init_done, mem_error);
    xi_debug(1, "FirmwareVer: %08X\n", xi_device_get_firmware_ver(&priv->dev_info));
    xi_debug(1, "LinkStatus: %08X\n", xi_device_get_pcie_link_status(&priv->dev_info));
    xi_debug(1, "DeviceAddr: %08X\n", xi_device_get_pcie_device_addr(&priv->dev_info));
    xi_debug(1, "PayloadSize: %08X\n", xi_device_get_pcie_payload_size(&priv->dev_info));
    xi_debug(1, "Device Temperature: %d\n", xi_device_get_temperature(&priv->dev_info));
    xi_debug(1, "Reference Clock: %d\n", priv->ref_clk_freq);

    xi_debug(0, "Init devices successful!\n");

    return 0;

video_cap_err:
    for (i = 0; i < priv->m_cChannlesPerDevice; i++) {
        if (priv->front_end[i] != NULL) {
            xi_front_end_Destroy(priv->front_end[i]);
            priv->front_end[i] = NULL;
        }
    }
front_end_err:
    if (priv->has_i2c)
        xi_i2c_master_deinit(&priv->i2c);

    xi_timestamp_enable_alarm_int(&priv->timestamp, false);

    xi_irq_set_control(&priv->irq_ctrl, false);
    os_atomic_set(&priv->intr_enabled, 0);

    return ret;
}

static void _deinit_devices(struct xi_driver_private *priv)
{
    int i;

    if (priv->m_simIsrTimer != NULL)
        os_timer_cancel(priv->m_simIsrTimer);

    video_capture_SetControlPortValue(&priv->video_cap[0], 0);

    xi_irq_set_control(&priv->irq_ctrl, false);
    os_atomic_set(&priv->intr_enabled, 0);

    for (i = 0; i < 2; i++) {
        xi_pcie_dma_controller_clear_cpldtag_valid(&priv->vpp_dma_ctrl[i]);
        xi_pcie_dma_controller_clear_interrupt(&priv->vpp_dma_ctrl[i]);
        xi_pcie_memory_writer_clear_interrupt(&priv->vpp_memory_writer[i]);
    }

    for (i = 0; i < priv->m_cChannlesPerDevice; i++) {
        if (priv->front_end[i] != NULL) {
            xi_front_end_Destroy(priv->front_end[i]);
            priv->front_end[i] = NULL;
        }
    }

    if (priv->has_i2c)
        xi_i2c_master_deinit(&priv->i2c);

    for (i = 0; i < priv->m_cChannlesPerDevice; i++) {
        video_capture_Deinitialize(&priv->video_cap[i]);
    }

    xi_timestamp_enable_alarm_int(&priv->timestamp, false);

    xi_gpio_clear_data_bits(&priv->gpio, GPIO_MASK_DVI_HPD);

    if (priv->post_reconfig_delay != 0xFFFFFFFF) {
        xi_device_reconfig(&priv->dev_info, priv->post_reconfig_delay);
        priv->post_reconfig_delay = 0xFFFFFFFF;
    }
}

static int _init_dma(struct xi_driver_private *priv)
{
    int ret;
    int i;

    /* alloc audio buffer */
    priv->audio_dma_buf = os_contig_dma_alloc(
            sizeof(AUDIO_CAPTURE_FRAME) * AUDIO_FRAME_COUNT, priv->dma_priv);
    if (priv->audio_dma_buf == NULL) {
        xi_debug(1, "os_contig_dma_alloc error: %m!\n");
        return OS_RETURN_NOMEM;
    }

    /* video chain init */
    for (i = 0; i < 2; i++) {
        ret = xi_pcie_dma_desc_chain_init(&priv->vpp_dma_chain[i], 16384, priv->dma_priv);
        if (ret != 0) {
            xi_debug(1, "xi_pcie_dma_desc_chain_init error: %m!\n");
            return OS_RETURN_NOMEM;
        }
    }

    /* vpp resource pool */
    i = (priv->m_dwVideoCaptureCaps & VPP_CAPS_VPP2) ? 2 : 1;
    ret = resource_pool_create(&priv->vpp_pool, i);
    if (ret != OS_RETURN_SUCCESS)
        return ret;

    return OS_RETURN_SUCCESS;
}

static void _deinit_dma(struct xi_driver_private *priv)
{
    int i;

    if (priv->audio_dma_buf != NULL)
        os_contig_dma_free(priv->audio_dma_buf);

    for (i = 0; i < 2; i++) {
        /* move to deinit_deivce ? TODO check
        xi_pcie_dma_controller_clear_cpldtag_valid(&priv->vpp_dma_ctrl[i]);
        xi_pcie_dma_controller_clear_interrupt(&priv->vpp_dma_ctrl[i]);

        xi_pcie_memory_writer_clear_interrupt(&priv->vpp_memory_writer[i]);
        */

        xi_pcie_dma_desc_chain_deinit(&priv->vpp_dma_chain[i]);
    }

    resource_pool_destroy(&priv->vpp_pool);
}

static void _deinit_house_keeping_thread(struct xi_driver_private *priv);
static int _init_house_keeping_thread(struct xi_driver_private *priv)
{
    int i;
    int ret;

    for (i = 0; i < priv->m_cChannlesPerDevice; i++) {
        priv->signal_notify_event[i] = xi_notify_event_manager_add(
                    &priv->notify_event_manager[i],
                    MWCAP_NOTIFY_VIDEO_INPUT_SOURCE_CHANGE
                    | MWCAP_NOTIFY_AUDIO_INPUT_SOURCE_CHANGE
                    | MWCAP_NOTIFY_VIDEO_SIGNAL_CHANGE
                    | NOTIFY_EVENT_VIDEO_CAPTURE_SETTINGS_CHANGE
                    | NOTIFY_EVENT_AUDIO_CAPTURE_SETTINGS_CHANGE
                    | NOTIFY_EVENT_VIDEO_CAPTURE_LOST_SYNC
                    | MWCAP_NOTIFY_HDMI_INFOFRAME_VS
                    | MWCAP_NOTIFY_LOOP_THROUGH_EDID_CHANGED
                    | MWCAP_NOTIFY_NEW_SDI_ANC_PACKET,
                    priv->m_eventSignal
                    );

        if (priv->signal_notify_event[i] == NULL) {
            _deinit_house_keeping_thread(priv);
            return OS_RETURN_ERROR;
        }
    }

    ret = xi_thread_request_init(&priv->house_thread_request);
    if (ret != OS_RETURN_SUCCESS) {
        _deinit_house_keeping_thread(priv);
        return ret;
    }

    for (i = 0; i < priv->m_cChannlesPerDevice * 2; i++) {
        int iChannel = i / 2;
        priv->m_apNotifyEventFrame[i] = xi_notify_event_manager_add(
                    &priv->notify_event_manager[iChannel],
                    0,
                    NULL);
        if (NULL == priv->m_apNotifyEventFrame[i]) {
            _deinit_house_keeping_thread(priv);
            return OS_RETURN_NOMEM;
        }
    }

    /* house keeping thread */
    priv->house_kthread = os_thread_create_and_run(house_keeping_thread_proc, priv, "XI-HOUSE");
    if (priv->house_kthread == NULL) {
        xi_debug(0, "Create kernel thread XI-HOUSE Error: %m\n");
        _deinit_house_keeping_thread(priv);
        return OS_RETURN_ERROR;
    }

    return 0;
}

static void _deinit_house_keeping_thread(struct xi_driver_private *priv)
{
    int i;

    if (priv->house_kthread) {
        xi_thread_request_request(&priv->house_thread_request,
                REQUEST_EXIT_THREAD, NULL, NULL, NULL);
        os_thread_wait_and_free(priv->house_kthread);
        priv->house_kthread = NULL;
    }

    for (i = 0; i < priv->m_cChannlesPerDevice; i++) {
        if (priv->signal_notify_event[i]) {
            xi_notify_event_manager_remove(&priv->notify_event_manager[i],
                    priv->signal_notify_event[i]);
            priv->signal_notify_event[i] = NULL;
        }
    }

    for (i = 0; i < priv->m_cChannlesPerDevice * 2; i++) {
        int iChannel = i / 2;
        if (priv->m_apNotifyEventFrame[i] != NULL) {
            xi_notify_event_manager_remove(
                        &priv->notify_event_manager[iChannel],
                        priv->m_apNotifyEventFrame[i]
                        );
            priv->m_apNotifyEventFrame[i] = NULL;
        }
    }

    xi_thread_request_deinit(&priv->house_thread_request);
}

static bool _load_bitmap(struct xi_driver_private *priv,
                         struct xi_image_buffer *imgbuf,
                         png_pix_info_t *png_info)
{
    struct mw_dma_desc *dma;
    int ret;

    if (!xi_image_buffer_create(imgbuf, &priv->board_heap,
                                png_info->width, png_info->height,
                                png_info->width * 4)) {
        ret = -1;
        goto destroy_png;
    }

    ret = mw_dma_memory_create_desc(
                &dma,
                MWCAP_VIDEO_MEMORY_TYPE_KERNEL,
                (unsigned long)karray_get_data(&png_info->arr_image),
                png_info->width * 4 * png_info->height,
                MW_DMA_TO_DEVICE,
                priv->dma_priv
                );
    if (ret != 0) {
        goto free_imgbuf;
    }

    mw_dma_memory_sync_for_device(dma);
    if (xi_driver_sg_put_frame(priv->driver,
                xi_image_buffer_get_address(imgbuf),
                xi_image_buffer_get_stride(imgbuf),
                0,
                0,
                xi_image_buffer_get_width(imgbuf),
                xi_image_buffer_get_height(imgbuf),
                MWCAP_VIDEO_COLOR_FORMAT_RGB,
                MWCAP_VIDEO_QUANTIZATION_FULL,
                MWCAP_VIDEO_SATURATION_FULL,
                dma->mwsg_list,
                dma->sglen,
                FALSE,
                png_info->width * 4,
                png_info->width,
                png_info->height,
                FALSE,
                FALSE,
                1000
                ) != 0) {
        ret = false;
        goto free_dma;
    }

    mw_dma_memory_destroy_desc(dma);
    karray_free(&png_info->arr_image);
    return true;

free_dma:
    mw_dma_memory_destroy_desc(dma);
free_imgbuf:
    xi_image_buffer_destroy(imgbuf);
destroy_png:
    karray_free(&png_info->arr_image);
    return ret;
}

static void _deinit_image_buffers(struct xi_driver_private *priv)
{
    xi_image_buffer_destroy(&priv->imgbuf_nosignal);
    xi_image_buffer_destroy(&priv->imgbuf_unsupported);
    xi_image_buffer_destroy(&priv->imgbuf_locking);

    shared_image_buf_manager_deinit(&priv->imgbuf_manager);
}

static int _init_image_buffers(struct xi_driver_private *priv,
                               png_pix_info_t *nosignal_png,
                               png_pix_info_t *unsupported_png,
                               png_pix_info_t *locking_png)
{
    shared_image_buf_manager_init(&priv->imgbuf_manager, &priv->board_heap);

    if ((nosignal_png == NULL || nosignal_png->width <=0 || nosignal_png->height <= 0 ||  karray_is_empty(&nosignal_png->arr_image))
            || (unsupported_png == NULL || unsupported_png->width <=0 || unsupported_png->height <= 0 ||  karray_is_empty(&unsupported_png->arr_image))
            || (locking_png == NULL || locking_png->width <=0 || locking_png->height <= 0 ||  karray_is_empty(&locking_png->arr_image))
            )
        return 0;

    if (!_load_bitmap(priv, &priv->imgbuf_nosignal, nosignal_png)
            || !_load_bitmap(priv, &priv->imgbuf_unsupported, unsupported_png)
            || !_load_bitmap(priv, &priv->imgbuf_locking, locking_png)) {
        _deinit_image_buffers(priv);
        return -1;
    }

    return 0;
}

bool xi_driver_is_support_msi(volatile void __iomem *reg_base)
{
    struct xi_device                   dev_info;
    u16 product_id;
    u32 firmware_ver;

    xi_device_init(&dev_info, reg_base+DEV_INFO_BASE_ADDR);
    product_id = xi_device_get_product_id(&dev_info);
    firmware_ver = xi_device_get_firmware_ver(&dev_info);

    return !((firmware_ver == 0x00010012 || firmware_ver == 0x00010011)
             && product_id == MWCAP_PRODUCT_ID_PRO_CAPTURE_DUAL_HDMI);
}

int xi_driver_init(struct xi_driver *pobj, volatile void __iomem *reg_base, os_dma_par_t dma_priv,
                   png_pix_info_t *nosignal_png,
                   png_pix_info_t *unsupported_png,
                   png_pix_info_t *locking_png,
                   os_pci_dev_t pci_dev,
                   os_pci_dev_t par_dev,
                   struct uio_device *uio_dev
                   )
{
    int ret = 0;
    int i;

    struct xi_driver_private *priv;

    if (NULL == pobj || NULL != pobj->priv) {
        xi_debug(1, "xi_driver is NULL or is already inited!\n");
        return OS_RETURN_INVAL;
    }

    priv = os_zalloc(sizeof(struct xi_driver_private));
    if (NULL == priv) {
        xi_debug(1, "alloc for xi driver priv error: %m\n");
        return OS_RETURN_NOMEM;
    }

    mw_pi7c9x2gxxx_init(&priv->pi7c9x2gxxx, pci_dev, par_dev);
    mw_pi7c9x2gxxx_acs_bugfix(&priv->pi7c9x2gxxx);

    /* init_devices need irq for i2c */
    pobj->priv = priv;

    priv->dma_priv = dma_priv;
    priv->driver = pobj;
    priv->post_reconfig_delay = 0xFFFFFFFF;
    priv->reg_base = reg_base;
    priv->uio_dev = uio_dev;

    priv->m_dwVideoCaptureCaps = 0;
    memset(priv->m_dwVideoCaptureStatus, 0, sizeof(priv->m_dwVideoCaptureStatus));
    os_atomic_set(&priv->m_dwVPP1CPLDTag, 0);
    os_atomic_set(&priv->m_dwVPP2CPLDTag, 0);

    priv->m_bEnableSystemSDIANC = FALSE;
    priv->m_b16ChannelAudio = FALSE;

    for (i = 0; i < MAX_CHANNELS_PER_DEVICE; i++) {
        priv->video_cap_enabled_int[i] =
                VPP_INT_MASK_VFS_OVERFLOW
                | VPP_INT_MASK_INPUT_LOST_SYNC
                | VPP_INT_MASK_INPUT_NEW_FIELD
                | VPP_INT_MASK_VFS_FULL_FIELD_DONE
                | VPP_INT_MASK_VFS_QUARTER_FRAME_DONE
                | VPP_INT_MASK_VPP_WB_FBWR_DONE
                | VPP_INT_MASK_VPP1_FBRD_DONE
                | VPP_INT_MASK_VPP2_FBRD_DONE;
    }

    os_atomic_set(&priv->audio_completed_block_id, -1);
    priv->audio_ref_count = 0;

    priv->video_strip_enable_lock = os_spin_lock_alloc();
    priv->audio_ref_lock = os_mutex_alloc();
    if (priv->video_strip_enable_lock == NULL
            || priv->audio_ref_lock == NULL) {
        ret = OS_RETURN_NOMEM;
        goto spin_err;
    }

    priv->sw_timer_event = os_event_alloc();
    priv->vpp1_dma_done = os_event_alloc();
    priv->vpp2_dma_done = os_event_alloc();
    priv->upload_fbwr_done = os_event_alloc();
    priv->upload_dma_done = os_event_alloc();
    priv->audio_cap_done = os_event_alloc();
    priv->gpio_irq_done = os_event_alloc();
    priv->sync_meter_done = os_event_alloc();
    priv->sdi_input_done = os_event_alloc();
    priv->vad_done = os_event_alloc();
    priv->m_eventHDMIGPIO = os_event_alloc();
    priv->m_eventHDMIClkDet = os_event_alloc();
    priv->m_eventHDMIRX = os_event_alloc();
    priv->m_eventSDIANC = os_event_alloc();
    priv->m_eventSignal = os_event_alloc();
    priv->m_eventVideoDMAFrameComplete[0] = os_event_alloc();
    priv->m_eventVideoDMAFrameComplete[1] = os_event_alloc();
    priv->m_eventVPPFBRDDone[0] = os_event_alloc();
    priv->m_eventVPPFBRDDone[1] = os_event_alloc();
    if (priv->sw_timer_event == NULL
            || priv->vpp1_dma_done == NULL
            || priv->vpp2_dma_done == NULL
            || priv->upload_fbwr_done == NULL
            || priv->upload_dma_done == NULL
            || priv->audio_cap_done == NULL
            || priv->gpio_irq_done == NULL
            || priv->sync_meter_done == NULL
            || priv->sdi_input_done == NULL
            || priv->vad_done == NULL
            || priv->m_eventHDMIGPIO == NULL
            || priv->m_eventHDMIClkDet == NULL
            || priv->m_eventHDMIRX == NULL
            || priv->m_eventSDIANC == NULL
            || priv->m_eventSignal == NULL
            || priv->m_eventVideoDMAFrameComplete[0] == NULL
            || priv->m_eventVideoDMAFrameComplete[1] == NULL
            || priv->m_eventVPPFBRDDone[0] == NULL
            || priv->m_eventVPPFBRDDone[1] == NULL
        ) {
        ret = OS_RETURN_NOMEM;
        goto event_err;
    }

    priv->vpp_dma_done[0] = priv->vpp1_dma_done;
    priv->vpp_dma_done[1] = priv->vpp2_dma_done;

    for (i = 0; i < MAX_CHANNELS_PER_DEVICE; i++) {
        xi_notify_event_manager_init(&priv->notify_event_manager[i]);
    }

    // Initialize software timer event manager
    if(!xi_sw_timer_event_manager_Create(&priv->sw_timer_manager, priv->sw_timer_event, 32)) {
        ret = OS_RETURN_NOMEM;
        goto sw_timer_err;
    }

    ret = _get_device_caps(priv);
    if (ret != 0) {
        xi_debug(0, "_get_device_caps error! ret=%d\n", ret);
        goto devcap_err;
    }

    priv->m_bSimISRMode = false;
    priv->m_simIsrTimer = os_timer_alloc(_sim_irq_process, pobj);
    if (priv->m_simIsrTimer == NULL) {
        xi_debug(0, "os_timer_alloc error! ret=%d\n", ret);
        goto sim_isr_err;
    }

    /* par_dev != NULL indacate to init eeprom */
    if (priv->m_bHasPI7C9X2GX08 && priv->channel_index == 0 && par_dev != NULL) {
        ret = mw_pi7c9x2gxxx_init_eeprom_content(&priv->pi7c9x2gxxx);
        if (ret != OS_RETURN_SUCCESS) {
            xi_debug(0, "mw_pi7c9x2gxxx_init_eeprom_content failed! ret=%d\n", ret);
            goto pci_brigge_err;
        }
    }

    // init parameter
    ret = _init_parameters(priv);
    if (0 != ret) {
        xi_debug(0, "_init_parameters error!\n");
        goto par_err;
    }

    ret = _init_devices(priv);
    if (0 != ret) {
        xi_debug(0, "_init_devices error!\n");
        goto devinit_err;
    }

    if(!xi_timer_event_manager_Create(&priv->timer_event_manager, &priv->timestamp, 256)) {
        ret = OS_RETURN_NOMEM;
        goto timer_err;
    }

    // Create heap for on board memory
    {
        u32 user_buf_size;
        u32 user_buf_addr = xi_device_get_user_buffer_address(&priv->dev_info, priv->m_cChannlesPerDevice, &user_buf_size);
        ret = xi_heap_create(&priv->board_heap, user_buf_addr, user_buf_size);
        if (ret != 0)
            goto heap_err;
    }

    ret = _init_dma(priv);
    if (0 != ret) {
        xi_debug(0, "_init_dma error!\n");
        goto dma_err;
    }

    ret = _init_image_buffers(priv, nosignal_png, unsupported_png, locking_png);
    if (0 != ret) {
        xi_debug(0, "_init_image_buffers error!\n");
        goto image_err;
    }

    ret = _init_house_keeping_thread(priv);
    if (0 != ret) {
        xi_debug(0, "_init_signal_detect_thread error!\n");
        goto house_err;
    }

    return 0;

house_err:
    _deinit_image_buffers(priv);
image_err:
    _deinit_dma(priv);
dma_err:
    xi_heap_destroy(&priv->board_heap);
heap_err:
    xi_timer_event_manager_Destroy(&priv->timer_event_manager);
timer_err:
    _deinit_devices(priv);
devinit_err:
    _deinit_parameters(priv);
par_err:
pci_brigge_err:
    if (priv->m_simIsrTimer != NULL)
        os_timer_free(priv->m_simIsrTimer);
    priv->m_bSimISRMode = false;
sim_isr_err:
devcap_err:
    xi_sw_timer_event_manager_Destroy(&priv->sw_timer_manager);
sw_timer_err:
event_err:
    if (priv->sw_timer_event != NULL)
        os_event_free(priv->sw_timer_event);
    if (priv->vpp1_dma_done != NULL)
        os_event_free(priv->vpp1_dma_done);
    if (priv->vpp2_dma_done != NULL)
        os_event_free(priv->vpp2_dma_done);
    if (priv->upload_fbwr_done != NULL)
        os_event_free(priv->upload_fbwr_done);
    if (priv->upload_dma_done != NULL)
        os_event_free(priv->upload_dma_done);
    if (priv->audio_cap_done != NULL)
        os_event_free(priv->audio_cap_done);
    if (priv->gpio_irq_done != NULL)
        os_event_free(priv->gpio_irq_done);
    if (priv->sync_meter_done != NULL)
        os_event_free(priv->sync_meter_done);
    if (priv->sdi_input_done != NULL)
        os_event_free(priv->sdi_input_done);
    if (priv->vad_done != NULL)
        os_event_free(priv->vad_done);
    if (priv->m_eventHDMIGPIO != NULL)
        os_event_free(priv->m_eventHDMIGPIO);
    if (priv->m_eventHDMIClkDet != NULL)
        os_event_free(priv->m_eventHDMIClkDet);
    if (priv->m_eventHDMIRX != NULL)
        os_event_free(priv->m_eventHDMIRX);
    if (priv->m_eventSDIANC != NULL)
        os_event_free(priv->m_eventSDIANC);
    if (priv->m_eventSignal != NULL)
        os_event_free(priv->m_eventSignal);
    if (priv->m_eventVideoDMAFrameComplete[0] != NULL)
        os_event_free(priv->m_eventVideoDMAFrameComplete[0]);
    if (priv->m_eventVideoDMAFrameComplete[1] != NULL)
        os_event_free(priv->m_eventVideoDMAFrameComplete[1]);
    if (priv->m_eventVPPFBRDDone[0] != NULL)
        os_event_free(priv->m_eventVPPFBRDDone[0]);
    if (priv->m_eventVPPFBRDDone[1] != NULL)
        os_event_free(priv->m_eventVPPFBRDDone[1]);

    priv->sw_timer_event = NULL;
    priv->vpp1_dma_done = NULL;
    priv->vpp2_dma_done = NULL;
    priv->upload_fbwr_done = NULL;
    priv->upload_dma_done = NULL;
    priv->audio_cap_done = NULL;
    priv->gpio_irq_done = NULL;
    priv->sync_meter_done = NULL;
    priv->sdi_input_done = NULL;
    priv->vad_done = NULL;
    priv->m_eventHDMIGPIO = NULL;
    priv->m_eventHDMIClkDet = NULL;
    priv->m_eventHDMIRX = NULL;
    priv->m_eventSDIANC = NULL;
    priv->m_eventSignal = NULL;
    priv->m_eventVideoDMAFrameComplete[0] = NULL;
    priv->m_eventVideoDMAFrameComplete[1] = NULL;
    priv->m_eventVPPFBRDDone[0] = NULL;
    priv->m_eventVPPFBRDDone[1] = NULL;
spin_err:
    if (priv->video_strip_enable_lock != NULL)
        os_spin_lock_free(priv->video_strip_enable_lock);
    if (priv->audio_ref_lock != NULL)
        os_mutex_free(priv->audio_ref_lock);

    priv->video_strip_enable_lock = NULL;
    priv->audio_ref_lock = NULL;

    for (i = 0; i < MAX_CHANNELS_PER_DEVICE; i++) {
        xi_notify_event_manager_deinit(&priv->notify_event_manager[i]);
    }
    os_free(pobj->priv);
    pobj->priv = NULL;
    return ret;
}

void xi_driver_deinit(struct xi_driver *pobj)
{
    struct xi_driver_private *priv;
    int i;

    if (NULL == pobj || NULL == pobj->priv) {
        xi_debug(1, "xi_driver is NULL or is not inited!\n");
        return;
    }

    priv = (struct xi_driver_private *)pobj->priv;

    if (priv->m_simIsrTimer != NULL) {
        os_timer_free(priv->m_simIsrTimer);
        priv->m_simIsrTimer = NULL;
    }
    priv->m_bSimISRMode = false;

    _deinit_house_keeping_thread(priv);
    _deinit_dma(priv);
    _deinit_image_buffers(priv);

    xi_timer_event_manager_Destroy(&priv->timer_event_manager);
    xi_heap_destroy(&priv->board_heap);

    _deinit_devices(priv);
    _deinit_parameters(priv);

    xi_sw_timer_event_manager_Destroy(&priv->sw_timer_manager);
    for (i = 0; i < MAX_CHANNELS_PER_DEVICE; i++) {
        xi_notify_event_manager_deinit(&priv->notify_event_manager[i]);
    }

    if (priv->sw_timer_event != NULL)
        os_event_free(priv->sw_timer_event);
    if (priv->vpp1_dma_done != NULL)
        os_event_free(priv->vpp1_dma_done);
    if (priv->vpp2_dma_done != NULL)
        os_event_free(priv->vpp2_dma_done);
    if (priv->upload_fbwr_done != NULL)
        os_event_free(priv->upload_fbwr_done);
    if (priv->upload_dma_done != NULL)
        os_event_free(priv->upload_dma_done);
    if (priv->audio_cap_done != NULL)
        os_event_free(priv->audio_cap_done);
    if (priv->gpio_irq_done != NULL)
        os_event_free(priv->gpio_irq_done);
    if (priv->sync_meter_done != NULL)
        os_event_free(priv->sync_meter_done);
    if (priv->sdi_input_done != NULL)
        os_event_free(priv->sdi_input_done);
    if (priv->vad_done != NULL)
        os_event_free(priv->vad_done);
    if (priv->m_eventHDMIGPIO != NULL)
        os_event_free(priv->m_eventHDMIGPIO);
    if (priv->m_eventHDMIClkDet != NULL)
        os_event_free(priv->m_eventHDMIClkDet);
    if (priv->m_eventHDMIRX != NULL)
        os_event_free(priv->m_eventHDMIRX);
    if (priv->m_eventSDIANC != NULL)
        os_event_free(priv->m_eventSDIANC);
    if (priv->m_eventSignal != NULL)
        os_event_free(priv->m_eventSignal);
    if (priv->m_eventVideoDMAFrameComplete[0] != NULL)
        os_event_free(priv->m_eventVideoDMAFrameComplete[0]);
    if (priv->m_eventVideoDMAFrameComplete[1] != NULL)
        os_event_free(priv->m_eventVideoDMAFrameComplete[1]);
    if (priv->m_eventVPPFBRDDone[0] != NULL)
        os_event_free(priv->m_eventVPPFBRDDone[0]);
    if (priv->m_eventVPPFBRDDone[1] != NULL)
        os_event_free(priv->m_eventVPPFBRDDone[1]);

    if (priv->video_strip_enable_lock != NULL)
        os_spin_lock_free(priv->video_strip_enable_lock);
    if (priv->audio_ref_lock != NULL)
        os_mutex_free(priv->audio_ref_lock);

    priv->sw_timer_event = NULL;
    priv->vpp1_dma_done = NULL;
    priv->vpp2_dma_done = NULL;
    priv->upload_fbwr_done = NULL;
    priv->upload_dma_done = NULL;
    priv->audio_cap_done = NULL;
    priv->gpio_irq_done = NULL;
    priv->sync_meter_done = NULL;
    priv->sdi_input_done = NULL;
    priv->vad_done = NULL;
    priv->m_eventHDMIGPIO = NULL;
    priv->m_eventHDMIClkDet = NULL;
    priv->m_eventHDMIRX = NULL;
    priv->m_eventSDIANC = NULL;
    priv->m_eventSignal = NULL;
    priv->m_eventVideoDMAFrameComplete[0] = NULL;
    priv->m_eventVideoDMAFrameComplete[1] = NULL;
    priv->m_eventVPPFBRDDone[0] = NULL;
    priv->m_eventVPPFBRDDone[1] = NULL;

    priv->video_strip_enable_lock = NULL;
    priv->audio_ref_lock = NULL;

    os_free(pobj->priv);
    pobj->priv = NULL;
}

void xi_driver_resume(struct xi_driver *pobj,
                      png_pix_info_t *nosignal_png,
                      png_pix_info_t *unsupported_png,
                      png_pix_info_t *locking_png,
                      os_pci_dev_t pci_dev,
                      os_pci_dev_t par_dev)
{
    struct xi_driver_private *priv = pobj->priv;
    /*
    os_event_clear(&priv->sw_timer_event);
    os_event_clear(&priv->vpp1_dma_done);
    os_event_clear(&priv->vpp2_dma_done);
    os_event_clear(&priv->upload_fbwr_done);
    os_event_clear(&priv->upload_dma_done);
    os_event_clear(&priv->audio_cap_done);
    os_event_clear(&priv->gpio_irq_done);
    os_event_clear(&priv->sync_meter_done);
    os_event_clear(&priv->sdi_input_done);
    os_event_clear(&priv->vad_done);
    os_event_clear(&priv->m_eventVideoDMAFrameComplete[0]);
    os_event_clear(&priv->m_eventVideoDMAFrameComplete[1]);
    os_event_clear(&priv->m_eventVPPFBRDDone[0]);
    os_event_clear(&priv->m_eventVPPFBRDDone[1]);
    */
    mw_pi7c9x2gxxx_init(&priv->pi7c9x2gxxx, pci_dev, par_dev);
    mw_pi7c9x2gxxx_acs_bugfix(&priv->pi7c9x2gxxx);

    _init_devices(priv);
    _init_image_buffers(priv, nosignal_png, unsupported_png, locking_png);
    //_init_house_keeping_thread(priv);

    xi_timestamp_set_time(&priv->timestamp, priv->suspend_ts);
    xi_timer_event_manager_Resume(&priv->timer_event_manager);

    if (priv->audio_ref_count > 0)
        _StartAudioCapture(priv);
}

void xi_driver_sleep(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    if (priv->audio_ref_count > 0)
        _StopAudioCapture(priv);

    priv->suspend_ts = xi_timestamp_get_time(&priv->timestamp);
    //_deinit_house_keeping_thread(priv);
    _deinit_image_buffers(priv);
    _deinit_devices(priv);
}

void xi_driver_shutdown(struct xi_driver *pobj)
{
    struct xi_driver_private *priv;

    if (NULL == pobj || NULL == pobj->priv) {
        xi_debug(1, "xi_driver is NULL or is not inited!\n");
        return;
    }

    priv = (struct xi_driver_private *)pobj->priv;

    if (priv->post_reconfig_delay != 0xFFFFFFFF) {
        xi_device_reconfig(&priv->dev_info, priv->post_reconfig_delay);
        priv->post_reconfig_delay = 0xFFFFFFFF;
    }
}


/************** timestamp ***************/
long long xi_driver_get_time(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_timestamp_get_time(&priv->timestamp);
}

void xi_driver_set_time(struct xi_driver *pobj, long long lltime)
{
    struct xi_driver_private *priv = pobj->priv;

    xi_timestamp_set_time(&priv->timestamp, lltime);
}

void xi_driver_update_time(struct xi_driver *pobj, long long lltime)
{
    struct xi_driver_private *priv = pobj->priv;

    xi_timestamp_update_time(&priv->timestamp, lltime);
}



/******************* timer *****************/
xi_timer *xi_driver_new_timer(struct xi_driver *pobj, os_event_t event)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_timer_event_manager_NewTimer(&priv->timer_event_manager, event);
}

void xi_driver_delete_timer(struct xi_driver *pobj, xi_timer *ptimer)
{
    struct xi_driver_private *priv = pobj->priv;

    xi_timer_event_manager_DeleteTimer(&priv->timer_event_manager, ptimer);
}

bool xi_driver_schedual_timer(struct xi_driver *pobj, long long llexpire, xi_timer *ptimer)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_timer_event_manager_ScheduleTimer(&priv->timer_event_manager, llexpire, ptimer);
}

bool xi_driver_cancel_timer(struct xi_driver *pobj, xi_timer *ptimer)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_timer_event_manager_CancelTimer(&priv->timer_event_manager, ptimer);
}


/**************** notify *****************/
xi_notify_event *xi_driver_new_notify_event(
        struct xi_driver *pobj,
        int iChannel,
        u64 enable_bits,
        os_event_t event
        )
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_notify_event_manager_add(&priv->notify_event_manager[iChannel], enable_bits, event);
}

void xi_driver_delete_notify_event(struct xi_driver *pobj, int iChannel, xi_notify_event *event)
{
    struct xi_driver_private *priv = pobj->priv;

    xi_notify_event_manager_remove(&priv->notify_event_manager[iChannel], event);
}

void xi_driver_notify_event_set_enable_bits(struct xi_driver *pobj, int iChannel, xi_notify_event *event, u64 enable_bits)
{
    struct xi_driver_private *priv = pobj->priv;

    xi_notify_event_manager_set_enable_bits(&priv->notify_event_manager[iChannel], event, enable_bits);
}

u64 xi_driver_notify_event_get_status_bits(struct xi_driver *pobj, int iChannel, xi_notify_event *event)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_notify_event_manager_get_status_bits(&priv->notify_event_manager[iChannel], event);
}


/**************** video *****************/
static int _alloc_vpp(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return resource_pool_alloc_resource(&priv->vpp_pool);
}

static void _free_vpp(struct xi_driver *pobj, int ivpp)
{
    struct xi_driver_private *priv = pobj->priv;

    resource_pool_free_resource(&priv->vpp_pool, ivpp);
}

static void _RegulateRect(
        const RECT * pRectSrc,
        RECT * pRectDest,
        int32_t cxTotal,
        int32_t cyTotal
        )
{
    if (NULL == pRectSrc) {
        pRectDest->right = cxTotal;
        pRectDest->bottom = cyTotal;
        pRectDest->left = pRectDest->top = 0;
        return;
    }

    pRectDest->left = max(pRectSrc->left, 0);
    pRectDest->top = max(pRectSrc->top, 0);
    pRectDest->right = min(pRectSrc->right, cxTotal);
    pRectDest->bottom = min(pRectSrc->bottom, cyTotal);

    if (pRectDest->right <= pRectDest->left
            || pRectDest->bottom <= pRectDest->top) {
        pRectDest->right = cxTotal;
        pRectDest->bottom = cyTotal;
        pRectDest->left = pRectDest->top = 0;
    }
}

static void _AspectRatioConvertion(
        int cxSrc,
        int cySrc,
        int nSrcAspectX,
        int nSrcAspectY,
        RECT * pSrcRect,
        int nDestAspectX,
        int nDestAspectY,
        RECT * pDestRect,
        MWCAP_VIDEO_ASPECT_RATIO_CONVERT_MODE aspectRatioConvertMode
        )
{
    LONG cxCrop = pSrcRect->right - pSrcRect->left;
    LONG cyCrop = pSrcRect->bottom - pSrcRect->top;

    LONGLONG llFactor1 = (LONGLONG)cyCrop * cxSrc * nSrcAspectY * nDestAspectX;
    LONGLONG llFactor2 = (LONGLONG)cxCrop * cySrc * nSrcAspectX * nDestAspectY;

    if (llFactor1 == llFactor2
            || llFactor1 == 0
            || llFactor2 == 0)
        return;

    switch (aspectRatioConvertMode) {
    case MWCAP_VIDEO_ASPECT_RATIO_IGNORE:
        break;

    case MWCAP_VIDEO_ASPECT_RATIO_CROPPING:
        if (llFactor1 < llFactor2) {
            // Clip X
            int32_t cxCropNew = (int32_t)os_div64(llFactor1, (cySrc * nSrcAspectX * nDestAspectY));
            if (cxCropNew == 0) cxCropNew = 1;
            pSrcRect->left += (cxCrop - cxCropNew) >> 1;
            pSrcRect->right = pSrcRect->left + cxCropNew;
        }
        else {
            // Clip Y
            int32_t cyCropNew = (int32_t)os_div64(llFactor2, (cxSrc * nSrcAspectY * nDestAspectX));
            if (cyCropNew == 0) cyCropNew = 1;
            pSrcRect->top += (cyCrop - cyCropNew) >> 1;
            pSrcRect->bottom = pSrcRect->top + cyCropNew;
        }
        break;

    case MWCAP_VIDEO_ASPECT_RATIO_PADDING:
        if (llFactor1 < llFactor2) {
            // Padding Y
            int32_t cyDest = pDestRect->bottom - pDestRect->top;
            int32_t cyDestNew = (int32_t)os_div64(cyDest * llFactor1, llFactor2);
            if (cyDestNew == 0) cyDestNew = 1;
            pDestRect->top += (cyDest - cyDestNew) >> 1;
            pDestRect->bottom = pDestRect->top + cyDestNew;
        }
        else {
            // Padding X
            int32_t cxDest = pDestRect->right - pDestRect->left;
            int32_t cxDestNew = (int32_t)os_div64(cxDest * llFactor2, llFactor1);
            if (cxDestNew == 0) cxDestNew = 1;
            pDestRect->left += (cxDest - cxDestNew) >> 1;
            pDestRect->right = pDestRect->left + cxDestNew;
        }
        break;
    }
}

static void _vpp_begin_get_frame(
        struct xi_driver *pobj,
        int iChannel,
        int iVPP,
        int iFrame,
        BOOLEAN bShowStatus,
        struct xi_image_buffer *pOSDImage,
        const RECT * pOSDRects,
        int cOSDRects,
        const RECT *pSourceRect,
        DWORD dwFOURCC,
        int32_t cx,
        int32_t cy,
        int nAspectX,
        int nAspectY,
        MWCAP_VIDEO_COLOR_FORMAT colorFormat,
        MWCAP_VIDEO_QUANTIZATION_RANGE quantRange,
        MWCAP_VIDEO_SATURATION_RANGE satRange,
        SHORT sContrast,
        SHORT sBrightness,
        SHORT sSaturation,
        SHORT sHue,
        MWCAP_VIDEO_DEINTERLACE_MODE deinterlaceMode,
        MWCAP_VIDEO_ASPECT_RATIO_CONVERT_MODE aspectRatioConvertMode
        )
{
    struct xi_driver_private *priv = pobj->priv;

    RECT rectSrc;
    RECT rectDest = { 0, 0, cx, cy };
    int32_t xSrc;
    int32_t ySrc;
    int32_t cxSrc;
    int32_t cySrc;
    int32_t cxDest;
    int32_t cyDest;
    int frameSource;
    BOOLEAN bEnableOSD;
    BOOLEAN bDeinterlace;
    struct xi_image_buffer *pImageBuffer = NULL;

    VPP_FRAME_CONTEXT frameContext;
    BOOLEAN bInterlaced;
    unsigned short wBlankRV, wBlankGY, wBlankBU;

    // Get current frame context
    video_capture_GetFrameContext(&priv->video_cap[iChannel], iFrame, &frameContext);

    // Get state image
    if (bShowStatus) {
        VIDEO_SIGNAL_STATUS status;

        xi_front_end_GetVideoSignalStatus(priv->front_end[iChannel], &status);

        switch (status.pubStatus.state) {
        case MWCAP_VIDEO_SIGNAL_NONE:
            pImageBuffer = &priv->imgbuf_nosignal;
            break;
        case MWCAP_VIDEO_SIGNAL_UNSUPPORTED:
            pImageBuffer = &priv->imgbuf_unsupported;
            break;
        case MWCAP_VIDEO_SIGNAL_LOCKING:
            pImageBuffer = &priv->imgbuf_locking;
            break;
        default:
            break;
        }
    }

    // Get color format
    if (colorFormat == MWCAP_VIDEO_COLOR_FORMAT_UNKNOWN) {
        if (FOURCC_IsRGB(dwFOURCC))
            colorFormat = MWCAP_VIDEO_COLOR_FORMAT_RGB;
        else
            colorFormat = cy < 720 ? MWCAP_VIDEO_COLOR_FORMAT_YUV601 : MWCAP_VIDEO_COLOR_FORMAT_YUV709;
    }

    if (quantRange == MWCAP_VIDEO_QUANTIZATION_UNKNOWN)
        quantRange = (colorFormat == MWCAP_VIDEO_COLOR_FORMAT_RGB) ? MWCAP_VIDEO_QUANTIZATION_FULL : MWCAP_VIDEO_QUANTIZATION_LIMITED;

    if (satRange == MWCAP_VIDEO_SATURATION_UNKNOWN)
        satRange = (colorFormat == MWCAP_VIDEO_COLOR_FORMAT_RGB) ? MWCAP_VIDEO_SATURATION_FULL : MWCAP_VIDEO_SATURATION_LIMITED;


    // Start read frame
    if (pImageBuffer && xi_image_buffer_isvalid(pImageBuffer)) {

        video_capture_StartVideoPipeline(
                    &priv->video_cap[iChannel],
                    iVPP,

                    // Options
                    TRUE,	// b8BitsInput
                    FALSE,	// bDeinterlace
                    FALSE,	// bEnableOSD
                    FALSE,	// bAlphaEnable
                    FALSE,	// bInputSelect
                    FALSE,	// bOutputSelect
                    FALSE,	// bHostVideoBGR

                    // Scale parameters
                    xi_image_buffer_get_width(pImageBuffer),
                    xi_image_buffer_get_height(pImageBuffer),

                    0,
                    0,
                    cx,
                    cy,

                    cx,
                    cy,

                    // Color adjust parameters
                    0,
                    0,
                    0,
                    0,

                    // Blank & border
                    FALSE,
                    0,
                    0,
                    0,

                    // Color space convertion
                    dwFOURCC,

                    MWCAP_VIDEO_COLOR_FORMAT_RGB,
                    MWCAP_VIDEO_QUANTIZATION_FULL,

                    1,
                    0,

                    colorFormat,
                    quantRange,
                    satRange
                    );

        video_capture_ReadImage(
                    &priv->video_cap[iChannel],
                    iVPP,
                    xi_image_buffer_get_address(pImageBuffer),
                    xi_image_buffer_get_stride(pImageBuffer),
                    false,
                    0,
                    0,
                    xi_image_buffer_get_width(pImageBuffer),
                    xi_image_buffer_get_height(pImageBuffer),
                    true
                    );
        return;
    }

    // Calc aspect ratio parameters
    _RegulateRect(pSourceRect, &rectSrc, frameContext.pubInfo.cx, frameContext.pubInfo.cy);

    if (nAspectX == 0 || nAspectY == 0) {
        int nGCD = GCD(cx, cy);
        nAspectX = cx / nGCD;
        nAspectY = cy / nGCD;
    }

    _AspectRatioConvertion(
            frameContext.pubInfo.cx,
            frameContext.pubInfo.cy,
            frameContext.pubInfo.nAspectX,
            frameContext.pubInfo.nAspectY,
            &rectSrc,
            nAspectX,
            nAspectY,
            &rectDest,
            aspectRatioConvertMode
            );

    xSrc = rectSrc.left;
    ySrc = rectSrc.top;
    cxSrc = rectSrc.right - rectSrc.left;
    cySrc = rectSrc.bottom - rectSrc.top;
    cxDest = rectDest.right - rectDest.left;
    cyDest = rectDest.bottom - rectDest.top;

    bDeinterlace = FALSE;
    frameSource = SOURCE_FULL_FRAME;

    bInterlaced = (frameContext.pubInfo.bInterlaced && !frameContext.pubInfo.bSegmentedFrame);

    if (cxSrc >= (cxDest * 2)
            && cySrc >= (cyDest * 2)
            && (priv->m_dwVideoCaptureCaps & VPP_CAPS_QUARTER_FB_DISABLED) == 0) {
        xSrc >>= 1;
        ySrc >>= 1;
        cxSrc >>= 1;
        cySrc >>= 1;

        frameSource = SOURCE_QUARTER_FRAME;
    } else if (bInterlaced) {
        if (deinterlaceMode == MWCAP_VIDEO_DEINTERLACE_TOP_FIELD
                || deinterlaceMode == MWCAP_VIDEO_DEINTERLACE_BOTTOM_FIELD) {
            int nTop, nBottom;

            if (deinterlaceMode == MWCAP_VIDEO_DEINTERLACE_TOP_FIELD) {
                nTop = (ySrc + 1) / 2;
                nBottom = (ySrc + cySrc - 1) / 2;
                frameSource = SOURCE_TOP_FIELD;
            }
            else {
                nTop = ySrc / 2;
                nBottom = (ySrc + cySrc - 2) / 2;
                frameSource = SOURCE_BOTTOM_FIELD;
            }

            ySrc = nTop;
            cySrc = (nBottom - nTop + 1);
        } else if (deinterlaceMode == MWCAP_VIDEO_DEINTERLACE_BLEND) {
            bDeinterlace = TRUE;
        }
    }

    // Start video processing
    bEnableOSD = (pOSDImage != NULL && cOSDRects != 0
            && (priv->m_dwVideoCaptureCaps & VPP_CAPS_OSD_DISABLED) == 0);

    _GetBlackColorValue(
                &wBlankRV,
                &wBlankGY,
                &wBlankBU,
                frameContext.colorFormat,
                frameContext.quantRange,
                frameContext.sRVBUScale,
                frameContext.sRVBUOffset
                );

    sContrast = _limit(sContrast, 50, 200) - 100;
    sBrightness = _limit(sBrightness, -100, 100);
    sSaturation = _limit(sSaturation, 0, 200) - 100;
    sHue = _limit(sHue, -90, 90);

    video_capture_StartVideoPipeline(
                &priv->video_cap[iChannel],
                iVPP,

                // Options
                FALSE,			// b8BitsInput
                bDeinterlace,	// bDeinterlace
                bEnableOSD,		// bEnableOSD
                FALSE,			// bAlphaEnable
                FALSE,			// bInputSelect
                FALSE,			// bOutputSelect
                FALSE,			// bHostVideoBGR

                // Scale parameters
                cxSrc,
                cySrc,

                rectDest.left,
                rectDest.top,
                cxDest,
                cyDest,

                cx,
                cy,

                // Color adjust parameters
                sContrast,
                sBrightness,
                sSaturation,
                sHue,

                // Blank & border
                (frameContext.pubInfo.state == MWCAP_VIDEO_FRAME_STATE_INITIAL) ||
                    (pImageBuffer && !xi_image_buffer_isvalid(pImageBuffer)),
                wBlankRV,
                wBlankGY,
                wBlankBU,

                // Color space convertion
                dwFOURCC,

                frameContext.colorFormat,
                frameContext.quantRange,

                frameContext.sRVBUScale,
                frameContext.sRVBUOffset,

                colorFormat,
                quantRange,
                satRange
                );

    if (bEnableOSD) {
        video_capture_ReadOSDFrame(&priv->video_cap[iChannel],
                iVPP,
               false,
               xi_image_buffer_get_address(pOSDImage),
               xi_image_buffer_get_stride(pOSDImage),
               xi_image_buffer_get_height(pOSDImage),
               pOSDRects,
               cOSDRects);
    }

    // Read frame
    xi_notify_event_manager_set_enable_bits(
                &priv->notify_event_manager[iChannel],
                priv->m_apNotifyEventFrame[iChannel * 2 + iVPP],
                NOTIFY_EVENT_VIDEO_CAPTURE_FRAME_CHANGE(iFrame));

    while (1) {
        // Get buffered lines
        int cyBuffered = _WaitForStripeDone(
                    pobj, iChannel, iVPP, frameSource, iFrame, ySrc, cySrc, 500);

        // Transfer buffered lines
        int cyStripe = min(cyBuffered - ySrc, cySrc);
        BOOLEAN bLastStripe = (cyStripe == cySrc);

        /*
        xi_debug(50, "Frame: %d, Source: %d, Stripe Height: %d, Last Stripe: %d\n",
                 iFrame, frameSource, cyStripe, bLastStripe);
                 */

        os_event_clear(priv->m_eventVPPFBRDDone[iVPP]);

        switch (frameSource) {
        case SOURCE_FULL_FRAME:
        case SOURCE_QUARTER_FRAME:
            video_capture_ReadFrame(
                        &priv->video_cap[iChannel],
                        iVPP,
                        iFrame,
                        (frameSource == SOURCE_FULL_FRAME),
                        false,
                        xSrc,
                        ySrc,
                        cxSrc,
                        cyStripe,
                        bLastStripe,
                        NULL
                        );
            break;

        case SOURCE_TOP_FIELD:
        case SOURCE_BOTTOM_FIELD:
            video_capture_ReadField(
                        &priv->video_cap[iChannel],
                        iVPP,
                        iFrame,
                        (frameSource == SOURCE_TOP_FIELD),
                        false,
                        xSrc,
                        ySrc,
                        cxSrc,
                        cyStripe,
                        bLastStripe
                        );
            break;
        }

        ySrc += cyStripe;
        cySrc -= cyStripe;

        if (bLastStripe)
            break;

        os_event_wait(priv->m_eventVPPFBRDDone[iVPP], 500);
    }
}

static void _vpp_end_get_frame(struct xi_driver *pobj, int iChannel, int ivpp)
{
    struct xi_driver_private *priv = pobj->priv;

    video_capture_StopVideoPipeline(&priv->video_cap[iChannel], ivpp);
}

static int _WaitForStripeDone(struct xi_driver *pobj, int iChannel,int iVPP, enum FRAME_SOURCE frameSource,
                              int iFrame, int ySrc, int cySrc, DWORD dwTimeout)
{
    struct xi_driver_private *priv = pobj->priv;

    int cyBuffered = 0;
    int cyMaxLines = ySrc + cySrc;

    os_clock_kick_t start_clock = os_get_clock_kick();

    int iEvent = iChannel * 2 + iVPP;

    while (TRUE) {
        VPP_FRAME_CONTEXT frameContext;
        os_clock_kick_t duration;

        os_event_clear(priv->m_apNotifyEventFrame[iEvent]->event);

        video_capture_GetFrameContext(&priv->video_cap[iChannel], iFrame, &frameContext);

        if (frameContext.pubInfo.state == MWCAP_VIDEO_FRAME_STATE_INITIAL)
            return cyMaxLines;
        else if (frameContext.pubInfo.state == MWCAP_VIDEO_FRAME_STATE_BUFFERED)
            return cyMaxLines;
        else if (frameContext.pubInfo.state == MWCAP_VIDEO_FRAME_STATE_F1_BUFFERING) {
            if ((frameSource == SOURCE_QUARTER_FRAME)
                || (frameContext.pubInfo.bTopFieldFirst && frameSource == SOURCE_TOP_FIELD)
                || (!frameContext.pubInfo.bTopFieldFirst && frameSource == SOURCE_BOTTOM_FIELD))
                return cyMaxLines;
        }

        _EnableStripeInterrupts(priv, iChannel);

        switch (frameSource) {
        case SOURCE_FULL_FRAME:
            cyBuffered = (frameContext.pubInfo.cy < cyMaxLines)
                    ? cyMaxLines : frameContext.cyFullFrameBuffered;
            break;
        case SOURCE_QUARTER_FRAME:
            cyBuffered = (frameContext.cyQuarterFrame < cyMaxLines)
                    ? cyMaxLines : frameContext.cyQuarterFrameBuffered;
            break;
        case SOURCE_TOP_FIELD:
            cyBuffered = (((frameContext.pubInfo.cy + 1) / 2) < cyMaxLines)
                    ? cyMaxLines : frameContext.cyTopFieldBuffered;
            break;
        case SOURCE_BOTTOM_FIELD:
            cyBuffered = ((frameContext.pubInfo.cy / 2) < cyMaxLines)
                    ? cyMaxLines : frameContext.cyBottomFieldBuffered;
            break;
        default:
            xi_debug(0, "ERROR: unsupport frame source\n");
            break;
        }

        if (cyBuffered > ySrc)
            return cyBuffered;

        duration = os_get_clock_kick() - start_clock;

        if (os_clock_kick_to_msecs(duration) >= dwTimeout
                || os_event_wait(priv->m_apNotifyEventFrame[iEvent]->event,
                                 dwTimeout - (int32_t)os_clock_kick_to_msecs(duration)) <= 0)
            return cyMaxLines;
    }
}

static int _sg_transfer_frame_to_host(
        struct xi_driver *pobj,
        int ivpp,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        u32 stride,
        u32 fourcc,
        int32_t cx,
        int32_t cy,
        int bpp,
        bool bottom_up,
        long cy_notify,
        const RECT *target_rect,
        u32 timeout/* ms */
        )
{
    struct xi_driver_private *priv = pobj->priv;
    physical_addr_t phy_addr;
    int ret;

    ret = xi_pcie_dma_desc_chain_build(&priv->vpp_dma_chain[ivpp],
            sgbuf, sgnum, fourcc, cx, cy, bpp, stride, bottom_up, cy_notify, target_rect);
    if (ret != 0)
        return ret;

    phy_addr = xi_pcie_dma_desc_chain_get_address(&priv->vpp_dma_chain[ivpp]);
    xi_pcie_dma_controller_xfer_chain(&priv->vpp_dma_ctrl[ivpp],
            phy_addr.addr_high, phy_addr.addr_low,
            xi_pcie_dma_desc_chain_get_first_block_desc_count(&priv->vpp_dma_chain[ivpp]));

    return 0;
}

int xi_driver_sg_get_frame(
        struct xi_driver *pobj,
        int iChannel,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        u32 stride,
        int i_frame,
        bool show_status,
        struct xi_image_buffer *osd_image,
        const RECT *osd_rects,
        int osd_count,
        u32 fourcc,
        int32_t cx,
        int32_t cy,
        int bpp,
        bool bottom_up,
        const RECT *src_rect,
        const RECT *target_rect,
        int aspectx,
        int aspecty,
        MWCAP_VIDEO_COLOR_FORMAT color_format,
        MWCAP_VIDEO_QUANTIZATION_RANGE quant_range,
        MWCAP_VIDEO_SATURATION_RANGE sat_range,
        short contrast,
        short brightness,
        short saturation,
        short hue,
        MWCAP_VIDEO_DEINTERLACE_MODE deinterlace_mode,
        MWCAP_VIDEO_ASPECT_RATIO_CONVERT_MODE aspect_ratio_convert_mode,
        LONG cyPartialNotify,
        LPFN_PARTIAL_NOTIFY lpfnPartialNotify,
        unsigned long pvPartialNotifyContext,
        u32 timeout/* ms */
        )
{
    struct xi_driver_private *priv = pobj->priv;
    int ivpp = _alloc_vpp(pobj);
    RECT rect_target;
    int ret = 0;

    if (ivpp < 0)
        return -1;

    // Get valid target rect
    _RegulateRect(target_rect, &rect_target, cx, cy);

    // Transfer frame to destination buffer
    priv->m_alpfnPartialNotify[ivpp] = lpfnPartialNotify;
    priv->m_apvPartialNotifyContext[ivpp] = pvPartialNotifyContext;

    xi_pcie_dma_controller_enable(&priv->vpp_dma_ctrl[ivpp], false);

    _sg_transfer_frame_to_host(pobj, ivpp, sgbuf, sgnum, stride, fourcc, cx, cy,
                               bpp, bottom_up, cyPartialNotify, &rect_target, timeout);
    _vpp_begin_get_frame(pobj,
                         iChannel,
                         ivpp,
                         i_frame,
                         show_status,
                         osd_image,
                         osd_rects,
                         osd_count,
                         src_rect,
                         fourcc,
                         rect_target.right - rect_target.left,
                         rect_target.bottom - rect_target.top,
                         aspectx,
                         aspecty,
                         color_format,
                         quant_range,
                         sat_range,
                         contrast,
                         brightness,
                         saturation,
                         hue,
                         deinterlace_mode,
                         aspect_ratio_convert_mode
                         );
    if (os_event_wait(priv->m_eventVideoDMAFrameComplete[ivpp], timeout) <= 0) {
        xi_debug(0, "ERROR: wait for video frame complete event timeout!\n");
        ret = -1;
    }
    _vpp_end_get_frame(pobj, iChannel, ivpp);
    xi_pcie_dma_controller_disable(&priv->vpp_dma_ctrl[ivpp]);

    priv->m_alpfnPartialNotify[ivpp] = NULL;

    _free_vpp(pobj, ivpp);

    return ret;
}

int xi_driver_sg_put_frame(
        struct xi_driver *pobj,
        u32 dest_addr,
        u32 dest_stride,
        u32 dest_x,
        u32 dest_y,
        u32 dest_width,
        u32 dest_height,
        MWCAP_VIDEO_COLOR_FORMAT cf_dest,
        MWCAP_VIDEO_QUANTIZATION_RANGE quant_dest,
        MWCAP_VIDEO_SATURATION_RANGE sat_dest,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        bool src_bottomup,
        u32 src_stride,
        u32 src_width,
        u32 src_height,
        bool src_pixel_alpha,
        bool src_pixel_xbgr,
        u32 timeout/* ms */
        )
{
    struct xi_driver_private *priv = pobj->priv;
    int ivpp = _alloc_vpp(pobj);
    physical_addr_t phy_addr;
    RECT xfer_rect = { 0, 0, src_width, src_height };
    int ret = 0;

    if (ivpp < 0)
        return -1;

    dest_addr += dest_stride * dest_y + dest_x * 4;

    if (quant_dest == MWCAP_VIDEO_QUANTIZATION_UNKNOWN)
        quant_dest = (cf_dest == MWCAP_VIDEO_COLOR_FORMAT_RGB)
                ? MWCAP_VIDEO_QUANTIZATION_FULL : MWCAP_VIDEO_QUANTIZATION_LIMITED;

    if (sat_dest == MWCAP_VIDEO_SATURATION_UNKNOWN)
        sat_dest = (cf_dest == MWCAP_VIDEO_COLOR_FORMAT_RGB)
                ? MWCAP_VIDEO_SATURATION_FULL : MWCAP_VIDEO_SATURATION_LIMITED;


    os_event_clear(priv->upload_dma_done);
    xi_pcie_dma_controller_enable(&priv->upload_dma_ctrl, false);

    os_event_clear(priv->upload_fbwr_done);
    video_capture_EnableWriteBack(&priv->video_cap[0], true);

    src_width = src_width / priv->m_nPixelsPerCycle * priv->m_nPixelsPerCycle;
    xfer_rect.right = src_width;

    xi_pcie_dma_desc_chain_build(
                &priv->vpp_dma_chain[ivpp],
                sgbuf, sgnum, MWFOURCC_RGBA,
                src_width, src_height, 32, src_stride,
                src_bottomup, 0 ,&xfer_rect);

    video_capture_StartVideoPipeline(
                &priv->video_cap[0],
                ivpp,

                // Options
                TRUE,	// b8BitsInput
                FALSE,	// bDeinterlace
                FALSE,	// bEnableOSD
                src_pixel_alpha,	// bAlphaEnable
                TRUE,	// bInputSelect
                TRUE,	// bOutputSelect
                src_pixel_xbgr,   // bHostVideoBGR

                // Scale parameters
                src_width, src_height,
                0, 0, dest_width, dest_height,
                dest_width, dest_height,

                // Color adjust parameters
                0,				// sContrast
                0,				// sBrightness
                0,				// sSaturation
                0,				// sHue

                // Blank & border
                FALSE,

                0,
                0,
                0,

                // Color space convertion
                (cf_dest == MWCAP_VIDEO_COLOR_FORMAT_RGB) ? MWFOURCC_RGBA : MWFOURCC_VYUA,

                MWCAP_VIDEO_COLOR_FORMAT_RGB,
                MWCAP_VIDEO_QUANTIZATION_FULL,

                1,
                0,

                cf_dest,
                quant_dest,
                sat_dest
                );

    phy_addr = xi_pcie_dma_desc_chain_get_address(&priv->vpp_dma_chain[ivpp]);
    xi_pcie_dma_controller_xfer_chain(&priv->upload_dma_ctrl,
            phy_addr.addr_high, phy_addr.addr_low,
            xi_pcie_dma_desc_chain_get_first_block_desc_count(&priv->vpp_dma_chain[ivpp]));

    video_capture_WriteFrame(&priv->video_cap[0], dest_addr, dest_stride, dest_width, dest_height);
    if (os_event_wait(priv->upload_fbwr_done, timeout) <= 0) {
        xi_debug(0, "ERROR: wait for upload fbwr done irq timeout!\n");
        ret = -1;
    }
    if (os_event_wait(priv->upload_dma_done, timeout) <= 0) {
        xi_debug(0, "ERROR: wait for upload dma done irq timeout!\n");
        ret = -1;
    }

    video_capture_StopVideoPipeline(&priv->video_cap[0], ivpp);
    _free_vpp(pobj, ivpp);

    video_capture_EnableWriteBack(&priv->video_cap[0], false);

    xi_pcie_dma_controller_disable(&priv->upload_dma_ctrl);
    xi_pcie_dma_controller_clear_cpldtag_valid(&priv->upload_dma_ctrl);
    xi_pcie_dma_controller_clear_interrupt(&priv->upload_dma_ctrl);

    if (ret == 0)
        xi_debug(20, "put frame successful!\n");

    return ret;
}


/*************** irq handler *****************/
/* @ret: 0 OK, call bottom; <0 Stop.  */
int xi_driver_irq_process_top(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;
    u32 bits_val;
    u32 cpld_tag;
    BOOLEAN cpld_tag_valid;

    if (NULL == priv)
        return -1;

    if (os_atomic_read(&priv->intr_enabled) == 0)
        return -1;

    bits_val = xi_irq_get_enabled_status(&priv->irq_ctrl);
    xi_debug(20, "bits_val:0x%x\n", bits_val);

    if (bits_val == 0 || bits_val == ~0)
        return -1;

    if (priv->has_i2c) {
        if (bits_val & IRQ_MASK_I2C) {
            xi_i2c_master_irq_handler_top(&priv->i2c);
        }
    }

    if (bits_val & IRQ_MASK_GPIO) {
        xi_irq_clear_enable_bits(&priv->irq_ctrl, IRQ_MASK_GPIO);
        if (priv->front_end[0])
            xi_front_end_InterruptISRHandler(priv->front_end[0],
                    FRONT_END_INTERRUPT_GPIO);
    }

    if (bits_val & IRQ_MASK_SDI_INPUT) {
        if (priv->front_end[0]) {
            xi_irq_clear_enable_bits(&priv->irq_ctrl, IRQ_MASK_SDI_INPUT);
            xi_front_end_InterruptISRHandler(priv->front_end[0],
                    FRONT_END_INTERRUPT_SDI_INPUT);
        }
    }

    if (bits_val & IRQ_MASK_VAD) {
        if (priv->front_end[0]) {
            xi_irq_clear_enable_bits(&priv->irq_ctrl, IRQ_MASK_VAD);
            xi_front_end_InterruptISRHandler(priv->front_end[0],
                    FRONT_END_INTERRUPT_VIDEO_ACTIVITY_DETECTOR);
        }
    }

    if (bits_val & IRQ_MASK_SYNC_METER) {
        if (priv->front_end[0]) {
            xi_irq_clear_enable_bits(&priv->irq_ctrl, IRQ_MASK_SYNC_METER);
            xi_front_end_InterruptISRHandler(priv->front_end[0],
                    FRONT_END_INTERRUPT_SYNC_METER);
        }
    }

    if (bits_val & IRQ_MASK_HDMI_PHY_GPIO) {
        if (priv->front_end[0]) {
            xi_irq_clear_enable_bits(&priv->irq_ctrl, IRQ_MASK_HDMI_PHY_GPIO);
            xi_front_end_InterruptISRHandler(priv->front_end[0],
                    FRONT_END_INTERRUPT_HDMI_PHY_GPIO);
        }
    }

    if (bits_val & IRQ_MASK_HDMI_PHY_CLKDET) {
        if (priv->front_end[0]) {
            xi_irq_clear_enable_bits(&priv->irq_ctrl, IRQ_MASK_HDMI_PHY_CLKDET);
            xi_front_end_InterruptISRHandler(priv->front_end[0], FRONT_END_INTERRUPT_HDMI_PHY_CLKDET);
        }

    }

    if (bits_val & IRQ_MASK_HDMI_RX) {
        if (priv->front_end[0]) {
            xi_irq_clear_enable_bits(&priv->irq_ctrl, IRQ_MASK_HDMI_RX);
            xi_front_end_InterruptISRHandler(priv->front_end[0], FRONT_END_INTERRUPT_HDMI_RX);
        }
    }

    if (bits_val & IRQ_MASK_AUD_DMA) {
        u8 completed_block_id;
        xi_pcie_ring_dma_controller_clear_interrupt(&priv->aud_dma_ctrl);

        xi_pcie_ring_dma_controller_get_status(&priv->aud_dma_ctrl, NULL,
                &completed_block_id);

        os_atomic_set(&priv->audio_completed_block_id, completed_block_id);
    }

    if (bits_val & IRQ_MASK_AUD_CAPTURE) {
        audio_capture_stop(&priv->audio_cap);
        xi_pcie_ring_dma_controller_set_control(&priv->aud_dma_ctrl, false);

        os_atomic_set(&priv->audio_completed_block_id, -1);
    }

    if (bits_val & IRQ_MASK_VID_CAPTURE) {
        int i;

        xi_irq_clear_enable_bits(&priv->irq_ctrl, IRQ_MASK_VID_CAPTURE);

        for (i = 0; i < priv->m_cChannlesPerDevice; i++) {
            priv->m_dwVideoCaptureStatus[i] = video_capture_GetIntRawStatus(&priv->video_cap[i]);
            video_capture_ClearIntRawStatus(&priv->video_cap[i], priv->m_dwVideoCaptureStatus[i]);
        }
    }

    if (bits_val & IRQ_MASK_VPP1_DMA) {
        xi_pcie_dma_controller_clear_interrupt(&priv->vpp_dma_ctrl[0]);

        xi_pcie_dma_controller_get_status(&priv->vpp_dma_ctrl[0], NULL,
                &cpld_tag_valid, NULL, NULL, NULL, NULL, NULL);
        cpld_tag = cpld_tag_valid ? xi_pcie_dma_controller_GetCPLDTag(&priv->vpp_dma_ctrl[0]) : 0;
        os_atomic_set(&priv->m_dwVPP1CPLDTag, cpld_tag);
    }

    if (bits_val & IRQ_MASK_VPP2_DMA) {
        xi_pcie_dma_controller_clear_interrupt(&priv->vpp_dma_ctrl[1]);

        xi_pcie_dma_controller_get_status(&priv->vpp_dma_ctrl[1], NULL,
                &cpld_tag_valid, NULL, NULL, NULL, NULL, NULL);
        cpld_tag = cpld_tag_valid ? xi_pcie_dma_controller_GetCPLDTag(&priv->vpp_dma_ctrl[1]) : 0;
        os_atomic_set(&priv->m_dwVPP2CPLDTag, cpld_tag);
    }

    if (bits_val & IRQ_MASK_VPP1_MWR_DMA) {
        xi_pcie_memory_writer_clear_interrupt(&priv->vpp_memory_writer[0]);
    }

    if (bits_val & IRQ_MASK_VPP2_MWR_DMA) {
        xi_pcie_memory_writer_clear_interrupt(&priv->vpp_memory_writer[1]);
    }

    if (bits_val & IRQ_MASK_TIMESTAMP) {
        xi_timestamp_enable_alarm_int(&priv->timestamp, false);
        priv->m_llCurrTime = xi_timestamp_get_time(&priv->timestamp);
    }

    if (bits_val & IRQ_MASK_UPLOAD_DMA) {
        xi_pcie_dma_controller_clear_interrupt(&priv->upload_dma_ctrl);
    }

    if (bits_val & IRQ_MASK_ANC1) {
        if (priv->front_end[0]) {
            xi_irq_clear_enable_bits(&priv->irq_ctrl, IRQ_MASK_ANC1);
            xi_front_end_InterruptISRHandler(priv->front_end[0], FRONT_END_INTERRUPT_SDI_ANC);
        }
    }

    if (bits_val & IRQ_MASK_ANC2) {
        sdianc_ClearInterrupts(&priv->m_sdiANC);
    }

    if (bits_val != 0) {
        os_atomic_or(&priv->bits_val, bits_val);
        return 0;
    }

    return -1;
}

void xi_driver_irq_process_bottom(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;
    u32 bits_val = 0;

    bits_val = os_atomic_xchg(&priv->bits_val, 0);

    if (bits_val & IRQ_MASK_I2C) {
        xi_i2c_master_irq_handler_bottom(&priv->i2c);
    }

    if (bits_val & IRQ_MASK_VID_CAPTURE) {
        _video_capture_interrupt_handler(priv);
    }

    if (bits_val & IRQ_MASK_AUD_CAPTURE) {
        os_event_set(priv->audio_cap_done);
    } else if(bits_val & IRQ_MASK_AUD_DMA) {
        int i;

        for (i = 0; i < priv->m_cChannlesPerDevice; i++)
            xi_notify_event_manager_notify(&priv->notify_event_manager[i], MWCAP_NOTIFY_AUDIO_FRAME_BUFFERED);
    }

    if (bits_val & (IRQ_MASK_VPP1_DMA | IRQ_MASK_VPP1_MWR_DMA)) {
        os_event_set(priv->vpp_dma_done[0]);
    }

    if (bits_val & (IRQ_MASK_VPP2_DMA | IRQ_MASK_VPP2_MWR_DMA)) {
        os_event_set(priv->vpp_dma_done[1]);
    }

    if (bits_val & IRQ_MASK_VPP1_DMA) {
        u32 cpld_tag;
        cpld_tag = (u32)os_atomic_xchg(&priv->m_dwVPP1CPLDTag, 0);
        if (cpld_tag != 0) {
            BOOLEAN bFrameCompleted = ((cpld_tag & 0x80000000) != 0);
            LONG cyCompleted = (LONG)(cpld_tag & 0x7FFFFFFF);

            if (priv->m_alpfnPartialNotify[0])
                priv->m_alpfnPartialNotify[0](priv->m_apvPartialNotifyContext[0],
                        bFrameCompleted, cyCompleted);

            if (bFrameCompleted)
                os_event_set(priv->m_eventVideoDMAFrameComplete[0]);
        }
    }

    if (bits_val & IRQ_MASK_VPP2_DMA) {
        u32 cpld_tag;
        cpld_tag = (u32)os_atomic_xchg(&priv->m_dwVPP2CPLDTag, 0);
        if (cpld_tag != 0) {
            BOOLEAN bFrameCompleted = ((cpld_tag & 0x80000000) != 0);
            LONG cyCompleted = (LONG)(cpld_tag & 0x7FFFFFFF);

            if (priv->m_alpfnPartialNotify[1])
                priv->m_alpfnPartialNotify[1](priv->m_apvPartialNotifyContext[1],
                        bFrameCompleted, cyCompleted);

            if (bFrameCompleted)
                os_event_set(priv->m_eventVideoDMAFrameComplete[1]);
        }
    }

    if (bits_val & IRQ_MASK_UPLOAD_DMA) {
        os_event_set(priv->upload_dma_done);
    }

    if (bits_val & IRQ_MASK_TIMESTAMP) {
        xi_timer_event_manager_ProcessAlarm(&priv->timer_event_manager, priv->m_llCurrTime);
        xi_timestamp_enable_alarm_int(&priv->timestamp, true);
    }

    if (bits_val & IRQ_MASK_ANC2) {
        xi_notify_event_manager_notify(&priv->notify_event_manager[0],
                                       MWCAP_NOTIFY_NEW_SDI_ANC_PACKET);
    }
}

static void _sim_irq_process(void *param)
{
    struct xi_driver *pobj = (struct xi_driver *)param;
    struct xi_driver_private *priv = pobj->priv;

    if (NULL == priv)
        return;

    if (!priv->m_bSimISRMode && xi_irq_is_timeout(&priv->irq_ctrl)) {
        xi_irq_set_control(&priv->irq_ctrl, false);
        priv->m_bSimISRMode = true;
    }

    if (priv->m_bSimISRMode) {
        if (xi_driver_irq_process_top(pobj) == 0) {
            xi_driver_irq_process_bottom(pobj);
        }
        os_timer_schedule_relative(priv->m_simIsrTimer, 1);
    } else
        os_timer_schedule_relative(priv->m_simIsrTimer, 100);
}

static void _video_capture_interrupt_handler(struct xi_driver_private *priv)
{
    int i;

    for (i = 0; i < priv->m_cChannlesPerDevice; i++) {
        ULONGLONG ullNotifyMasks = 0;

        if (0 == priv->video_cap[i].reg_base)
            goto out;

        if (priv->m_dwVideoCaptureStatus[i] & (VPP_INT_MASK_VFS_OVERFLOW | VPP_INT_MASK_INPUT_LOST_SYNC)) {
            xi_debug(7, "NOTIFY_EVENT_VIDEO_CAPTURE_LOST_SYNC\n");
            ullNotifyMasks |= video_capture_StopInput(&priv->video_cap[i]);
            ullNotifyMasks |= NOTIFY_EVENT_VIDEO_CAPTURE_LOST_SYNC;
        } else {
            ullNotifyMasks |= video_capture_InterruptHandler(&priv->video_cap[i], priv->m_dwVideoCaptureStatus[i]);
        }

        if (ullNotifyMasks)
            xi_notify_event_manager_notify(&priv->notify_event_manager[i], ullNotifyMasks);
    }

    if (priv->m_dwVideoCaptureStatus[0] & VPP_INT_MASK_VPP_WB_FBWR_DONE)
        os_event_set(priv->upload_fbwr_done);

    if (priv->m_dwVideoCaptureStatus[0] & VPP_INT_MASK_VPP1_FBRD_DONE)
        os_event_set(priv->m_eventVPPFBRDDone[0]);

    if (priv->m_dwVideoCaptureStatus[0] & VPP_INT_MASK_VPP2_FBRD_DONE)
        os_event_set(priv->m_eventVPPFBRDDone[1]);

out:
    xi_irq_set_enable_bits(&priv->irq_ctrl, IRQ_MASK_VID_CAPTURE);
}

static int house_keeping_thread_proc(void *data)
{
    struct xi_driver_private *priv = data;

    os_event_t wait_events[] = {
        xi_thread_request_get_request_event(&priv->house_thread_request),
        priv->gpio_irq_done,
        priv->sync_meter_done,
        priv->sdi_input_done,
        priv->vad_done,
        priv->sw_timer_event,
        priv->m_eventSignal,
        priv->audio_cap_done,
        priv->m_eventHDMIGPIO,
        priv->m_eventHDMIClkDet,
        priv->m_eventHDMIRX,
        priv->m_eventSDIANC
    };
    os_clock_kick_t current_time;
    os_clock_kick_t dwPrevFanAdjustTime;
    os_clock_kick_t dwPrevHouseKeepingTime[MAX_CHANNELS_PER_DEVICE];
    int32_t timeout;
    int house_keeping_interval = 50;
    int i;

    xi_debug(5, "house keeping thread started\n");

#if defined(__linux__)
    os_set_freezable();
#endif

    dwPrevFanAdjustTime = os_get_clock_kick();
    current_time = os_get_clock_kick();
    for (i = 0; i < priv->m_cChannlesPerDevice; i++) {
        dwPrevHouseKeepingTime[i] = current_time;
    }

    _UpdateEDID(priv);

    for (;;) {
        timeout = 50;
        os_event_wait_for_multiple(wait_events, ARRAY_SIZE(wait_events), timeout);

        if (xi_thread_request_wait_for_request(&priv->house_thread_request,
                    NULL, NULL, NULL, NULL, 0) > 0) {
            if (!_process_request(priv))
                break;
        }

        if (os_event_try_wait(priv->sw_timer_event)) {
            xi_timer *ptimer = NULL;
            while ((ptimer = xi_sw_timer_event_manager_GetExpiredTimer(
                            &priv->sw_timer_manager)) != NULL) {
                xi_front_end_TimerExpireHandler(priv->front_end[0], ptimer);
            }
        }

        if (os_event_try_wait(priv->m_eventSignal)) {
            for (i = 0; i < priv->m_cChannlesPerDevice; i++) {
                u64 status_bits = xi_notify_event_manager_get_status_bits(
                                      &priv->notify_event_manager[i], priv->signal_notify_event[i]);

                if (status_bits & MWCAP_NOTIFY_VIDEO_INPUT_SOURCE_CHANGE)
                    parameter_SetVideoInputSource(&priv->parameter_manager,
                                                  xi_front_end_GetVideoInputSource(priv->front_end[0]));

                if (status_bits & MWCAP_NOTIFY_AUDIO_INPUT_SOURCE_CHANGE)
                    parameter_SetAudioInputSource(&priv->parameter_manager,
                                                  xi_front_end_GetAudioInputSource(priv->front_end[0]));

                if (status_bits & MWCAP_NOTIFY_VIDEO_SIGNAL_CHANGE) {
                    VIDEO_SIGNAL_STATUS videoSignalStatus;

                    xi_debug(8, "MWCAP_NOTIFY_VIDEO_SIGNAL_CHANGE\n");
                    xi_front_end_GetVideoSignalStatus(priv->front_end[i], &videoSignalStatus);
                    xi_debug(5, "videoSignalStatus state:%d\n", videoSignalStatus.pubStatus.state);

                    if (videoSignalStatus.pubStatus.state == MWCAP_VIDEO_SIGNAL_LOCKED) {
                        house_keeping_interval = 100;
                    } else {
                        house_keeping_interval = 50;
                    }
                }

                if (status_bits & (NOTIFY_EVENT_VIDEO_CAPTURE_LOST_SYNC| NOTIFY_EVENT_VIDEO_CAPTURE_SETTINGS_CHANGE)) {
                    xi_debug(8, "NOTIFY_EVENT_VIDEO_CAPTURE_SETTINGS_CHANGE or NOTIFY_EVENT_VIDEO_CAPTURE_LOST_SYNC\n");
                    _ResetVideoCapture(priv, i);
                }

                if (status_bits & NOTIFY_EVENT_AUDIO_CAPTURE_SETTINGS_CHANGE) {
                    xi_debug(8, "NOTIFY_EVENT_AUDIO_CAPTURE_SETTINGS_CHANGE\n");
                    _ResetAudioCapture(priv);
                }

                if (status_bits & MWCAP_NOTIFY_HDMI_INFOFRAME_VS) {
                    HDMI_INFOFRAME_PACKET vsPacket;
                    BOOLEAN bValid = xi_front_end_GetInfoFramePacket(priv->front_end[i],
                                                                     MWCAP_HDMI_INFOFRAME_ID_VS, &vsPacket);

                    xi_debug(8, "MWCAP_NOTIFY_HDMI_INFOFRAME_VS\n");

                    if (bValid) {
                        DWORD dwRegistrationId = hdmi_payload_GetRegistrationId(
                                                     &vsPacket.vsInfoFramePayload);

                        BOOLEAN bTimeCodeValid;
                        MWCAP_SMPTE_TIMECODE timeCode;

                        switch (dwRegistrationId) {
                        case 0x080046:
                            timeCode.byFrames = vsPacket.vsInfoFramePayload.abyVSData[4];
                            timeCode.bySeconds = vsPacket.vsInfoFramePayload.abyVSData[5];
                            timeCode.byMinutes = vsPacket.vsInfoFramePayload.abyVSData[6];
                            timeCode.byHours = vsPacket.vsInfoFramePayload.abyVSData[7];
                            bTimeCodeValid = TRUE;
                            break;

                        case 0x000085:
                            timeCode.byFrames = vsPacket.vsInfoFramePayload.abyVSData[2];
                            timeCode.bySeconds = vsPacket.vsInfoFramePayload.abyVSData[3];
                            timeCode.byMinutes = vsPacket.vsInfoFramePayload.abyVSData[4];
                            timeCode.byHours = vsPacket.vsInfoFramePayload.abyVSData[5];
                            bTimeCodeValid = TRUE;
                            break;

                        default:
                            bTimeCodeValid = FALSE;
                        }

                        if (bTimeCodeValid) {
                            ULONGLONG llNotifyMasks = video_capture_SMPTETimeCodeHandler(
                                                          &priv->video_cap[i], &timeCode);

                            if (llNotifyMasks)
                                xi_notify_event_manager_notify(&priv->notify_event_manager[i],
                                                               llNotifyMasks);
                        }
                    }
                }

                if (status_bits & MWCAP_NOTIFY_LOOP_THROUGH_EDID_CHANGED) {
                    xi_debug(20, "MWCAP_NOTIFY_LOOP_THROUGH_EDID_CHANGED\n");
                    _UpdateEDID(priv);
                }

                if (priv->m_bEnableSystemSDIANC && (status_bits & MWCAP_NOTIFY_NEW_SDI_ANC_PACKET)) {
                    MWCAP_SDI_ANC_PACKET ancPacket;
                    xi_debug(50, "MWCAP_NOTIFY_NEW_SDI_ANC_PACKET\n");

                    do {
                        xi_driver_get_sdianc_packet(priv->driver, &ancPacket);
                        if (ancPacket.byDID == 0)
                            break;
                        video_capture_ANCPacketHandler(&priv->video_cap[0], &ancPacket);
                    } while (1);
                }
            }
        }

        if (os_event_try_wait(priv->audio_cap_done)) {
            _ResetAudioCapture(priv);
        }

        current_time = os_get_clock_kick();
        for (i = 0; i < priv->m_cChannlesPerDevice; i++) {
            if (os_clock_kick_before_eq(dwPrevHouseKeepingTime[i] + os_msecs_to_clock_kick(house_keeping_interval),
                                        current_time)) {
                dwPrevHouseKeepingTime[i] = current_time;
            }

            _UpdateStripeInterrupts(priv, i);
        }

        current_time = os_get_clock_kick();
        if (os_clock_kick_before_eq(current_time, dwPrevFanAdjustTime + 10000)) {
            _AdjustFanDuty(priv);
            dwPrevFanAdjustTime = current_time;
        }

#if defined(__linux__)
        os_try_to_freeze();
#endif
    }

    for (i = 0; i < priv->m_cChannlesPerDevice; i++)
        _StopVideoCapture(priv, i);
    _StopAudioCapture(priv);

    os_thread_terminate_self(priv->house_kthread);

    return 0;
}


/******************* signal functions *******************/
void xi_driver_set_input_source_scan(struct xi_driver *pobj, bool enable_scan)
{
    struct xi_driver_private *priv = pobj->priv;

    xi_thread_request_request(&priv->house_thread_request, REQUEST_SET_INPUT_SORUCE_SCAN,
            (void *)enable_scan, NULL, NULL);
}

bool xi_driver_get_input_source_scan(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return parameter_GetInputSourceScan(&priv->parameter_manager);
}

bool xi_driver_get_input_source_scan_state(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;
    bool scanning = false;

    xi_thread_request_request(&priv->house_thread_request, REQUEST_GET_INPUT_SOURCE_SCAN_STATE,
            (void *)&scanning, NULL, NULL);

    return scanning;
}

void xi_driver_set_av_input_source_link(struct xi_driver *pobj, bool link)
{
    struct xi_driver_private *priv = pobj->priv;

    xi_thread_request_request(&priv->house_thread_request, REQUEST_SET_AV_INPUT_SOURCE_LINK,
            (void *)link, NULL, NULL);
}

bool xi_driver_get_av_input_source_link(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return parameter_GetAVInputSourceLink(&priv->parameter_manager);
}

void xi_driver_set_video_input_source(struct xi_driver *pobj, u32 input_source)
{
    struct xi_driver_private *priv = pobj->priv;

    xi_thread_request_request(&priv->house_thread_request, REQUEST_SET_VIDEO_INPUT_SOURCE,
            (void *)(unsigned long)input_source, NULL, NULL);
}

u32 xi_driver_get_video_input_source(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;
    u32 input_source = INPUT_SOURCE(MWCAP_VIDEO_INPUT_TYPE_NONE, 0);

    xi_thread_request_request(&priv->house_thread_request, REQUEST_GET_VIDEO_INPUT_SOURCE,
            &input_source, NULL, NULL);

    xi_debug(5, "input_source:%x\n", input_source);

    return input_source;
}

void xi_driver_set_audio_input_source(struct xi_driver *pobj, u32 input_source)
{
    struct xi_driver_private *priv = pobj->priv;

    xi_thread_request_request(&priv->house_thread_request, REQUEST_SET_AUDIO_INPUT_SOURCE,
            (void *)(unsigned long)input_source, NULL, NULL);
}

u32 xi_driver_get_audio_input_source(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;
    u32 input_source = INPUT_SOURCE(MWCAP_AUDIO_INPUT_TYPE_NONE, 0);

    xi_thread_request_request(&priv->house_thread_request, REQUEST_GET_AUDIO_INPUT_SOURCE,
            &input_source, NULL, NULL);

    return input_source;
}

const u32 *xi_driver_get_supported_video_input_sources(struct xi_driver *pobj, int *count)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_front_end_GetSupportedVideoInputSources(priv->front_end[0], count);
}

const u32 *xi_driver_get_supported_audio_input_sources(struct xi_driver *pobj, int *count)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_front_end_GetSupportedAudioInputSources(priv->front_end[0], count);
}

void xi_driver_get_input_specific_status(struct xi_driver *pobj, int iChannel, INPUT_SPECIFIC_STATUS *status)
{
    struct xi_driver_private *priv = pobj->priv;

    xi_front_end_GetInputSpecificStatus(priv->front_end[iChannel], status);
}

void xi_driver_get_video_signal_status(struct xi_driver *pobj, int iChannel, VIDEO_SIGNAL_STATUS *status)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_front_end_GetVideoSignalStatus(priv->front_end[iChannel], status);
}

void xi_driver_get_audio_signal_status(struct xi_driver *pobj, int iChannel, AUDIO_SIGNAL_STATUS *status)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_front_end_GetAudioSignalStatus(priv->front_end[iChannel], status);
}

void xi_driver_get_sdi_link_status(struct xi_driver *pobj, int iChannel, MWCAP_SDI_SINGLE_LINK_STATUS *status)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_front_end_GetSDILinkStatus(priv->front_end[iChannel], status);
}

int xi_driver_get_sdi_quad_link_manual_mode(struct xi_driver *pobj, int iChannel)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_front_end_GetSDIQuadLinkManualMode(priv->front_end[iChannel]);
}

int xi_driver_set_sdi_quad_link_manual_mode(struct xi_driver *pobj, int iChannel, int nMode)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_front_end_SetSDIQuadLinkManualMode(priv->front_end[iChannel], nMode);
}


/******************* audio process *********************/
int xi_driver_get_completed_audio_block_id(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return os_atomic_read(&priv->audio_completed_block_id);
}

const AUDIO_CAPTURE_FRAME *xi_driver_get_audio_capture_frame(
        struct xi_driver *pobj, int block_id)
{
    struct xi_driver_private *priv = pobj->priv;

    return (const AUDIO_CAPTURE_FRAME *)
        os_contig_dma_get_virt(priv->audio_dma_buf) + block_id;
}

int xi_driver_acquire_audio_capture(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    os_mutex_lock(priv->audio_ref_lock);
    if (++priv->audio_ref_count == 1) {
        _StartAudioCapture(priv);
    }
    os_mutex_unlock(priv->audio_ref_lock);

    return 0;
}

void xi_driver_release_audio_capture(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    os_mutex_lock(priv->audio_ref_lock);
    if (--priv->audio_ref_count == 0) {
        _StopAudioCapture(priv);
    }
    os_mutex_unlock(priv->audio_ref_lock);
}

static void NormalizeAudioSampleData(
        AUDIO_INPUT_MODE mode,
        BOOLEAN bMSBFirst,
        DWORD dwMasks,
        int cPaddingBits,
        const DWORD *pdwSrc,
        DWORD *pdwDest,
        DWORD dwCount
        )
{
    DWORD i;

    switch (mode) {
    case AUDIO_INPUT_SPDIF:
    case AUDIO_INPUT_INTERNAL:
        for (i = 0; i < dwCount; i++)
            pdwDest[i] = (pdwSrc[i] << 4) & dwMasks;
        break;

    case AUDIO_INPUT_I2S:
        for (i = 0; i < dwCount; i++)
            pdwDest[i] = (pdwSrc[i] << cPaddingBits) & dwMasks;
        break;

    case AUDIO_INPUT_SERIAL:
        if (bMSBFirst) {
            for (i = 0; i < dwCount; i++)
                pdwDest[i] = (pdwSrc[i] << cPaddingBits) & dwMasks;
        }
        else {
            for (i = 0; i < dwCount; i++)
                pdwDest[i] = pdwSrc[i] & dwMasks;
        }
        break;

    default:
        os_memset(pdwDest, 0, dwCount * sizeof(DWORD));
        break;
    }
}

static void NormalizeAudioSampleDataMux(
        AUDIO_INPUT_MODE mode,
        BOOLEAN bMSBFirst,
        DWORD dwMasks,
        int cPaddingBits,
        const DWORD *pdwSrc,
        DWORD *pdwDest,
        DWORD dwCount,
        int iChannel
        )
{
    DWORD i;

    switch (mode) {
    case AUDIO_INPUT_SPDIF:
    case AUDIO_INPUT_INTERNAL:
        for (i = 0; i < dwCount; i+=8) {
            pdwDest[i] = (pdwSrc[i*2 + iChannel] << 4) & dwMasks;
            pdwDest[i + 4] = (pdwSrc[i*2 + iChannel + 8] << 4) & dwMasks;
        }
        break;

    case AUDIO_INPUT_I2S:
        for (i = 0; i < dwCount; i+=8) {
            pdwDest[i] = (pdwSrc[i*2 + iChannel] << cPaddingBits) & dwMasks;
            pdwDest[i + 4] = (pdwSrc[i*2 + iChannel + 8] << cPaddingBits) & dwMasks;
        }
        break;

    case AUDIO_INPUT_SERIAL:
        if (bMSBFirst) {
            for (i = 0; i < dwCount; i+=8) {
                pdwDest[i] = (pdwSrc[i*2 + iChannel] << cPaddingBits) & dwMasks;
                pdwDest[i + 4] = (pdwSrc[i*2 + iChannel + 8] << cPaddingBits) & dwMasks;
            }
        }
        else {
            for (i = 0; i < dwCount; i+=8) {
                pdwDest[i] = pdwSrc[i*2 + iChannel] & dwMasks;
                pdwDest[i + 4] = pdwSrc[i*2 + iChannel + 8] & dwMasks;
            }
        }
        break;

    default:
        os_memset(pdwDest, 0, dwCount * sizeof(DWORD));
        break;
    }
}

void xi_driver_normalize_audio_capture_frame(
        struct xi_driver *pobj,
        const AUDIO_SIGNAL_STATUS * pStatus,
        const AUDIO_CAPTURE_FRAME * pAudioFrameSrc,
        MWCAP_AUDIO_CAPTURE_FRAME * pAudioFrameDest
        )
{
    struct xi_driver_private *priv = pobj->priv;

    // Num data bits: byMSBIndex + 1
    // Num valid bits: cBitsPerSample
    //
    // SPDIF: [P,C,U,V,DATA_24BITS,1'b0,valid,left,start]
    // AUDIO_INPUT_SERIAL: MSB first: [PADDING_BITS, DATA_BITS], LSB first: [DATA_BITS, PADDING_BITS]
    // I2S: [PADDING_BITS, DATA_BITS]

    DWORD dwMasks;
    int cPaddingBits;

    pAudioFrameDest->dwSyncCode = pAudioFrameSrc->dwSyncCode;
    pAudioFrameDest->llTimestamp = pAudioFrameSrc->llTimestamp;
    pAudioFrameDest->dwFlags = priv->m_b16ChannelAudio ? MWCAP_AUDIO_16_CHANNELS : 0;

    dwMasks = (DWORD)(((1ULL << pStatus->pubStatus.cBitsPerSample) - 1)
            << (32 - pStatus->pubStatus.cBitsPerSample));
    cPaddingBits = 32 - (pStatus->byMSBIndex + 1);

    NormalizeAudioSampleData(
                pStatus->audioInputMode,
                pStatus->bMSBFirst,
                dwMasks,
                cPaddingBits,
                pAudioFrameSrc->adwSamples,
                pAudioFrameDest->adwSamples,
                ARRAY_SIZE(pAudioFrameDest->adwSamples)
                );
}

void xi_driver_normalize_audio_capture_frame_mux(
        struct xi_driver *pobj,
        int iChannel,
        const AUDIO_SIGNAL_STATUS * pStatus,
        const AUDIO_CAPTURE_FRAME * pAudioFrameSrc1,
        const AUDIO_CAPTURE_FRAME * pAudioFrameSrc2,
        MWCAP_AUDIO_CAPTURE_FRAME * pAudioFrameDest
        )
{
    DWORD dwMasks;
    int cPaddingBits;

    pAudioFrameDest->dwSyncCode = pAudioFrameSrc1->dwSyncCode;
    pAudioFrameDest->llTimestamp = pAudioFrameSrc1->llTimestamp;
    pAudioFrameDest->dwFlags = 0;

    dwMasks = (DWORD)(((1ULL << pStatus->pubStatus.cBitsPerSample) - 1) << (32 - pStatus->pubStatus.cBitsPerSample));
    cPaddingBits = 32 - (pStatus->byMSBIndex + 1);

    NormalizeAudioSampleDataMux(
                pStatus->audioInputMode,
                pStatus->bMSBFirst,
                dwMasks,
                cPaddingBits,
                pAudioFrameSrc1->adwSamples,
                pAudioFrameDest->adwSamples,
                ARRAY_SIZE(pAudioFrameDest->adwSamples) / 2,
                iChannel
                );

    NormalizeAudioSampleDataMux(
                pStatus->audioInputMode,
                pStatus->bMSBFirst,
                dwMasks,
                cPaddingBits,
                pAudioFrameSrc2->adwSamples,
                pAudioFrameDest->adwSamples + (ARRAY_SIZE(pAudioFrameDest->adwSamples) / 2),
                ARRAY_SIZE(pAudioFrameDest->adwSamples) / 2,
                iChannel
                );
}

int xi_driver_get_normalized_audio_capture_frame(struct xi_driver *pobj,
        int iChannel, int *piBlockId, MWCAP_AUDIO_CAPTURE_FRAME *audio_frame_dest)
{
    struct xi_driver_private *priv = pobj->priv;
    AUDIO_SIGNAL_STATUS audio_signal_status;
    LONG lBlockId;
    LONG lNewBlockId;
    LONG lBlocksPerFrame;
    LONG lNewBlockCount;
    const AUDIO_CAPTURE_FRAME * audio_frame;

    if (NULL == piBlockId)
        return OS_RETURN_INVAL;

    lBlockId = *piBlockId;

    lNewBlockId = xi_driver_get_completed_audio_block_id(pobj);
    if (lBlockId < 0)
        lBlockId = lNewBlockId;

    lBlocksPerFrame = priv->m_b16To2AudioMux ? 2 : 1;
    lNewBlockCount = (lNewBlockId < lBlockId) ? (lNewBlockId + AUDIO_FRAME_COUNT - lBlockId + 1) : (lNewBlockId - lBlockId + 1);
    if (lNewBlockId < 0 || lNewBlockCount < lBlocksPerFrame || lNewBlockCount >= AUDIO_FRAME_COUNT - 1) {
        *piBlockId = lBlockId;
        return OS_RETURN_NODATA;
    }

    xi_driver_get_audio_signal_status(pobj, iChannel, &audio_signal_status);

    audio_frame = xi_driver_get_audio_capture_frame(pobj, lBlockId);
    if (priv->m_b16To2AudioMux)
        xi_driver_normalize_audio_capture_frame_mux(pobj, iChannel, &audio_signal_status, audio_frame, xi_driver_get_audio_capture_frame(pobj, (lBlockId + 1) % AUDIO_FRAME_COUNT), audio_frame_dest);
    else
        xi_driver_normalize_audio_capture_frame(pobj, &audio_signal_status, audio_frame, audio_frame_dest);

    audio_frame_dest->iFrame = lBlockId;
    audio_frame_dest->cFrameCount = AUDIO_FRAME_COUNT;

    lBlockId = (lBlockId + lBlocksPerFrame) % AUDIO_FRAME_COUNT;

    *piBlockId = lBlockId;

    return 0;
}

void xi_driver_SetFrontEndVolume(
        struct xi_driver *pobj,
        int i,
        int iChannel,
        int nVolume
        )
{
    struct xi_driver_private *priv = pobj->priv;

    xi_front_end_SetVolume(priv->front_end[priv->m_bMultiChannelFPGA ? i : 0], iChannel, nVolume);
}

int xi_driver_GetFrontEndVolume(
        struct xi_driver *pobj,
        int i,
        int iChannel
        )
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_front_end_GetVolume(priv->front_end[priv->m_bMultiChannelFPGA ? i : 0], iChannel);
}

static bool _process_request(struct xi_driver_private *priv)
{
    bool ret = true;
    void *param1;
    void *param2;
    void *param3;
    u32 cmd;

    cmd = xi_thread_request_get_request_command(&priv->house_thread_request,
            &param1, &param2, &param3);
    switch (cmd) {
    case REQUEST_EXIT_THREAD:
        ret = FALSE;
        xi_thread_request_ack(&priv->house_thread_request, 0);
        break;

    case REQUEST_SET_INPUT_SORUCE_SCAN:
        {
            bool enable_scan = (bool)param1;

            parameter_SetInputSourceScan(&priv->parameter_manager, enable_scan);
            xi_front_end_SetInputSourceScan(priv->front_end[0], enable_scan);
            xi_thread_request_ack(&priv->house_thread_request, 0);
            break;
        }

    case REQUEST_GET_INPUT_SOURCE_SCAN_STATE:
        {
            *(BOOLEAN *)param1 = xi_front_end_IsScanning(priv->front_end[0]);
            xi_thread_request_ack(&priv->house_thread_request, 0);
            break;
        }

    case REQUEST_SET_AV_INPUT_SOURCE_LINK:
        {
            bool av_link = (bool)param1;

            parameter_SetAVInputSourceLink(&priv->parameter_manager, av_link);
            xi_front_end_SetAVInputSourceLink(priv->front_end[0], av_link);
            xi_thread_request_ack(&priv->house_thread_request, 0);
            break;
        }

    case REQUEST_SET_VIDEO_INPUT_SOURCE:
        parameter_SetVideoInputSource(&priv->parameter_manager, (u32)(unsigned long)param1);
        xi_front_end_SetVideoInputSource(priv->front_end[0], (u32)(unsigned long)param1);
        xi_thread_request_ack(&priv->house_thread_request, 0);
        break;

    case REQUEST_GET_VIDEO_INPUT_SOURCE:
        *(u32 *)param1 = xi_front_end_GetVideoInputSource(priv->front_end[0]);
        xi_thread_request_ack(&priv->house_thread_request, 0);
        break;

    case REQUEST_SET_AUDIO_INPUT_SOURCE:
        parameter_SetAudioInputSource(&priv->parameter_manager, (u32)(unsigned long)param1);
        xi_front_end_SetAudioInputSource(priv->front_end[0], (u32)(unsigned long)param1);
        xi_thread_request_ack(&priv->house_thread_request, 0);
        break;

    case REQUEST_GET_AUDIO_INPUT_SOURCE:
        *(u32 *)param1 = xi_front_end_GetAudioInputSource(priv->front_end[0]);
        xi_thread_request_ack(&priv->house_thread_request, 0);
        break;

    case REQUEST_SET_EDID:
        parameter_SetEDID(&priv->parameter_manager, (const u8 *)param1, (short)(unsigned long)param2);
        _UpdateEDID(priv);
        xi_thread_request_ack(&priv->house_thread_request, 0);
        break;

    case REQUEST_SET_ENABLE_HDCP:
        parameter_SetEnableHDCP(&priv->parameter_manager, (bool)param1);
        xi_front_end_SetEnableHDCP(priv->front_end[0], (bool)param1);
        xi_thread_request_ack(&priv->house_thread_request, 0);
        break;

    case REQUEST_SET_AUTO_H_ALIGN:
        {
            bool auto_align = (bool)param1;

            parameter_SetAutoHAlign(&priv->parameter_manager, auto_align);
            xi_thread_request_ack(&priv->house_thread_request, 0);
                break;
        }

    case REQUEST_SET_SAMPLING_PHASE:
        {
            BYTE bySamplingPhase = *(BYTE *)param1;

            if (bySamplingPhase == 0xFF)
                xi_front_end_TriggerSamplingPhaseAdjust(priv->front_end[0]);
            else if (!parameter_GetAutoSamplingPhaseAdjust(&priv->parameter_manager))
                xi_front_end_SetSamplingPhase(priv->front_end[0], bySamplingPhase);

            xi_thread_request_ack(&priv->house_thread_request, 0);
            break;
        }

    case REQUEST_SET_VIDEO_CUSTOM_TIMING_ARRAY:
        {
            os_ptrdiff_t cItems = (os_ptrdiff_t)param2;
            parameter_SetCustomVideoTimings(
                        &priv->parameter_manager,
                        (MWCAP_VIDEO_CUSTOM_TIMING *)param1, (int)cItems);
            xi_thread_request_ack(&priv->house_thread_request, 0);
            break;
        }

    case REQUEST_SET_VIDEO_CUSTOM_RESOLUTION_ARRAY:
        {
            MWCAP_VIDEO_TIMING timing;
            os_ptrdiff_t cItems = (os_ptrdiff_t)param2;

            parameter_SetVideoResolutions(
                        &priv->parameter_manager,
                        (MWCAP_SIZE *)param1, (int)cItems);

            timing.dwType = MWCAP_VIDEO_TIMING_NONE;
            xi_front_end_SetCurrentVideoTiming(priv->front_end[0], &timing);

            xi_thread_request_ack(&priv->house_thread_request, 0);
            break;
        }

    default:
        xi_debug(0, "Error request: %d\n", cmd);
        break;
    }

    return ret;
}

static void _ResetVideoCapture(struct xi_driver_private *priv, int iChannel)
{
    VIDEO_CAPTURE_SETTINGS settings;

    _StopVideoCapture(priv, iChannel);

    xi_front_end_GetVideoCaptureSettings(priv->front_end[iChannel], &settings);
    video_capture_SetControlPortValue(&priv->video_cap[iChannel], 0);
    video_capture_SetControlPortValue(&priv->video_cap[iChannel],
                                      settings.byInputSelect);

    xi_debug(10, "settings.bValid:%d,byInputSelect:%d\n", settings.bValid, settings.byInputSelect);
    if (!settings.bValid)
        return;

    os_msleep(50); // wait for stop complete

    xi_debug(10, "iChannel:%d,bFramePaking:%d,yRightFrame:%d,bInterlacedFramePacking:%d,cyInterlacedFramePacking:%d,bInterlaced:%d,bSegmentedFrame:%d\n",
        iChannel, settings.bFramePaking, settings.yRightFrame, settings.bInterlacedFramePacking, settings.cyInterlacedFramePacking, settings.bInterlaced, settings.bSegmentedFrame);
    xi_debug(10, "colorFormat:%d,quantRange:%d,sRVBUScale:%d,sRVBUOffset:%d\n",
        settings.colorFormat, settings.quantRange, settings.sRVBUScale, settings.sRVBUOffset);
    xi_debug(10, "x:%d,y:%d,cx:%d,cy:%d,nAspectX:%d,nAspectY:%d\n",
        settings.x, settings.y, settings.cx, settings.cy, settings.nAspectX, settings.nAspectY);

    video_capture_StartInput(
                &priv->video_cap[iChannel],
                FALSE,
                FALSE,
                settings.bFramePaking,
                settings.yRightFrame,
                settings.bInterlacedFramePacking,
                settings.cyInterlacedFramePacking,
                settings.bInterlaced,
                settings.bSegmentedFrame,
                FALSE,
                parameter_GetTopFieldFirst(&priv->parameter_manager),
                settings.colorFormat,
                settings.quantRange,
                settings.sRVBUScale,
                settings.sRVBUOffset,
                settings.x,
                settings.y,
                settings.cx,
                settings.cy,
                settings.nAspectX,
                settings.nAspectY,
                parameter_GetLowLatencyStripeHeight(&priv->parameter_manager),
                parameter_GetLowLatencyStripeHeight(&priv->parameter_manager) / 2,
                video_capture_GetInputFrameID(&priv->video_cap[iChannel])
                );
}

static void _StopVideoCapture(struct xi_driver_private *priv, int iChannel)
{
    ULONGLONG ullMasks = video_capture_StopInput(&priv->video_cap[iChannel]);

    if (ullMasks != 0)
        xi_notify_event_manager_notify(&priv->notify_event_manager[iChannel], ullMasks);
}

static void _StartAudioCapture(struct xi_driver_private *priv)
{
    mw_physical_addr_t dma_address;
    AUDIO_CAPTURE_SETTINGS settings;
    int i;

    dma_address = os_contig_dma_get_phys(priv->audio_dma_buf);

    xi_pcie_ring_dma_controller_setup_ring_buffer(&priv->aud_dma_ctrl,
            sizeof(dma_address) > 4 ? ((dma_address >> 32) & 0xFFFFFFFF) : 0,
            dma_address & 0xFFFFFFFF,
            sizeof(AUDIO_CAPTURE_FRAME),
            AUDIO_FRAME_COUNT);
    xi_pcie_ring_dma_controller_enable(&priv->aud_dma_ctrl);

    for (i = 0; i < priv->m_cChannlesPerDevice; i++)
        xi_notify_event_manager_notify(&priv->notify_event_manager[i],
                                       MWCAP_NOTIFY_AUDIO_INPUT_RESET);

    xi_front_end_GetAudioCaptureSettings(priv->front_end[0], &settings);

    audio_capture_set_control_port_value(&priv->audio_cap, settings.byInputSelect);

    if (settings.bFreeRun) {
        audio_capture_set_free_run_sample_rate(&priv->audio_cap, priv->ref_clk_freq,
                settings.dwSampleRate);
    }

    audio_capture_start(
                &priv->audio_cap,
                settings.audioInputMode,
                settings.bFreeRun,
                settings.bLeftAlign,
                settings.bMSBFirst,
                settings.byMSBIndex,
                priv->m_b16ChannelAudio ? 0xFF : 0x0F,
                priv->m_b16ChannelAudio ? MWCAP_AUDIO_SAMPLES_PER_FRAME / 2 : MWCAP_AUDIO_SAMPLES_PER_FRAME,
                0
                );
}

static void _StopAudioCapture(struct xi_driver_private *priv)
{
    audio_capture_stop(&priv->audio_cap);
    xi_pcie_ring_dma_controller_disable(&priv->aud_dma_ctrl);
    xi_pcie_ring_dma_controller_clear_interrupt(&priv->aud_dma_ctrl);
    os_atomic_set(&priv->audio_completed_block_id, -1);
}

static void _ResetAudioCapture(struct xi_driver_private *priv)
{
    os_mutex_lock(priv->audio_ref_lock);
    if (priv->audio_ref_count > 0) {
        _StopAudioCapture(priv);
        _StartAudioCapture(priv);
    }
    os_mutex_unlock(priv->audio_ref_lock);
}

static void _GetBlackColorValue(
        unsigned short *r_v,
        unsigned short *g_y,
        unsigned short *b_u,
        MWCAP_VIDEO_COLOR_FORMAT colorFormat,
        MWCAP_VIDEO_QUANTIZATION_RANGE quantRange,
        SHORT sRVBUScale,
        SHORT sRVBUOffset
        )
{
    if (r_v == NULL || g_y == NULL || b_u == NULL)
        return;

    switch (colorFormat) {
    case MWCAP_VIDEO_COLOR_FORMAT_RGB:
        switch (quantRange) {
        case MWCAP_VIDEO_QUANTIZATION_LIMITED:
            *r_v = 64;
            *g_y = 64;
            *b_u = 64;
            break;

        default:
            *r_v = 0;
            *g_y = 0;
            *b_u = 0;
            break;
        }
        break;

    default:
        switch (quantRange) {
        case MWCAP_VIDEO_QUANTIZATION_LIMITED:
            *r_v = 512;
            *g_y = 64;
            *b_u = 512;
            break;

        default:
            *r_v = 512;
            *g_y = 0;
            *b_u = 512;
            break;
        }
        break;
    }

    *r_v = (*r_v - sRVBUOffset) / sRVBUScale;
    *b_u = (*b_u - sRVBUOffset) / sRVBUScale;

    // 16bit values
    *r_v <<= 6;
    *g_y <<= 6;
    *b_u <<= 6;
}

static void _EnableStripeInterrupts(struct xi_driver_private *priv, int iChannel)
{
    os_spin_lock(priv->video_strip_enable_lock);

    priv->video_strip_last_enable_time[iChannel] = os_get_clock_kick();

    if ((priv->video_cap_enabled_int[iChannel] & VPP_INT_MASK_VFS_FULL_STRIPE_DONE) != 0)
        goto out;

    xi_debug(20, "EnableStripeInterrupts\n");

    priv->video_cap_enabled_int[iChannel] |= (VPP_INT_MASK_VFS_FULL_STRIPE_DONE | VPP_INT_MASK_VFS_QUARTER_STRIPE_DONE);
    video_capture_SetIntEnables(&priv->video_cap[iChannel], priv->video_cap_enabled_int[iChannel]);

out:
    os_spin_unlock(priv->video_strip_enable_lock);
}

static void _UpdateStripeInterrupts(struct xi_driver_private *priv, int iChannel)
{
    os_spin_lock(priv->video_strip_enable_lock);

    if ((priv->video_cap_enabled_int[iChannel] & VPP_INT_MASK_VFS_FULL_STRIPE_DONE) != 0 &&
            os_clock_kick_to_msecs(os_get_clock_kick() - priv->video_strip_last_enable_time[iChannel]) >= 1000) {
        xi_debug(20, "DisableStripeInterrupts\n");

        priv->video_cap_enabled_int[iChannel] &= ~(VPP_INT_MASK_VFS_FULL_STRIPE_DONE | VPP_INT_MASK_VFS_QUARTER_STRIPE_DONE);
        video_capture_SetIntEnables(&priv->video_cap[iChannel], priv->video_cap_enabled_int[iChannel]);

    }

    os_spin_unlock(priv->video_strip_enable_lock);
}


// Firmware related
static BOOLEAN _GetFirmwareStorageInfo(struct xi_driver *pobj,
    DWORD *pcbStorage,
    DWORD *pcbEraseBlock,
    DWORD *pcbProgramBlock,
    DWORD *pcbHeaderOffset)
{
    struct xi_driver_private *priv = pobj->priv;
    DWORD cbStorage;

    if (!qspiflash_micron_GetStorageInfo(&priv->spi_flash, &cbStorage, pcbEraseBlock, pcbProgramBlock)
            || cbStorage <= MAX_FIRMWARE_HEADER_SIZE)
        return FALSE;

    if (cbStorage > priv->m_cbFirmwareStorageLimit)
        cbStorage = priv->m_cbFirmwareStorageLimit;

    if (pcbStorage)
        *pcbStorage = cbStorage;
    if (pcbHeaderOffset)
        *pcbHeaderOffset = (cbStorage - MAX_FIRMWARE_HEADER_SIZE);

    return TRUE;
}

static BOOLEAN IsValidFirmwareHeader(const MW_FIRMWARE_INFO_HEADER * pFWInfoHdr, const MWCAP_CHANNEL_INFO * pChannelInfo)
{
    DWORD cbSectionHeaders = pFWInfoHdr->cSections * sizeof(MW_FIRMWARE_SECTION_HEADER);

    return (pFWInfoHdr->dwMagic == MW_FIRMWARE_HEADER_MAGIC &&
            pFWInfoHdr->wProductID == pChannelInfo->wProductID &&
            pFWInfoHdr->byFirmwareID == pChannelInfo->byFirmwareID &&
            pFWInfoHdr->cbHeader == (cbSectionHeaders + sizeof(MW_FIRMWARE_INFO_HEADER)) &&
            pChannelInfo->dwFirmwareVersion == pFWInfoHdr->dwFirmwareVersion);
}

/* Magewell Capture Extensions */
int xi_driver_get_channel_info(struct xi_driver *pobj, int iChannel, MWCAP_CHANNEL_INFO *pInfo)
{
    struct xi_driver_private *priv = pobj->priv;
    MW_FIRMWARE_INFO_HEADER firmwareHeader = { 0 };
    DWORD cbHeaderOffset;

    pInfo->wFamilyID = MW_FAMILY_ID_PRO_CAPTURE;
    pInfo->wProductID = priv->product_id;
    pInfo->chHardwareVersion = 'A' + (char)(priv->hardware_ver >> 16) - 1;
    pInfo->byFirmwareID = xi_device_get_firmware_id(&priv->dev_info);;
    pInfo->dwFirmwareVersion = xi_device_get_firmware_ver(&priv->dev_info);
    pInfo->dwDriverVersion = (VER_MAJOR << 24) | (VER_MINOR << 16) | VER_BUILD;

    switch (pInfo->wProductID) {
    case MWCAP_PRODUCT_ID_PRO_CAPTURE_EZ100A:
        os_strlcpy(pInfo->szFamilyName, "EZ Capture", MW_FAMILY_NAME_LEN);
        break;

    default:
        os_strlcpy(pInfo->szFamilyName, "Pro Capture", MW_FAMILY_NAME_LEN);
        break;
    }

    os_strlcpy(pInfo->szProductName, priv->product_name, MW_PRODUCT_NAME_LEN);
    os_strlcpy(pInfo->szFirmwareName, "Factory Default", MW_FIRMWARE_NAME_LEN);

    if (_GetFirmwareStorageInfo(pobj, NULL, NULL, NULL, &cbHeaderOffset)) {
        // Read upgraded firmware header
        qspiflash_micron_Read(&priv->spi_flash, cbHeaderOffset + 0x1000,
                              (BYTE *)&firmwareHeader, sizeof(firmwareHeader));

        // Read factory default firmware header if upgrade firmware is not available
        if (!IsValidFirmwareHeader(&firmwareHeader, pInfo))
            qspiflash_micron_Read(&priv->spi_flash, cbHeaderOffset,
                                  (BYTE *)&firmwareHeader, sizeof(firmwareHeader));

        if (IsValidFirmwareHeader(&firmwareHeader, pInfo)) {
            os_strlcpy(pInfo->szFirmwareName, firmwareHeader.szFirmwareName, MW_FIRMWARE_NAME_LEN - 1);
            pInfo->szFirmwareName[MW_FIRMWARE_NAME_LEN - 1] = '\0';
        }
    }

    os_memcpy(pInfo->szBoardSerialNo, priv->szBoardSerialNo, sizeof(pInfo->szBoardSerialNo));

    pInfo->byBoardIndex = priv->board_index;
    pInfo->byChannelIndex = priv->m_bMultiChannelFPGA ? iChannel : priv->channel_index;

    return 0;
}

void xi_driver_get_family_info(struct xi_driver *pobj, MWCAP_PRO_CAPTURE_INFO * pInfo)
{
    struct xi_driver_private *priv = pobj->priv;

    pInfo->byBoardIndex = priv->board_index;
    pInfo->byPCIBusID = priv->pci_bus_id;
    pInfo->byPCIDevID = priv->pci_dev_id;
    pInfo->byLinkType = (xi_device_get_pcie_link_status(&priv->dev_info) >> 8) & 0xF;
    pInfo->byLinkWidth = xi_device_get_pcie_link_status(&priv->dev_info) & 0x3F;
    pInfo->wMaxPayloadSize = (xi_device_get_pcie_payload_size(&priv->dev_info) >> 16) * 4;
    pInfo->wMaxReadRequestSize = (xi_device_get_pcie_payload_size(&priv->dev_info) & 0x7FF) * 4;
    pInfo->cbTotalMemorySize = xi_device_get_memory_size(&priv->dev_info);
    pInfo->cbFreeMemorySize = xi_heap_get_free_size(&priv->board_heap);
}

void xi_driver_get_video_caps(struct xi_driver *pobj, MWCAP_VIDEO_CAPS *pInfo)
{
    struct xi_driver_private *priv = pobj->priv;

    pInfo->dwCaps = 0;
    pInfo->wMaxInputWidth = priv->m_cxMaxInput;
    pInfo->wMaxInputHeight = priv->m_cyMaxInput;
    pInfo->wMaxOutputWidth = priv->m_cxMaxOutput;
    pInfo->wMaxOutputHeight = priv->m_cyMaxOutput;
}

void xi_driver_get_audio_caps(struct xi_driver *pobj, MWCAP_AUDIO_CAPS *pInfo)
{
    pInfo->dwCaps = 0;
}

int xi_driver_get_firmware_storage_info(struct xi_driver *pobj,
        MWCAP_FIRMWARE_STORAGE *pStorage)
{
    if (!_GetFirmwareStorageInfo(pobj, &pStorage->cbStorage, &pStorage->cbEraseBlock,
                &pStorage->cbProgramBlock, &pStorage->cbHeaderOffset))
        return -1;

    return 0;
}

int xi_driver_set_firmware_erase(struct xi_driver *pobj, MWCAP_FIRMWARE_ERASE *pErase)
{
    struct xi_driver_private *priv = pobj->priv;
    DWORD cbStorage;

    if (!qspiflash_micron_GetStorageInfo(&priv->spi_flash, &cbStorage, NULL, NULL))
        return -1;

    if (pErase->cbOffset >= cbStorage)
        return OS_RETURN_INVAL;

    qspiflash_micron_EraseRange(&priv->spi_flash, pErase->cbOffset,
            min(pErase->cbErase, cbStorage - pErase->cbOffset));

    return 0;
}

int xi_driver_firmware_data_write(struct xi_driver *pobj, u32 offset, u8 *data, u32 count)
{
    struct xi_driver_private *priv = pobj->priv;
    DWORD cbStorage;
    DWORD cbData;

    if (!qspiflash_micron_GetStorageInfo(&priv->spi_flash, &cbStorage, NULL, NULL))
        return -1;

    if (offset >= cbStorage)
        return OS_RETURN_INVAL;

    cbData = min(count, cbStorage - offset);

    qspiflash_micron_Write(&priv->spi_flash, offset, data, cbData);

    return cbData;
}

int xi_driver_firmware_data_read(struct xi_driver *pobj, u32 offset, u8 *data, u32 count)
{
    struct xi_driver_private *priv = pobj->priv;
    DWORD cbStorage;
    DWORD cbData;

    if (!qspiflash_micron_GetStorageInfo(&priv->spi_flash, &cbStorage, NULL, NULL))
        return -1;

    if (offset >= cbStorage)
        return OS_RETURN_INVAL;

    cbData = min(count, cbStorage - offset);

    qspiflash_micron_Read(&priv->spi_flash, offset, data, cbData);

    return cbData;
}

void xi_driver_set_led_mode(struct xi_driver *pobj, unsigned int mode)
{
    struct xi_driver_private *priv = pobj->priv;

    xi_device_set_led_mode(&priv->dev_info, mode);
}

void xi_driver_set_post_reconfig_delay(struct xi_driver *pobj, u32 delay)
{
    struct xi_driver_private *priv = pobj->priv;

    priv->post_reconfig_delay = delay;
}

void xi_driver_get_video_frame_info(struct xi_driver *pobj,
                                    int iChannel,
                                    int iframe,
                                    MWCAP_VIDEO_FRAME_INFO *info)
{
    struct xi_driver_private *priv = pobj->priv;

    video_capture_GetFrameInfo(&priv->video_cap[iChannel], iframe, info);
}

void xi_driver_get_video_buffer_info(struct xi_driver *pobj,
                                     int iChannel,
                                     MWCAP_VIDEO_BUFFER_INFO *info)
{
    struct xi_driver_private *priv = pobj->priv;

    video_capture_GetBufferInfo(&priv->video_cap[iChannel], info);
}

int xi_driver_get_sdianc_packets(struct xi_driver *pobj,
                                 int iChannel,
                                 int iFrame,
                                 int iCount,
                                 MWCAP_SDI_ANC_PACKET *pPackets)
{
    struct xi_driver_private *priv = pobj->priv;

    return video_capture_GetSDIANCPackets(&priv->video_cap[iChannel], iFrame, iCount, pPackets);
}

long xi_driver_get_core_temperature(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_device_get_temperature(&priv->dev_info);
}

int xi_driver_set_edid(struct xi_driver *pobj, u8 *data, int size)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_thread_request_request(&priv->house_thread_request,
            REQUEST_SET_EDID, data, (void *)(long)size, NULL);
}

const u8 *xi_driver_get_edid(struct xi_driver *pobj, int *size)
{
    struct xi_driver_private *priv = pobj->priv;
    int edid_size = parameter_GetEDIDSize(&priv->parameter_manager);

    if (size != NULL)
        *size = edid_size;

    return parameter_GetEDID(&priv->parameter_manager);
}

u32 xi_driver_get_hdmi_infoframe_valid_flags(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_front_end_GetInfoFrameValidFlags(priv->front_end[0]);
}

int xi_driver_get_hdmi_infoframe_packet(struct xi_driver *pobj,
        MWCAP_HDMI_INFOFRAME_ID id, HDMI_INFOFRAME_PACKET *pInfoFramePacket)
{
    struct xi_driver_private *priv = pobj->priv;

    if (!xi_front_end_GetInfoFramePacket(priv->front_end[0], id, pInfoFramePacket))
        return OS_RETURN_NODEV;

    return 0;
}

void xi_driver_set_video_input_aspect_ratio(struct xi_driver *pobj,
        MWCAP_VIDEO_ASPECT_RATIO *pAspectRatio)
{
    struct xi_driver_private *priv = pobj->priv;
    MWCAP_VIDEO_ASPECT_RATIO ratio;

    parameter_SetVideoInputAspectRatio(&priv->parameter_manager,
            pAspectRatio->nAspectX, pAspectRatio->nAspectY);

    parameter_GetVideoInputAspectRatio(&priv->parameter_manager,
        &ratio.nAspectX, &ratio.nAspectY);

    mw_netlink_notify_user_front_end(priv->uio_dev, 0, NETLINK_CMD_FRONT_END_SYNC_VIDEO_RATIO,
        MW_NETLINK_TYPE_SET, &ratio, NULL, sizeof(ratio), 0, 0);
}

bool xi_driver_get_video_input_aspect_ratio(struct xi_driver *pobj,
        MWCAP_VIDEO_ASPECT_RATIO *pAspectRatio)
{
    struct xi_driver_private *priv = pobj->priv;

    return parameter_GetVideoInputAspectRatio(&priv->parameter_manager,
            &pAspectRatio->nAspectX, &pAspectRatio->nAspectY);
}

void xi_driver_set_video_input_color_format(struct xi_driver *pobj,
        MWCAP_VIDEO_COLOR_FORMAT colorFormat)
{
    struct xi_driver_private *priv = pobj->priv;

    parameter_SetVideoInputColorFormat(&priv->parameter_manager, colorFormat);

    colorFormat = parameter_GetVideoInputColorFormat(&priv->parameter_manager);
    xi_debug(5, "colorFormat:%d\n", colorFormat);
    mw_netlink_notify_user_front_end(priv->uio_dev, 0, NETLINK_CMD_FRONT_END_SYNC_VIDEO_FORMAT,
        MW_NETLINK_TYPE_SET, &colorFormat, NULL, sizeof(colorFormat), 0, 0);
}

MWCAP_VIDEO_COLOR_FORMAT
xi_driver_get_video_input_color_format(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return parameter_GetVideoInputColorFormat(&priv->parameter_manager);
}

void xi_driver_set_video_input_quantization_range(struct xi_driver *pobj,
        MWCAP_VIDEO_QUANTIZATION_RANGE range)
{
    struct xi_driver_private *priv = pobj->priv;
    MWCAP_VIDEO_QUANTIZATION_RANGE quant_range;

    parameter_SetVideoInputQuantizationRange(&priv->parameter_manager, range);

    quant_range = parameter_GetVideoInputQuantizationRange(&priv->parameter_manager);
    mw_netlink_notify_user_front_end(priv->uio_dev, 0, NETLINK_CMD_FRONT_END_SYNC_VIDEO_QUANT_RANGE,
        MW_NETLINK_TYPE_SET, &quant_range, NULL, sizeof(quant_range), 0, 0);
}

MWCAP_VIDEO_QUANTIZATION_RANGE
xi_driver_get_video_input_quantization_range(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return parameter_GetVideoInputQuantizationRange(&priv->parameter_manager);
}


bool xi_driver_get_video_auto_h_align(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return parameter_GetAutoHAlign(&priv->parameter_manager);
}

int xi_driver_set_video_auto_h_align(struct xi_driver *pobj, bool enable)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_thread_request_request(&priv->house_thread_request,
            REQUEST_SET_AUTO_H_ALIGN, (void *)enable, NULL, NULL);
}

u8 xi_driver_get_video_sampling_phase(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return parameter_GetSamplingPhase(&priv->parameter_manager);
}

int xi_driver_set_video_sampling_phase(struct xi_driver *pobj, u8 sampling_phase)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_thread_request_request(&priv->house_thread_request,
            REQUEST_SET_SAMPLING_PHASE, (void *)&sampling_phase, NULL, NULL);
}

bool xi_driver_get_video_sampling_phase_auto(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return parameter_GetAutoSamplingPhaseAdjust(&priv->parameter_manager);
}

int xi_driver_set_video_sampling_phase_auto(struct xi_driver *pobj, bool enable)
{
    struct xi_driver_private *priv = pobj->priv;

    parameter_SetAutoSamplingPhaseAdjust(&priv->parameter_manager, enable);

    return 0;
}

int xi_driver_set_video_timing(struct xi_driver *pobj, const MWCAP_VIDEO_TIMING *timing)
{
    struct xi_driver_private *priv = pobj->priv;

    xi_debug(5, "enter\n");

    return mw_netlink_notify_user_front_end(priv->uio_dev, 0, NETLINK_CMD_SET_CURRENT_VIDEO_TIMING, MW_NETLINK_TYPE_SET, (uint8_t *)timing, NULL, sizeof(*timing), 0, 100);
}

int xi_driver_set_video_custom_timing(struct xi_driver *pobj, const MWCAP_VIDEO_CUSTOM_TIMING *timing)
{
    struct xi_driver_private *priv = pobj->priv;

    xi_debug(5, "enter\n");

    return mw_netlink_notify_user_front_end(priv->uio_dev, 0, NETLINK_CMD_SET_CUSTOM_VIDEO_TIMING, MW_NETLINK_TYPE_SET, (uint8_t *)timing, NULL, sizeof(*timing), 0, 100);
}

/* @ret; <0 error; >=0 timing count */
int xi_driver_get_video_preferred_timing_array(struct xi_driver *pobj, MWCAP_VIDEO_TIMING *timing)
{
    struct xi_driver_private *priv = pobj->priv;
    os_ptrdiff_t count = 0;
    int ret = -1;

    ret = mw_netlink_notify_user_front_end(priv->uio_dev, 0, NETLINK_CMD_FRONT_END_GET_PREFERRED_VIDEO_TIMINGS, MW_NETLINK_TYPE_GET, (uint8_t *)timing, &count, sizeof(*timing), sizeof(count), 100);
    if (ret < 0)
        return ret;

    return (int)count;
}

int xi_driver_get_video_custom_timing_count(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return parameter_GetCustomVideoTimingCount(&priv->parameter_manager);
}

const MWCAP_VIDEO_CUSTOM_TIMING * xi_driver_get_video_custom_timing_array(
        struct xi_driver *pobj, int *count)
{
    struct xi_driver_private *priv = pobj->priv;

    if (count != NULL)
        *count = parameter_GetCustomVideoTimingCount(&priv->parameter_manager);

    return parameter_GetCustomVideoTimings(&priv->parameter_manager);
}

int xi_driver_set_video_custom_timing_array(struct xi_driver *pobj,
            MWCAP_VIDEO_CUSTOM_TIMING *timings, int count)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_thread_request_request(&priv->house_thread_request,
            REQUEST_SET_VIDEO_CUSTOM_TIMING_ARRAY, (void *)timings, (void *)(long)count, NULL);
}

int xi_driver_get_video_custom_resolution_count(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return parameter_GetVideoResolutionCount(&priv->parameter_manager);
}

const MWCAP_SIZE * xi_driver_get_video_custom_resolution_array(
        struct xi_driver *pobj, int *count)
{
    struct xi_driver_private *priv = pobj->priv;

    if (count != NULL)
        *count = parameter_GetVideoResolutionCount(&priv->parameter_manager);

    return parameter_GetVideoResolutions(&priv->parameter_manager);
}

int xi_driver_set_video_custom_resolution_array(struct xi_driver *pobj,
            MWCAP_SIZE *resolutions, int count)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_thread_request_request(&priv->house_thread_request,
            REQUEST_SET_VIDEO_CUSTOM_RESOLUTION_ARRAY, (void *)resolutions, (void *)(long)count, NULL);
}


int xi_driver_set_hdcp_support(struct xi_driver *pobj, bool enable)
{
    struct xi_driver_private *priv = pobj->priv;

    return xi_thread_request_request(&priv->house_thread_request,
            REQUEST_SET_ENABLE_HDCP, (void *)enable, NULL, NULL);
}

bool xi_driver_get_hdcp_support(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return parameter_GetEnableHDCP(&priv->parameter_manager);
}

short xi_driver_i2c_read_regs(
        struct xi_driver *pobj,
        int ch, uint8_t devaddr, uint8_t regaddr, uint8_t *data, short size)
{
    struct xi_driver_private *priv = pobj->priv;
    short _size;
    int ret;

    _size = min(256U - regaddr, (unsigned int)size);

    ret = xi_i2c_master_read_regs(
                &priv->i2c, ch, devaddr, regaddr, data, _size);
    if (ret != 0) {
        _size = 0;
    }

    return _size;
}

short xi_driver_i2c_write_regs(
        struct xi_driver *pobj,
        int ch, uint8_t devaddr, uint8_t regaddr, uint8_t *data, short size)
{
    struct xi_driver_private *priv = pobj->priv;
    short _size;
    int ret;

    _size = min(256U - regaddr, (unsigned int)size);

    ret = xi_i2c_master_write_regs(
                &priv->i2c, ch, devaddr, regaddr, data, _size);
    if (ret != 0) {
        _size = 0;
    }


    return _size;
}

WORD xi_driver_gspi_read_reg(struct xi_driver *pobj,
                              MWCAP_DEVICE_GSPI_S *gpi_s)
{
    struct xi_driver_private *priv = pobj->priv;
    WORD wRet;

    xi_spi_set_chip_select(&priv->gspi, gpi_s->dwChipSelects);
    // Wait 1.5ns
    xi_spi_write(&priv->gspi, gpi_s->wRegAddr | 0x8000);
    // Wait 148.4ns - T
    wRet = (WORD)xi_spi_read(&priv->gspi);
    // Wait 37.1ns
    xi_spi_clear_chip_select(&priv->gspi, gpi_s->dwChipSelects);

    return wRet;
}

void xi_driver_gspi_write_reg(struct xi_driver *pobj,
                              MWCAP_DEVICE_GSPI_S *gpi_s)
{
    struct xi_driver_private *priv = pobj->priv;

    xi_spi_set_chip_select(&priv->gspi, gpi_s->dwChipSelects);
    // Wait 1.5ns
    xi_spi_write(&priv->gspi, gpi_s->wRegAddr);
    // Wait 37.1ns - T
    xi_spi_write(&priv->gspi, gpi_s->wData);
    // Wait 37.1ns
    xi_spi_clear_chip_select(&priv->gspi, gpi_s->dwChipSelects);
}

void xi_driver_set_sdianc_type(struct xi_driver *pobj,
                               MWCAP_SDI_ANC_TYPE *pANCType)
{
    struct xi_driver_private *priv = pobj->priv;

    sdianc_SetANCType(&priv->m_sdiANC, pANCType->byId, pANCType->bHANC,
                       pANCType->bVANC, pANCType->byDID, pANCType->bySDID);
}

void xi_driver_get_sdianc_packet(struct xi_driver *pobj,
                                 MWCAP_SDI_ANC_PACKET *pPacket)
{
    struct xi_driver_private *priv = pobj->priv;
    DWORD *pdwANCPacket = (DWORD *)pPacket;
    int nReadId;

    if (sdianc_GetStatus(&priv->m_sdiANC, NULL, NULL, NULL, &nReadId, NULL)) {
        DWORD dwData;
        int cbUDW;
        int cdwData;

        sdianc_SetAddress(&priv->m_sdiANC, nReadId);

        dwData = sdianc_ReadPacketData(&priv->m_sdiANC);
        *pdwANCPacket++ = dwData;

        cbUDW = pPacket->byDC;
        cdwData = (cbUDW - 1 + 3) / 4;
        while (cdwData--)
            *pdwANCPacket++ = sdianc_ReadPacketData(&priv->m_sdiANC);

        sdianc_PopPacket(&priv->m_sdiANC);
    }
    else {
        pPacket->byDID = 0;
        pPacket->bySDID = 0;
        pPacket->byDC = 0;
    }
}

BOOLEAN xi_driver_GetEnableSystemSDIANC(struct xi_driver *pobj)
{
    struct xi_driver_private *priv = pobj->priv;

    return !!priv->m_bEnableSystemSDIANC;
}

void xi_driver_SetEnableSystemSDIANC(struct xi_driver *pobj, BOOLEAN bEnable)
{
    struct xi_driver_private *priv = pobj->priv;

    priv->m_bEnableSystemSDIANC = !!bEnable;
}

int xi_driver_SetQuadSDIForceConvMode2SI(struct xi_driver *pobj, BOOLEAN bEnable)
{
    struct xi_driver_private *priv = pobj->priv;

    parameter_SetQuadSDIForceConvMode2SI(&priv->parameter_manager, bEnable);

    return 0;
}

static bool _AddBasicAudioToEDID(BYTE * pbyEDID, int *cbEDID)
{
    PEDID_BASE_BLOCK pBase = (PEDID_BASE_BLOCK)pbyEDID;
    PEDID_CEA_EXT_BLOCK_HEADER pExtBlock = (PEDID_CEA_EXT_BLOCK_HEADER)(pBase + 1);
    PEDID_CEA_DATA_BLOCK_HEADER pHdr;

    if (cbEDID == NULL)
        return false;

    if (os_memcmp(g_abyEDIDHeader, pBase->abyHeader, sizeof(g_abyEDIDHeader)) != 0
        || !EDID_VerifyBlock(pbyEDID)
        || pBase->byVersion != 1
        || pBase->byRevision < 3
        || pBase->byExtensionBlockCount > 1)
        return false;

    if (pBase->byExtensionBlockCount == 0) {
        pBase->byExtensionBlockCount = 1;
        os_memset(pExtBlock, 0, 128);
        pExtBlock->byBlockTagNumber = EDID_BLOCK_TAG_CEA_EXT;
        pExtBlock->byRevisionNumber = 3;
        pExtBlock->byDetailedTimingDescOffset = 4;
        pExtBlock->byNumNativeDTDs = 1;
        pExtBlock->byFlags = 0;
        *cbEDID = 256;
    }
    else if (!EDID_VerifyBlock((BYTE *)pExtBlock)
             || pExtBlock->byBlockTagNumber != EDID_BLOCK_TAG_CEA_EXT
             || pExtBlock->byRevisionNumber != 3)
        return false;

    pHdr = EDID_FindVSDB(pExtBlock, g_abyHDMI_VSDB);
    if (NULL == pHdr) {
        EDID_HDMI_VSDB_HEADER hdmiVSDBHdr;
        hdmiVSDBHdr.byTagCode = EDID_CEA_BLOCK_TAG_VENDOR_SPECIFIC;
        hdmiVSDBHdr.byDataLength = 5;
        hdmiVSDBHdr.byAB = 0x00;
        hdmiVSDBHdr.byCD = 0x00;
        os_memcpy(hdmiVSDBHdr.abyIEEE_OUI, g_abyHDMI_VSDB, sizeof(g_abyHDMI_VSDB));
        if (!EDID_InsertCEADataBlock(pExtBlock, NULL, (PEDID_CEA_DATA_BLOCK_HEADER)&hdmiVSDBHdr))
            return false;
    }

    pExtBlock->byFlags |= EDID_CEA_FLAG_BASIC_AUDIO;

    pbyEDID[127] = EDID_CalcCheckSum(pbyEDID);
    pbyEDID[255] = EDID_CalcCheckSum((BYTE *)pExtBlock);
    return true;
}

static bool _LimitPixelClockToEDID(BYTE * pbyEDID, int cbEDID)
{
    PEDID_BASE_BLOCK pBase;
    PEDID_CEA_EXT_BLOCK_HEADER pExtBlock;
    PEDID_CEA_DATA_BLOCK_HEADER pHdr;
    PEDID_DISPLAY_RANGE_LIMITS_DESC pRangeLimit;
    PEDID_DETAILED_TIMING_DESC pTimings;
    int cDTDs;
    int i;

    if (cbEDID < 256)
        return true;

    pBase = (PEDID_BASE_BLOCK)pbyEDID;
    pExtBlock = (PEDID_CEA_EXT_BLOCK_HEADER)(pBase + 1);

    pHdr = EDID_FindVSDB(pExtBlock, g_abyHDMI_FORUM);
    if (NULL == pHdr)
        return true;

    EDID_RemoveCEADataBlock(pExtBlock, pHdr);

    pRangeLimit = FindDisplayRangeLimitDesc(pBase);
    if (NULL != pRangeLimit
        && pRangeLimit->byMaxPixelClockFreq > 30) {
        pRangeLimit->byMaxPixelClockFreq = 30;
    }

    pTimings = (PEDID_DETAILED_TIMING_DESC)pBase->abyPreferredTimingMode;
    for (i = 0; i < 4; i++) {
        if (pTimings->wPixelClockFreq > 30000)
            pTimings->wPixelClockFreq >>= 1;

        pTimings++;
    }

    pTimings = (PEDID_DETAILED_TIMING_DESC)((BYTE *)pExtBlock + pExtBlock->byDetailedTimingDescOffset);
    cDTDs = EDID_GetCEADetailedTimingCount(pExtBlock);
    for (i = 0; i < cDTDs; i++) {
        if (pTimings->wPixelClockFreq > 30000)
            pTimings->wPixelClockFreq >>= 1;

        pTimings++;
    }

    pbyEDID[127] = EDID_CalcCheckSum(pbyEDID);
    pbyEDID[255] = EDID_CalcCheckSum((BYTE *)pExtBlock);
    return true;
}

static void _UpdateEDID(struct xi_driver_private *priv)
{
    int cbEDID;
    BYTE abyEDID[256];

    BYTE byEDIDMode = parameter_GetEDIDMode(&priv->parameter_manager);

    xi_debug(5, "byEDIDMode:%d, edid[255]:%x\n", byEDIDMode, priv->parameter_manager.m_abyEDID[255]);
    // byEDIDMode = MWCAP_EDID_MODE_FIXED;

    if (byEDIDMode & MWCAP_EDID_MODE_FIXED) {
        xi_debug(20, "Set default EDID\r\n");
        xi_front_end_SetEDID(priv->front_end[0],
                    parameter_GetEDID(&priv->parameter_manager),
                    parameter_GetEDIDSize(&priv->parameter_manager));
    }
    else if (xi_front_end_GetLoopThroughEDID(priv->front_end[0], abyEDID, &cbEDID)) {
        if (byEDIDMode & MWCAP_EDID_MODE_ADD_AUDIO)
            _AddBasicAudioToEDID(abyEDID, &cbEDID);
        if (byEDIDMode & MWCAP_EDID_MODE_LIMIT_PIXEL_CLOCK)
            _LimitPixelClockToEDID(abyEDID, cbEDID);

        xi_debug(20, "Set loop through EDID\r\n");
        xi_front_end_SetEDID(priv->front_end[0], abyEDID, cbEDID);
    }
    else if ((byEDIDMode & MWCAP_EDID_MODE_KEEP_LAST) == 0) {
        xi_debug(5, "Set default EDID\r\n");
        xi_front_end_SetEDID(priv->front_end[0],
                    parameter_GetEDID(&priv->parameter_manager),
                    parameter_GetEDIDSize(&priv->parameter_manager));
    }
}

static void _AdjustFanDuty(struct xi_driver_private *priv)
{
    BYTE byDuty = 0;

    if (!priv->m_bHasPWMFan)
        return;

    byDuty = parameter_GetFanDuty(&priv->parameter_manager);
    if (byDuty == 0) {
        LONG Temper = xi_device_get_temperature(&priv->dev_info);
        if (Temper >= 850)
            byDuty = 100;
        else if (Temper >= 800)
            byDuty = 90;
        else if (Temper >= 750)
            byDuty = 80;
        else if (Temper >= 700)
            byDuty = 70;
        else if (Temper >= 650)
            byDuty = 65;
        else if (Temper >= 600)
            byDuty = 60;
        else if (Temper >= 550)
            byDuty = 55;
        else if (Temper >= 500)
            byDuty = 50;
        else if (Temper >= 450)
            byDuty = 45;
        else
            byDuty = 40;
    }

    pwm_fan_ctrl_SetDuty(&priv->m_pwmFanCtrl, byDuty);
}

struct xi_front_end **xi_driver_get_front_end(struct xi_driver *pobj)
{
    struct xi_driver_private *priv;

    if (!pobj)
        return NULL;

    priv = pobj->priv;
    if (!priv)
        return NULL;

    return priv->front_end;
}

struct uio_device *xi_driver_get_uio_dev(struct xi_driver *pobj)
{
    struct xi_driver_private *priv;

    if (!pobj)
        return NULL;

    priv = pobj->priv;
    if (!priv)
        return NULL;

    return priv->uio_dev;
}

void xi_driver_set_crypto_serial(struct xi_driver *pobj, CHAR *szBoardSerialNo)
{
    struct xi_driver_private *priv;

    if (!pobj)
        return;

    priv = pobj->priv;
    if (!priv)
        return;

    os_memcpy(priv->szBoardSerialNo, szBoardSerialNo, sizeof(priv->szBoardSerialNo));
}

int xi_driver_get_device_info(struct xi_driver *pobj, struct mw_board_info *info)
{
    struct xi_driver_private *priv;

    if (!pobj)
        return -EINVAL;

    priv = pobj->priv;
    if (!priv)
        return -EINVAL;

    info->front_end_type = priv->front_end_type;
	info->m_cChannlesPerDevice = priv->m_cChannlesPerDevice;
	info->ref_clk_freq = priv->ref_clk_freq;
	info->m_cxMaxInput = priv->m_cxMaxInput;
	info->m_cxMaxOutput = priv->m_cxMaxOutput;
	info->m_cyMaxInput = priv->m_cyMaxInput;
	info->m_cyMaxOutput = priv->m_cyMaxOutput;
	info->m_bEnableSystemSDIANC = priv->m_bEnableSystemSDIANC;
	info->m_bHasLoopthrough = priv->m_bHasLoopthrough;
	info->has_gspi = priv->has_gspi;
    info->m_nPixelsPerCycle = priv->m_nPixelsPerCycle;
    info->hardware_ver = priv->hardware_ver;

    return 0;
}

int xi_driver_set_enable_irq(struct xi_driver *pobj, FRONT_END_INTERRUPT_SOURCE intSource)
{
    struct xi_driver_private *priv;

    if (!pobj)
        return -EINVAL;

    priv = pobj->priv;
    if (!priv)
        return -EINVAL;

    switch (intSource) {
    case FRONT_END_INTERRUPT_HDMI_PHY_CLKDET:
        xi_irq_set_enable_bits(&priv->irq_ctrl, IRQ_MASK_HDMI_PHY_CLKDET);
        break;
    case FRONT_END_INTERRUPT_HDMI_RX:
        xi_irq_set_enable_bits(&priv->irq_ctrl, IRQ_MASK_HDMI_RX);
        break;
    case FRONT_END_INTERRUPT_GPIO:
        xi_irq_set_enable_bits(&priv->irq_ctrl, IRQ_MASK_GPIO);
        break;
    case FRONT_END_INTERRUPT_SYNC_METER:
        xi_irq_set_enable_bits(&priv->irq_ctrl, IRQ_MASK_SYNC_METER);
        break;
    case FRONT_END_INTERRUPT_SDI_INPUT:
        xi_irq_set_enable_bits(&priv->irq_ctrl, IRQ_MASK_SDI_INPUT);
        break;
    case FRONT_END_INTERRUPT_VIDEO_ACTIVITY_DETECTOR:
        xi_irq_set_enable_bits(&priv->irq_ctrl, IRQ_MASK_VAD);
        break;
    case FRONT_END_INTERRUPT_HDMI_PHY_GPIO:
        xi_irq_set_enable_bits(&priv->irq_ctrl, IRQ_MASK_HDMI_PHY_GPIO);
        break;
    case FRONT_END_INTERRUPT_SDI_ANC:
        xi_irq_set_enable_bits(&priv->irq_ctrl, IRQ_MASK_ANC1);
        break;
    default:
        break;
    }

    return 0;
}

int xi_driver_set_disable_irq(struct xi_driver *pobj, FRONT_END_INTERRUPT_SOURCE intSource)
{
    struct xi_driver_private *priv;

    if (!pobj)
        return -EINVAL;

    priv = pobj->priv;
    if (!priv)
        return -EINVAL;

    switch (intSource) {
    case FRONT_END_INTERRUPT_HDMI_PHY_CLKDET:
        xi_irq_clear_enable_bits(&priv->irq_ctrl, IRQ_MASK_HDMI_PHY_CLKDET);
        break;
    case FRONT_END_INTERRUPT_HDMI_RX:
        xi_irq_clear_enable_bits(&priv->irq_ctrl, IRQ_MASK_HDMI_RX);
        break;
    case FRONT_END_INTERRUPT_GPIO:
        xi_irq_clear_enable_bits(&priv->irq_ctrl, IRQ_MASK_GPIO);
        break;
    case FRONT_END_INTERRUPT_SYNC_METER:
        xi_irq_clear_enable_bits(&priv->irq_ctrl, IRQ_MASK_SYNC_METER);
        break;
    case FRONT_END_INTERRUPT_SDI_INPUT:
        xi_irq_clear_enable_bits(&priv->irq_ctrl, IRQ_MASK_SDI_INPUT);
        break;
    case FRONT_END_INTERRUPT_VIDEO_ACTIVITY_DETECTOR:
        xi_irq_clear_enable_bits(&priv->irq_ctrl, IRQ_MASK_VAD);
        break;
    case FRONT_END_INTERRUPT_HDMI_PHY_GPIO:
        xi_irq_clear_enable_bits(&priv->irq_ctrl, IRQ_MASK_HDMI_PHY_GPIO);
        break;
    case FRONT_END_INTERRUPT_SDI_ANC:
        xi_irq_clear_enable_bits(&priv->irq_ctrl, IRQ_MASK_ANC1);
        break;
    default:
        break;
    }

    return 0;
    
}
