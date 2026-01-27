////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#ifndef __FRONT_END_H__
#define __FRONT_END_H__

#include "ospi/ospi.h"
#include "mw-procapture-extension.h"

#include "supports/xi-timer.h"
#include "avstream/parameter-manager.h"
#include "supports/sw-timer-event-manager.h"
#include "supports/notify-event-manager.h"

#include "front-end/front-end-types.h"
#include "mw-fe-uapi.h"

#define MAX_INPUT_SOURCE (10)

// Combined signal status
typedef struct _SIGNAL_STATUS {
	INPUT_SPECIFIC_STATUS			inputSpecificStatus;
	VIDEO_SIGNAL_STATUS				videoSignalStatus;
	AUDIO_SIGNAL_STATUS				audioSignalStatus;
} SIGNAL_STATUS;

// Video settings
typedef struct _VIDEO_CAPTURE_SETTINGS {
	BOOLEAN							bValid;
	BYTE							byInputSelect;
	int								x;
	int								y;
	int								cx;
	int								cy;
	int								nAspectX;
	int								nAspectY;
	BOOLEAN							bInterlaced;
	BOOLEAN							bSegmentedFrame;
	BOOLEAN							bFramePaking;
	int								yRightFrame;
	BOOLEAN							bInterlacedFramePacking;
	int								cyInterlacedFramePacking;

	MWCAP_VIDEO_COLOR_FORMAT		colorFormat;
	MWCAP_VIDEO_QUANTIZATION_RANGE	quantRange;
	SHORT							sRVBUScale;
	SHORT							sRVBUOffset;
} VIDEO_CAPTURE_SETTINGS;

// Audio settings
typedef struct _AUDIO_CAPTURE_SETTINGS {
	BOOLEAN							bFreeRun;
	BYTE							byInputSelect;
	AUDIO_INPUT_MODE				audioInputMode;
	BOOLEAN							bLeftAlign;
	BOOLEAN							bMSBFirst;
	BYTE							byMSBIndex;
	WORD							wChannelValid;
	DWORD							dwSampleRate;
} AUDIO_CAPTURE_SETTINGS;

struct xi_front_end;

struct xi_front_end_notify_sink {

	struct xi_front_end *owner;

	void (*OnNotify)(struct xi_front_end *owner, ULONGLONG ullNotifyBits,
		struct xi_front_end *caller);
};

struct xi_front_end {
	/*Device*/
	int                                 chn_index;
	void                               *driver;
	xi_notify_event_manager            *m_pNotifyEventManager;
	xi_sw_timer_event_manager          *m_pSWTimerEventManager;
	struct parameter_t                 *m_pParameterManager;
	int                                 m_iChannel;
	struct uio_device	               *uio_dev;
	/*Limit*/
	int                                 m_cxMax;
	int                                 m_cyMax;
	/*Lock*/
	os_spinlock_t                       m_lockStatus;
	/*Signal Stauts*/
	INPUT_SPECIFIC_STATUS               m_inputSpecificStatus;
	VIDEO_SIGNAL_STATUS                 m_videoSignalStatus;
	AUDIO_SIGNAL_STATUS                 m_audioSignalStatus;
	VIDEO_CAPTURE_SETTINGS              m_videoCaptureSettings;
	AUDIO_CAPTURE_SETTINGS              m_audioCaptureSettings;

	/*Private information*/
	struct front_end_input_source       inputSource;
	int                                 volume[16];
};

void xi_front_end_SetInputSourceScan(struct xi_front_end *front, BOOLEAN bEnableScan);

void xi_front_end_SetAVInputSourceLink(struct xi_front_end *front, BOOLEAN bLink);

void xi_front_end_SetVideoInputSource(struct xi_front_end *front, DWORD dwInputSource);

DWORD xi_front_end_GetVideoInputSource(struct xi_front_end *front);

void xi_front_end_SetAudioInputSource(struct xi_front_end *front, DWORD dwInputSource);

DWORD xi_front_end_GetAudioInputSource(struct xi_front_end *front);

BOOLEAN xi_front_end_IsSupportVideoInputSource(struct xi_front_end *front, DWORD dwInputSource);

BOOLEAN xi_front_end_IsSupportAudioInputSource(struct xi_front_end *front, DWORD dwInputSource);

