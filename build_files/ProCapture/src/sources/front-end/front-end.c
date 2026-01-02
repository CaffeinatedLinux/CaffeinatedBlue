////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#include "ospi/ospi.h"

#include "front-end/front-end.h"
#include "front-end-utils.h"

#include "avstream/xi-driver-priv.h"
#include "avstream/mw-netlink.h"

static BOOLEAN base_IsSupportVideoInputSource(struct xi_front_end *front, DWORD dwInputSource);
static BOOLEAN base_IsSupportAudioInputSource(struct xi_front_end *front, DWORD dwInputSource);

static BOOLEAN base_IsVideoCaptureSettingsChanged(struct xi_front_end *front,
		VIDEO_CAPTURE_SETTINGS *pSettings);
static BOOLEAN base_IsAudioCaptureSettingsChanged(struct xi_front_end *front,
		AUDIO_CAPTURE_SETTINGS *pSettings);


// Input source selection
void xi_front_end_SetInputSourceScan(struct xi_front_end *front, BOOLEAN bEnableScan)
{
	mw_netlink_notify_user_front_end(front->uio_dev, front->chn_index, NETLINK_CMD_SET_INPUT_SOURCE_SCAN,
		MW_NETLINK_TYPE_SET, &bEnableScan, NULL, sizeof(bEnableScan), 0, 0);
}

void xi_front_end_SetAVInputSourceLink(struct xi_front_end *front, BOOLEAN bLink)
{
	/*notify user*/
	mw_netlink_notify_user_front_end(front->uio_dev, front->chn_index, NETLINK_CMD_SET_AV_INPUT_SOURCE_LINK,
		MW_NETLINK_TYPE_SET, &bLink, NULL, sizeof(bLink), 0, 0);
}

void xi_front_end_SetVideoInputSource(struct xi_front_end *front, DWORD dwInputSource)
{
	if (base_IsSupportVideoInputSource(front, dwInputSource))
		mw_netlink_notify_user_front_end(front->uio_dev, front->chn_index, NETLINK_CMD_SET_VIDEO_INPUT_SOURCE,
			MW_NETLINK_TYPE_SET, &dwInputSource, NULL, sizeof(dwInputSource), 0, 0);
}

DWORD xi_front_end_GetVideoInputSource(struct xi_front_end *front)
{
	return front->inputSource.videoInputSource;
}

void xi_front_end_SetAudioInputSource(struct xi_front_end *front, DWORD dwInputSource)
{
	mw_netlink_notify_user_front_end(front->uio_dev, front->chn_index, NETLINK_CMD_SET_AUDIO_INPUT_SOURCE,
			MW_NETLINK_TYPE_SET, &dwInputSource, NULL, sizeof(dwInputSource), 0, 0);
}

DWORD xi_front_end_GetAudioInputSource(struct xi_front_end *front)
{
	return front->inputSource.audioInputSource;
}

BOOLEAN xi_front_end_IsSupportVideoInputSource(struct xi_front_end *front, DWORD dwInputSource)
{
	return base_IsSupportVideoInputSource(front, dwInputSource);
}

BOOLEAN xi_front_end_IsSupportAudioInputSource(struct xi_front_end *front, DWORD dwInputSource)
{
	return base_IsSupportAudioInputSource(front, dwInputSource);
}

const DWORD *xi_front_end_GetSupportedVideoInputSources(struct xi_front_end *front, int *pcInputSources)
{
	*pcInputSources = front->inputSource.numVideoInputSources;

	return front->inputSource.videoSupportInputSources;
}

const DWORD *xi_front_end_GetSupportedAudioInputSources(struct xi_front_end *front, int *pcInputSources)
{
	*pcInputSources = front->inputSource.numAudioInputSources;

	return front->inputSource.audioSupportInputSources;
}

// Analog audio related
void xi_front_end_SetVolume(struct xi_front_end *front, int iChannel, int nVolume)
{
	mw_netlink_notify_user_front_end(front->uio_dev, front->chn_index, NETLINK_CMD_SET_VOLUME,
			MW_NETLINK_TYPE_SET, &iChannel, &nVolume, sizeof(iChannel), sizeof(nVolume), 0);
}

int xi_front_end_GetVolume(struct xi_front_end *front, int iChannel)
{
	return front->volume[iChannel];
}

