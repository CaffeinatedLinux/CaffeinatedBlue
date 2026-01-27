////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#ifndef __FRONT_END_UTILS_H__
#define __FRONT_END_UTILS_H__

#include "mw-procapture-extension.h"

BOOLEAN IsDifferentSyncInfo(
        const MWCAP_VIDEO_SYNC_INFO * pInfo1,
        const MWCAP_VIDEO_SYNC_INFO * pInfo2,
        DWORD dwFrameDurationChangeThreshold
        );

BOOLEAN IsDifferentVideoTiming(
        const MWCAP_VIDEO_TIMING * pTiming1,
        const MWCAP_VIDEO_TIMING * pTiming2
        );

BOOLEAN IsDifferentVideoTimingSettings(
        const MWCAP_VIDEO_TIMING_SETTINGS * pSettings1,
        const MWCAP_VIDEO_TIMING_SETTINGS * pSettings2
        );

BOOLEAN IsDifferentInputSpecificStatus(
        const INPUT_SPECIFIC_STATUS * pStatus1,
        const INPUT_SPECIFIC_STATUS * pStatus2,
        DWORD dwFrameDurationChangeThreshold
        );

BOOLEAN IsDifferentHDMIVideoTiming(
    const MWCAP_HDMI_VIDEO_TIMING * pTiming1,
    const MWCAP_HDMI_VIDEO_TIMING * pTiming2,
    DWORD dwFrameDurationChangeThreshold
    );

BOOLEAN IsDifferentVideoSignalStatus(
        const VIDEO_SIGNAL_STATUS * pStatus1,
        const VIDEO_SIGNAL_STATUS * pStatus2,
        DWORD dwFrameDurationChangeThreshold
        );

BOOLEAN IsDifferentAudioSignalStatus(
        const AUDIO_SIGNAL_STATUS * pStatus1,
        const AUDIO_SIGNAL_STATUS * pStatus2
        );

BOOLEAN IsDifferentVideoCaptureSettings(
        const VIDEO_CAPTURE_SETTINGS * pSettings1,
        const VIDEO_CAPTURE_SETTINGS * pSettings2
        );

BOOLEAN IsDifferentAudioCaptureSettings(
        const AUDIO_CAPTURE_SETTINGS * pSettings1,
        const AUDIO_CAPTURE_SETTINGS * pSettings2
        );

void InitFreeRunAudioSignalStatus(
        AUDIO_SIGNAL_STATUS * pStatus
        );

void InitFreeRunAudioCaptureSettings(
        AUDIO_CAPTURE_SETTINGS * pSettings
        );

BOOLEAN IsSupportInputSource(
        DWORD dwInputSource,
        const DWORD * pdwInputSources,
        int cInputSources
        );

#endif /*__FRONT_END_UTILS_H__ */