const DWORD *xi_front_end_GetSupportedVideoInputSources(struct xi_front_end *front, int *pcInputSources);

const DWORD *xi_front_end_GetSupportedAudioInputSources(struct xi_front_end *front, int *pcInputSources);

// Analog audio related
// Unit: dB * 0x10000, eg 1dB = 0x10000
void xi_front_end_SetVolume(struct xi_front_end *front, int iChannel, int nVolume);
int xi_front_end_GetVolume(struct xi_front_end *front, int iChannel);

// HDMI related
void xi_front_end_SetEDID(struct xi_front_end *front, const BYTE *pbyEDID, WORD cbEDID);

void xi_front_end_SetEnableHDCP(struct xi_front_end *front, BOOLEAN bEnable);

DWORD xi_front_end_GetInfoFrameValidFlags(struct xi_front_end *front);

BOOLEAN xi_front_end_GetInfoFramePacket(struct xi_front_end *front,
	MWCAP_HDMI_INFOFRAME_ID infoFrameId, HDMI_INFOFRAME_PACKET *pPacket);

BOOLEAN xi_front_end_GetLoopThroughEDID(struct xi_front_end *front,
					 BYTE *pbyEDID, int *cbEDID);

BOOLEAN xi_front_end_GetLoopThroughConnected(struct xi_front_end *front);

// Component/VGA related
void xi_front_end_TriggerSamplingPhaseAdjust(struct xi_front_end *front);

void xi_front_end_SetSamplingPhase(struct xi_front_end *front, BYTE bySamplingPhase);

BOOLEAN xi_front_end_SetCurrentVideoTiming(struct xi_front_end *front,
	const MWCAP_VIDEO_TIMING *pTiming);

BOOLEAN xi_front_end_SetCustomVideoTiming(struct xi_front_end *front,
	const MWCAP_VIDEO_CUSTOM_TIMING *pTiming);

// Event handlers
void xi_front_end_InterruptISRHandler(struct xi_front_end *front, FRONT_END_INTERRUPT_SOURCE interruptSource);

void xi_front_end_InterruptDPCHandler(struct xi_front_end *front, FRONT_END_INTERRUPT_SOURCE interruptSource);

BOOLEAN xi_front_end_TimerExpireHandler(struct xi_front_end *front, xi_timer *pTimerEvent);

void xi_front_end_SignalActivityHandler(struct xi_front_end *front, BOOL bSignalActive);


// Signal status
void xi_front_end_GetInputSpecificStatus(struct xi_front_end *front, INPUT_SPECIFIC_STATUS *pStatus);

void xi_front_end_GetVideoSignalStatus(struct xi_front_end *front, VIDEO_SIGNAL_STATUS *pStatus);

void xi_front_end_GetAudioSignalStatus(struct xi_front_end *front, AUDIO_SIGNAL_STATUS *pStatus);

void xi_front_end_GetVideoCaptureSettings(struct xi_front_end *front, VIDEO_CAPTURE_SETTINGS *pSettings);

void xi_front_end_GetAudioCaptureSettings(struct xi_front_end *front, AUDIO_CAPTURE_SETTINGS *pSettings);

BOOLEAN xi_front_end_IsVideoCaptureSettingsChanged(struct xi_front_end *front,
	VIDEO_CAPTURE_SETTINGS *pSettings);

BOOLEAN xi_front_end_IsAudioCaptureSettingsChanged(struct xi_front_end *front,
	AUDIO_CAPTURE_SETTINGS *pSettings);

void xi_front_end_Destroy(struct xi_front_end *front);

// SDI related
void xi_front_end_GetSDILinkStatus(struct xi_front_end *front, MWCAP_SDI_SINGLE_LINK_STATUS *pLinkStatus);
int xi_front_end_GetSDIQuadLinkManualMode(struct xi_front_end *front);
int xi_front_end_SetSDIQuadLinkManualMode(struct xi_front_end *front, int nMode);

BOOLEAN xi_front_end_IsScanning(struct xi_front_end *front);
void xi_front_end_Notify(struct xi_front_end *front, ULONGLONG ullNotifyBits);
int xi_front_end_init(struct xi_front_end *front, void *driver,
	struct xi_front_end_notify_sink *notify_sink, int cxMax, int cyMax);
void xi_front_end_deinit(struct xi_front_end *front);
#endif /* __FRONT_END_H__ */