// HDMI related
void xi_front_end_SetEDID(struct xi_front_end *front, const BYTE *pbyEDID, WORD cbEDID)
{
	mw_netlink_notify_user_front_end(front->uio_dev, front->chn_index, NETLINK_CMD_SET_EDID,
			MW_NETLINK_TYPE_SET, pbyEDID, &cbEDID, cbEDID, sizeof(cbEDID), 0);
}

void xi_front_end_SetEnableHDCP(struct xi_front_end *front, BOOLEAN bEnable)
{
	mw_netlink_notify_user_front_end(front->uio_dev, front->chn_index, NETLINK_CMD_SET_ENABLE_HDCP,
		MW_NETLINK_TYPE_SET, &bEnable, NULL, sizeof(bEnable), 0, 0);
}

DWORD xi_front_end_GetInfoFrameValidFlags(struct xi_front_end *front)
{
	DWORD flags;

	mw_netlink_notify_user_front_end(front->uio_dev, front->chn_index, NETLINK_CMD_FRONT_END_GET_INFO_FRAME_VALID_FLAGS,
		MW_NETLINK_TYPE_GET, &flags, NULL, sizeof(flags), 0, 500);

	return flags;
}

BOOLEAN xi_front_end_GetInfoFramePacket(struct xi_front_end *front,
		MWCAP_HDMI_INFOFRAME_ID infoFrameId, HDMI_INFOFRAME_PACKET *pPacket)
{
	int ret;

	ret = mw_netlink_notify_user_front_end(front->uio_dev, front->chn_index, NETLINK_CMD_FRONT_END_GET_INFO_FRAME_PACKET,
		MW_NETLINK_TYPE_GET, &infoFrameId, pPacket, sizeof(infoFrameId), sizeof(*pPacket), 500);

	if (ret == 0)
		return TRUE;

	return FALSE;
}

BOOLEAN xi_front_end_GetLoopThroughEDID(struct xi_front_end *front,
										 BYTE *pbyEDID, int *cbEDID)
{
	int ret;

	ret = mw_netlink_notify_user_front_end(front->uio_dev, front->chn_index, NETLINK_CMD_FRONT_END_GET_LOOP_THROUGH_EDID,
		MW_NETLINK_TYPE_GET, pbyEDID, cbEDID, 256, sizeof(*cbEDID), 500);

	if (ret < 0)
		return FALSE;

	return TRUE;
}

BOOLEAN xi_front_end_GetLoopThroughConnected(struct xi_front_end *front)
{
	return FALSE;
}

// Component/VGA related
void xi_front_end_TriggerSamplingPhaseAdjust(struct xi_front_end *front)
{
	mw_netlink_notify_user_front_end(front->uio_dev, front->chn_index, NETLINK_CMD_TRIGGER_SAMPLING_PHASE_ADJUST,
			MW_NETLINK_TYPE_SET, NULL, NULL, 0, 0, 0);
}

void xi_front_end_SetSamplingPhase(struct xi_front_end *front, BYTE bySamplingPhase)
{
	mw_netlink_notify_user_front_end(front->uio_dev, front->chn_index, NETLINK_CMD_SET_SAMPLING_PHASE,
		MW_NETLINK_TYPE_SET, &bySamplingPhase, NULL, sizeof(bySamplingPhase), 0, 0);
}

BOOLEAN xi_front_end_SetCurrentVideoTiming(struct xi_front_end *front,
		const MWCAP_VIDEO_TIMING *pTiming)
{
	return mw_netlink_notify_user_front_end(front->uio_dev, front->chn_index, NETLINK_CMD_SET_CURRENT_VIDEO_TIMING,
		MW_NETLINK_TYPE_SET, pTiming, NULL, sizeof(*pTiming), 0, 0);
}

BOOLEAN xi_front_end_SetCustomVideoTiming(struct xi_front_end *front,
		const MWCAP_VIDEO_CUSTOM_TIMING *pTiming)
{
	return mw_netlink_notify_user_front_end(front->uio_dev, front->chn_index, NETLINK_CMD_SET_CUSTOM_VIDEO_TIMING,
		MW_NETLINK_TYPE_SET, pTiming, NULL, sizeof(*pTiming), 0, 0);
}

// Event handlers
void xi_front_end_InterruptISRHandler(struct xi_front_end *front, FRONT_END_INTERRUPT_SOURCE interruptSource)
{
	xi_debug(10, "interruptSource:%d\n", interruptSource);
	mw_netlink_notify_user_front_end(front->uio_dev, front->chn_index, NETLINK_CMD_INTERRUPT_ISR_HANDLE,
		MW_NETLINK_TYPE_SET, &interruptSource, NULL, sizeof(interruptSource), 0, 0);
}

