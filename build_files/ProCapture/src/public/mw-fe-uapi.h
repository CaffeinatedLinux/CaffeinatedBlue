////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2024 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#ifndef _MW_FE_UAPI_H_
#define _MW_FE_UAPI_H_
#include <linux/ioctl.h>
#include <linux/types.h>
#include "front-end/front-end-types.h"

#if defined(__cplusplus)
extern "C" {
#endif

// Interrupt sources
typedef enum _FRONT_END_INTERRUPT_SOURCE {
	FRONT_END_INTERRUPT_SDI_LOCK_CHANGED,
	FRONT_END_INTERRUPT_SDI_INPUT,
	FRONT_END_INTERRUPT_ADV_RECEIVER,
	FRONT_END_INTERRUPT_SYNC_METER,
	FRONT_END_INTERRUPT_VIDEO_ACTIVITY_DETECTOR,
	FRONT_END_INTERRUPT_HDMI_PHY_GPIO,
	FRONT_END_INTERRUPT_HDMI_PHY_CLKDET,
	FRONT_END_INTERRUPT_HDMI_RX,
	FRONT_END_INTERRUPT_HDMI_TX_HPD,
	FRONT_END_INTERRUPT_SDI_ANC,
	FRONT_END_INTERRUPT_GPIO
} FRONT_END_INTERRUPT_SOURCE;

/**
 * List of supported front-end types
 */
enum MW_FRONT_END_TYPE {
	MW_FRONT_END_AIO,
	MW_FRONT_END_DVI,
	MW_FRONT_END_HDMI,
	MW_FRONT_END_DUAL_DVI,
	MW_FRONT_END_GV7601,
	MW_FRONT_END_ADV761X,
	MW_FRONT_END_ADV7842,
	MW_FRONT_END_XILINX_3G_SDI,
	MW_FRONT_END_XILINX_HDMI,
	MW_FRONT_END_AIO_4K_PLUS,
	MW_FRONT_END_DUAL_ADV761X,
	MW_FRONT_END_AIO_4K,
	MW_FRONT_END_QUAD_LINK_SDI,
	MW_FRONT_END_AIO_LITE,
	MW_FRONT_END_DP,
	MW_FRONT_END_ADV7180
};

enum NETLINK_MSG_CMD {
	NETLINK_CMD_SET_INPUT_SOURCE_SCAN = 0x01,
	NETLINK_CMD_SET_AV_INPUT_SOURCE_LINK,
	NETLINK_CMD_SET_VIDEO_INPUT_SOURCE,
	NETLINK_CMD_SET_AUDIO_INPUT_SOURCE,
	NETLINK_CMD_SET_VOLUME,
	NETLINK_CMD_SET_EDID,
	NETLINK_CMD_SET_ENABLE_HDCP,
	NETLINK_CMD_TRIGGER_SAMPLING_PHASE_ADJUST,
	NETLINK_CMD_SET_SAMPLING_PHASE,
	NETLINK_CMD_SET_CURRENT_VIDEO_TIMING,
	NETLINK_CMD_SET_CUSTOM_VIDEO_TIMING,
	NETLINK_CMD_INTERRUPT_ISR_HANDLE,
	NETLINK_CMD_INTERRUP_DPC_HANDLE,
	NETLINK_CMD_TIMER_EXPIRE_HANDLER,
	NETLINK_CMD_HOUSE_KEEPING_HANDLER,
	NETLINK_CMD_SIGNAL_ACTIVITY_HANDLER,
	NETLINK_CMD_CHANGED_INPUT_SOURCE,
	NETLINK_CMD_SET_SDI_QUADLINK_MANUAL_MODE,
	NETLINK_CMD_FRONT_END_DESTROY,
	NETLINK_CMD_FRONT_END_SYNC_VIDEO_RATIO,
	NETLINK_CMD_FRONT_END_SYNC_VIDEO_FORMAT,
	NETLINK_CMD_FRONT_END_SYNC_VIDEO_QUANT_RANGE,

	NETLINK_CMD_FRONT_END_GET_PREFERRED_VIDEO_TIMINGS,
	NETLINK_CMD_FRONT_END_GET_SPECIFIC_STATUS,
	NETLINK_CMD_FRONT_END_GET_VIDEO_SIGNAL_STATUS,
	NETLINK_CMD_FRONT_END_GET_AUDIO_SIGNAL_STATUS,
	NETLINK_CMD_FRONT_END_GET_VIDEO_CAPTURE_SETTINGS,
	NETLINK_CMD_FRONT_END_GET_AUDIO_CAPTURE_SETTINGS,
	NETLINK_CMD_FRONT_END_GET_SDI_LINK_STATUS,
	NETLINK_CMD_FRONT_END_GET_SDI_QUAD_LINK_MANUAL_MODE,
	NETLINK_CMD_FRONT_END_GET_INFO_FRAME_VALID_FLAGS,
	NETLINK_CMD_FRONT_END_GET_INFO_FRAME_PACKET,
	NETLINK_CMD_FRONT_END_GET_VOLUME,
	NETLINK_CMD_FRONT_END_GET_LOOP_THROUGH_EDID,
	NETLINK_CMD_FRONT_END_GET_SCAN_STATE
};

typedef enum _MW_PARAMETER_EDID_TYPE {
	MW_PARAMETER_EDID_2K,
	MW_PARAMETER_EDID_4K,
	MW_PARAMETER_EDID_4Kp,
	MW_PARAMETER_EDID_DVI_4K
} MW_PARAMETER_EDID_TYPE;

typedef enum _MW_CAP_VIDEO_QUANTIZATION_RANGE {
	MW_CAP_VIDEO_QUANTIZATION_UNKNOWN           = 0x00,
	MW_CAP_VIDEO_QUANTIZATION_FULL              = 0x01, // Black level: 0, White level: 255/1023/4095/65535
	MW_CAP_VIDEO_QUANTIZATION_LIMITED           = 0x02 // Black level: 16/64/256/4096, White level: 235(240)/940(960)/3760(3840)/60160(61440)
} MW_CAP_VIDEO_QUANTIZATION_RANGE;

struct mw_karray {
	char            *data;
	int             count;
	int             max_count;
	int             element_size;
	int             alloc_size;
};

struct device_params {
	// void                          *priv;
	MW_PARAMETER_EDID_TYPE          m_nEDIDType;

	// HDCP
	BOOLEAN                         m_bEnableHDCP;

	// EDID
	uint8_t                         m_byEDIDMode;
	uint8_t                         m_abyEDID[256];
	uint16_t                        m_cbEDID;

	// Signal detection
	BOOLEAN                         m_bEnableHPSyncMeter;
	uint32_t                        m_dwFrameDurationChangeThreshold;

	uint32_t                        m_dwVideoSyncSliceLevel;

	// Auto scan signal sources
	BOOLEAN                         m_bInputSourceScan;
	uint32_t                        m_dwScanPeriod;
	uint32_t                        m_dwScanKeepDuration;
	struct mw_karray                m_arrScanInputSources; //DWORD
	BOOLEAN                         m_bAVInputSourceLink;

	uint32_t                        m_dwVideoInputSource;
	uint32_t                        m_dwAudioInputSource;

	// Video timings
	BOOLEAN                         m_bAutoHAlign;

	uint8_t                         m_bySamplingPhase;
	BOOLEAN                         m_bAutoSamplingPhaseAdjust;

	// Video controls
	int                             m_nVideoInputAspectX;
	int                             m_nVideoInputAspectY;
	MWCAP_VIDEO_COLOR_FORMAT        m_colorFormatVideoInput;
	MW_CAP_VIDEO_QUANTIZATION_RANGE m_quantRangeVideoInput;

	// Video activity detection threshold
	uint8_t                         m_byVADThreshold;
	/* MWCAP_VIDEO_CUSTOM_TIMING */
	struct mw_karray                m_arrCustomVideoTimings;
	struct mw_karray                m_arrVideoResolutions;
	BOOLEAN                         m_bQuadSDIForceConvMode2SI;
};

typedef enum _MW_NETLINK_ACK_STATUS {
	MW_NETLINK_ACK_OK = 1,
	MW_NETLINK_ACK_ERROR,
} MW_NETLINK_ACK_STATUS;

typedef enum _MW_NETLINK_ACK_TYPES {
	MW_NETLINK_TYPE_SET = 1,
	MW_NETLINK_TYPE_GET,
} MW_NETLINK_ACK_TYPES;

/**
 * cmd:enum NETLINK_MSG_CMD
 */
struct mw_netlink_msg {
	union {
		uint8_t data[512];
		struct {
			uint8_t cmd;
			uint8_t handle;
			uint8_t front_chn_index;
			uint8_t type;
			bool is_ack;
			uint8_t param1[256];
			uint8_t param2[128];
			uint8_t len1;
			uint8_t len2;
			uint8_t status;
		} ack;
	};
};

struct mw_board_info {
	enum MW_FRONT_END_TYPE front_end_type;
	int                    m_cChannlesPerDevice;

	uint32_t               map_size;
	uint32_t               ref_clk_freq;

	// Limitations
	bool                   has_gspi;
	int                    m_cxMaxInput;
	int                    m_cyMaxInput;
	int                    m_cxMaxOutput;
	int                    m_cyMaxOutput;

	BOOLEAN                m_bEnableSystemSDIANC;
	BOOLEAN                m_bHasLoopthrough;
	int                    m_nPixelsPerCycle;
	uint32_t               hardware_ver;
};

struct mw_fe_signal_status {
	int                   chn;/*front end channel index*/
	unsigned long long    ullNotifyBits;
	INPUT_SPECIFIC_STATUS m_inputSpecificStatus;
	VIDEO_SIGNAL_STATUS   m_videoSignalStatus;
	AUDIO_SIGNAL_STATUS   m_audioSignalStatus;
};

struct mw_i2c_wr_reg_params {
	int      ch;
	uint8_t  devaddr;
	uint8_t  regaddr;
	uint8_t  val[256];
	uint16_t count;
};

struct mw_netlink_info {
	int user_pid;/*user->kernel*/
	int netlink_id;/*kernel->user*/
};

struct crypto_info {
	char serial[16];
};

enum mw_ioctl_num {
	MW_IOCTL_FE_SYNC_SIGNAL_STATUS_NUM,
	MW_IOCTL_GET_DEVICE_INFO_NUM,
	MW_IOCTL_GET_BOARD_PARAMS_NUM,
	MW_IOCTL_I2C_READ_NUM,
	MW_IOCTL_I2C_WRITE_NUM,
	MW_IOCTL_I2C_READ_REG_NUM,
	MW_IOCTL_I2C_WRITE_REG_NUM,
	MW_IOCTL_CREATE_NETLINK_NUM,
	MW_IOCTL_SYNC_CRYPTO_SERIAL_NUM,
	MW_IOCTL_NOTIFY_EVENT_MANAGER_NUM,
	MW_IOCTL_FE_SYNC_INPUT_SOURCE_NUM,
	MW_IOCTL_FE_SYNC_VOLUME_NUM,
	MW_IOCTL_NOTIFY_IRQ_CTL_NUM,
};

/**
 * front end input source information
 */
struct front_end_input_source {
	DWORD                     videoInputSource;
	DWORD                     audioInputSource;
	DWORD                     videoSupportInputSources[10];
	int                       numVideoInputSources;
	DWORD                     audioSupportInputSources[10];
	int                       numAudioInputSources;
	unsigned long long        ullNotifyBits;
};

typedef enum _mw_irq_cmd {
	IRQ_CMD_DISABLE,
	IRQ_CMD_ENABLE
} mw_irq_cmd_t;

struct mw_irq_ctl {
	FRONT_END_INTERRUPT_SOURCE intSource;
	mw_irq_cmd_t cmd;
};

struct front_end_volume_info {
	int chn_index;
	int volume;
};

#define MW_IOCTL_BASE                        'm'
#define MW_IO(nr)                            _IO(MW_IOCTL_BASE, nr)
#define MW_IOR(nr, type)                     _IOR(MW_IOCTL_BASE, nr, type)
#define MW_IOW(nr, type)                     _IOW(MW_IOCTL_BASE, nr, type)
#define MW_IOWR(nr, type)                    _IOWR(MW_IOCTL_BASE, nr, type)

/*DEVICE*/
#define MW_IOCTL_GET_DEVICE_INFO             MW_IOW(MW_IOCTL_GET_DEVICE_INFO_NUM, struct mw_board_info)
#define MW_IOCTL_GET_BOARD_PARAMS            MW_IOR(MW_IOCTL_GET_BOARD_PARAMS_NUM, struct device_params)
#define MW_IOCTL_CREATE_NETLINK              MW_IOWR(MW_IOCTL_CREATE_NETLINK_NUM, struct mw_netlink_info)
#define MW_IOCTL_SYNC_CRYPTO_SERIAL          MW_IOW(MW_IOCTL_SYNC_CRYPTO_SERIAL_NUM, struct crypto_info)
#define MW_IOCTL_NOTIFY_EVENT_MANAGER        MW_IOW(MW_IOCTL_NOTIFY_EVENT_MANAGER_NUM, unsigned long long)
#define MW_IOCTL_NOTIFY_IRQ_CTL              MW_IOW(MW_IOCTL_NOTIFY_IRQ_CTL_NUM, struct mw_irq_ctl)
/*I2C*/
#define MW_IOCTL_I2C_READ                    MW_IOWR(MW_IOCTL_I2C_READ_NUM, struct mw_i2c_wr_reg_params)
#define MW_IOCTL_I2C_WRITE                   MW_IOWR(MW_IOCTL_I2C_WRITE_NUM, struct mw_i2c_wr_reg_params)
#define MW_IOCTL_I2C_READ_REG                MW_IOWR(MW_IOCTL_I2C_READ_REG_NUM, struct mw_i2c_wr_reg_params)
#define MW_IOCTL_I2C_WRITE_REG               MW_IOWR(MW_IOCTL_I2C_WRITE_REG_NUM, struct mw_i2c_wr_reg_params)
/*FRONT END*/
#define MW_IOCTL_FE_SYNC_SIGNAL_STATUS       MW_IOW(MW_IOCTL_FE_SYNC_SIGNAL_STATUS_NUM, struct mw_fe_signal_status)
#define MW_IOCTL_FE_SYNC_INPUT_SOURCE        MW_IOW(MW_IOCTL_FE_SYNC_INPUT_SOURCE_NUM, struct front_end_input_source)
#define MW_IOCTL_FE_SYNC_VOLUME              MW_IOW(MW_IOCTL_FE_SYNC_VOLUME_NUM, struct front_end_volume_info)

#endif