void xi_front_end_InterruptDPCHandler(struct xi_front_end *front, FRONT_END_INTERRUPT_SOURCE interruptSource)
{
	mw_netlink_notify_user_front_end(front->uio_dev, front->chn_index, NETLINK_CMD_INTERRUP_DPC_HANDLE,
		MW_NETLINK_TYPE_SET, &interruptSource, NULL, sizeof(interruptSource), 0, 0);
}

BOOLEAN xi_front_end_TimerExpireHandler(struct xi_front_end *front, xi_timer *pTimerEvent)
{
	BOOLEAN ret = FALSE;

	return ret;
}

// Signal status
void xi_front_end_GetInputSpecificStatus(struct xi_front_end *front, INPUT_SPECIFIC_STATUS *pStatus)
{
	os_spin_lock_bh(front->m_lockStatus);
	os_memcpy(pStatus, &front->m_inputSpecificStatus, sizeof(*pStatus));
	os_spin_unlock_bh(front->m_lockStatus);
}

void xi_front_end_GetVideoSignalStatus(struct xi_front_end *front, VIDEO_SIGNAL_STATUS *pStatus)
{
	// Since the video signal status is frequently accessed in the VPP thread,
	// it is recorded locally in the kernel.
	os_spin_lock_bh(front->m_lockStatus);
	os_memcpy(pStatus, &front->m_videoSignalStatus, sizeof(*pStatus));
	os_spin_unlock_bh(front->m_lockStatus);
}

void xi_front_end_GetAudioSignalStatus(struct xi_front_end *front, AUDIO_SIGNAL_STATUS *pStatus)
{
	// Since the audio signal status is frequently accessed in the alsa thread,
	// it is recorded locally in the kernel.
	os_spin_lock_bh(front->m_lockStatus);
	os_memcpy(pStatus, &front->m_audioSignalStatus, sizeof(*pStatus));
	os_spin_unlock_bh(front->m_lockStatus);
}

void xi_front_end_GetVideoCaptureSettings(struct xi_front_end *front,
										  VIDEO_CAPTURE_SETTINGS *pSettings)
{
	os_spin_lock_bh(front->m_lockStatus);
	os_memcpy(pSettings, &front->m_videoCaptureSettings, sizeof(*pSettings));
	os_spin_unlock_bh(front->m_lockStatus);
}

void xi_front_end_GetAudioCaptureSettings(struct xi_front_end *front, AUDIO_CAPTURE_SETTINGS *pSettings)
{
	os_spin_lock_bh(front->m_lockStatus);
	os_memcpy(pSettings, &front->m_audioCaptureSettings, sizeof(*pSettings));
	os_spin_unlock_bh(front->m_lockStatus);
}

BOOLEAN xi_front_end_IsVideoCaptureSettingsChanged(struct xi_front_end *front,
		VIDEO_CAPTURE_SETTINGS *pSettings)
{
	return base_IsVideoCaptureSettingsChanged(front, pSettings);
}

BOOLEAN xi_front_end_IsAudioCaptureSettingsChanged(struct xi_front_end *front,
		AUDIO_CAPTURE_SETTINGS *pSettings)
{
	return base_IsAudioCaptureSettingsChanged(front, pSettings);
}

void xi_front_end_Destroy(struct xi_front_end *front)
{
	mw_netlink_notify_user_front_end(front->uio_dev, front->chn_index, NETLINK_CMD_FRONT_END_DESTROY,
		MW_NETLINK_TYPE_SET, NULL, NULL, 0, 0, 0);
	xi_front_end_deinit(front);
	os_free(front);
}

/* functions for sub devices*/
int xi_front_end_init(struct xi_front_end *front, void *driver,
		struct xi_front_end_notify_sink *notify_sink, int cxMax, int cyMax)
{
	front->m_cxMax = cxMax;
	front->m_cyMax = cyMax;
	front->driver = driver;
	front->m_pNotifyEventManager    = xi_driver_get_notify_event_manager(driver, front->m_iChannel);
	front->m_pSWTimerEventManager   = xi_driver_get_sw_timer_event_manager(driver);
	front->m_pParameterManager      = xi_driver_get_parameter_manager(driver);
	front->uio_dev                  = xi_driver_get_uio_dev(driver);
	front->m_lockStatus = os_spin_lock_alloc();
	if (front->m_lockStatus == NULL)
		return OS_RETURN_NOMEM;

	InitFreeRunAudioSignalStatus(&front->m_audioSignalStatus);
	InitFreeRunAudioCaptureSettings(&front->m_audioCaptureSettings);

	return 0;
}

void xi_front_end_deinit(struct xi_front_end *front)
{
	if (front->m_lockStatus != NULL) {
		os_spin_lock_free(front->m_lockStatus);
		front->m_lockStatus = NULL;
	}

	front->driver = NULL;
	front->m_pNotifyEventManager = NULL;
	front->m_pSWTimerEventManager = NULL;
	front->m_pParameterManager = NULL;
}

BOOLEAN xi_front_end_IsScanning(struct xi_front_end *front)
{
	BOOLEAN m_bScanning;

	mw_netlink_notify_user_front_end(front->uio_dev,
		front->chn_index,
		NETLINK_CMD_FRONT_END_GET_SCAN_STATE,
		MW_NETLINK_TYPE_GET,
		&m_bScanning,
		NULL,
		sizeof(m_bScanning),
		0,
		500);

	return m_bScanning;
}

void xi_front_end_Notify(struct xi_front_end *front, ULONGLONG ullNotifyBits)
{
	if (front->m_pNotifyEventManager)
		xi_notify_event_manager_notify(front->m_pNotifyEventManager, ullNotifyBits);
}

static BOOLEAN base_IsSupportVideoInputSource(struct xi_front_end *front, DWORD dwInputSource)
{
	int cInputSources;
	const DWORD *pdwInputSources =
		xi_front_end_GetSupportedVideoInputSources(front, &cInputSources);
	return IsSupportInputSource(dwInputSource, pdwInputSources, cInputSources);
}

static BOOLEAN base_IsSupportAudioInputSource(struct xi_front_end *front, DWORD dwInputSource)
{
	int cInputSources;
	const DWORD *pdwInputSources =
		xi_front_end_GetSupportedAudioInputSources(front, &cInputSources);
	return IsSupportInputSource(dwInputSource, pdwInputSources, cInputSources);
}

// Signal status

static BOOLEAN base_IsVideoCaptureSettingsChanged(struct xi_front_end *front,
		VIDEO_CAPTURE_SETTINGS *pSettings)
{
	return IsDifferentVideoCaptureSettings(pSettings, &front->m_videoCaptureSettings);
}

static BOOLEAN base_IsAudioCaptureSettingsChanged(struct xi_front_end *front,
		AUDIO_CAPTURE_SETTINGS *pSettings)
{
	return IsDifferentAudioCaptureSettings(pSettings, &front->m_audioCaptureSettings);
}

// SDI related
void xi_front_end_GetSDILinkStatus(struct xi_front_end *front, MWCAP_SDI_SINGLE_LINK_STATUS *pLinkStatus)
{
	if (pLinkStatus) {
		mw_netlink_notify_user_front_end(front->uio_dev,
		front->chn_index,
		NETLINK_CMD_FRONT_END_GET_SDI_LINK_STATUS,
		MW_NETLINK_TYPE_GET,
		pLinkStatus,
		NULL,
		sizeof(*pLinkStatus),
		0,
		500);
	}
}

int xi_front_end_GetSDIQuadLinkManualMode(struct xi_front_end *front)
{
	int m_quadLinkManualMode;

	if (!front)
		return MWCAP_SDI_QUAD_LINK_MANUAL_MODE_OFF;

	mw_netlink_notify_user_front_end(front->uio_dev,
		front->chn_index,
		NETLINK_CMD_FRONT_END_GET_SDI_QUAD_LINK_MANUAL_MODE,
		MW_NETLINK_TYPE_GET,
		&m_quadLinkManualMode,
		NULL,
		sizeof(m_quadLinkManualMode),
		0,
		500);

	return m_quadLinkManualMode;
}

int xi_front_end_SetSDIQuadLinkManualMode(struct xi_front_end *front, int nMode)
{
	if (nMode > MWCAP_SDI_QUAD_LINK_MANUAL_SQD || nMode < MWCAP_SDI_QUAD_LINK_MANUAL_MODE_OFF)
		return -EINVAL;

	return mw_netlink_notify_user_front_end(front->uio_dev, front->chn_index, NETLINK_CMD_SET_SDI_QUADLINK_MANUAL_MODE,
		MW_NETLINK_TYPE_SET, &nMode, NULL, sizeof(nMode), 0, 0);
}

