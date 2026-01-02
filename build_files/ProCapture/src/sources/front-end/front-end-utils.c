////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#include "front-end/front-end.h"
#include "front-end-utils.h"

BOOLEAN IsDifferentSyncInfo(
        const MWCAP_VIDEO_SYNC_INFO * pInfo1,
        const MWCAP_VIDEO_SYNC_INFO * pInfo2,
        DWORD dwFrameDurationChangeThreshold
        )
{
    int nFrameDurationMin;
    int nFrameDurationMax;

    if (pInfo1->bySyncType != pInfo2->bySyncType)
        return TRUE;

    if (pInfo1->bySyncType == 0)
        return FALSE;

    if (pInfo1->bInterlaced != pInfo2->bInterlaced)
        return TRUE;

    if (pInfo1->bySyncType == VIDEO_SYNC_HS_VS) {
        if (pInfo1->bHSPolarity != pInfo2->bHSPolarity
                || pInfo1->bVSPolarity != pInfo2->bVSPolarity)
            return TRUE;
    }

    if (pInfo1->wVSyncLineCount != pInfo2->wVSyncLineCount
            || pInfo1->wFrameLineCount != pInfo2->wFrameLineCount)
        return TRUE;

    nFrameDurationMin = (int)pInfo1->dwFrameDuration - (int)dwFrameDurationChangeThreshold;
    nFrameDurationMax = (int)pInfo1->dwFrameDuration + (int)dwFrameDurationChangeThreshold;
    return ((int)pInfo2->dwFrameDuration < nFrameDurationMin) || ((int)pInfo2->dwFrameDuration > nFrameDurationMax);
}

BOOLEAN IsDifferentVideoTiming(
        const MWCAP_VIDEO_TIMING * pTiming1,
        const MWCAP_VIDEO_TIMING * pTiming2
        )
{
    if (pTiming1->dwType != pTiming2->dwType)
        return TRUE;

    if (pTiming1->dwType == MWCAP_VIDEO_TIMING_NONE)
        return FALSE;

    return (pTiming1->dwPixelClock != pTiming2->dwPixelClock
        || pTiming1->bInterlaced != pTiming2->bInterlaced
        || pTiming1->bySyncType != pTiming2->bySyncType
        || (pTiming1->bySyncType == VIDEO_SYNC_HS_VS && pTiming1->bHSPolarity != pTiming2->bHSPolarity)
        || (pTiming1->bySyncType == VIDEO_SYNC_HS_VS && pTiming1->bVSPolarity != pTiming2->bVSPolarity)
        || pTiming1->wHActive != pTiming2->wHActive
        || pTiming1->wHFrontPorch != pTiming2->wHFrontPorch
        || pTiming1->wHSyncWidth != pTiming2->wHSyncWidth
        || pTiming1->wHBackPorch != pTiming2->wHBackPorch
        || pTiming1->wVActive != pTiming2->wVActive
        || pTiming1->wVFrontPorch != pTiming2->wVFrontPorch
        || pTiming1->wVSyncWidth != pTiming2->wVSyncWidth
        || pTiming1->wVBackPorch != pTiming2->wVBackPorch
        );
}

BOOLEAN IsDifferentVideoTimingSettings(
        const MWCAP_VIDEO_TIMING_SETTINGS * pSettings1,
        const MWCAP_VIDEO_TIMING_SETTINGS * pSettings2
        )
{
    return (pSettings1->wAspectX != pSettings2->wAspectX
        || pSettings1->wAspectY != pSettings2->wAspectY
        || pSettings1->x != pSettings2->x
        || pSettings1->y != pSettings2->y
        || pSettings1->cx != pSettings2->cx
        || pSettings1->cy != pSettings2->cy
        || pSettings1->cxTotal != pSettings2->cxTotal
        || pSettings1->byClampPos != pSettings2->byClampPos
        );
}

BOOLEAN IsDifferentInputSpecificStatus(
        const INPUT_SPECIFIC_STATUS * pStatus1,
        const INPUT_SPECIFIC_STATUS * pStatus2,
        DWORD dwFrameDurationChangeThreshold
        )
{
    if (pStatus1->pubStatus.bValid != pStatus2->pubStatus.bValid)
        return TRUE;

    if (!pStatus1->pubStatus.bValid)
        return FALSE;

    if (pStatus1->pubStatus.dwVideoInputType != pStatus2->pubStatus.dwVideoInputType)
        return TRUE;

    switch (pStatus1->pubStatus.dwVideoInputType) {
    case MWCAP_VIDEO_INPUT_TYPE_HDMI:
        return (pStatus1->pubStatus.hdmiStatus.bHDMIMode != pStatus2->pubStatus.hdmiStatus.bHDMIMode
                || pStatus1->pubStatus.hdmiStatus.bHDCP != pStatus2->pubStatus.hdmiStatus.bHDCP
                || pStatus1->pubStatus.hdmiStatus.byBitDepth != pStatus2->pubStatus.hdmiStatus.byBitDepth
                || pStatus1->pubStatus.hdmiStatus.pixelEncoding != pStatus2->pubStatus.hdmiStatus.pixelEncoding
                || pStatus1->pubStatus.hdmiStatus.byVIC != pStatus2->pubStatus.hdmiStatus.byVIC
                || pStatus1->pubStatus.hdmiStatus.bITContent != pStatus2->pubStatus.hdmiStatus.bITContent
                || pStatus1->pubStatus.hdmiStatus.b3DFormat != pStatus2->pubStatus.hdmiStatus.b3DFormat
                || (pStatus1->pubStatus.hdmiStatus.b3DFormat && (pStatus1->pubStatus.hdmiStatus.by3DStructure != pStatus2->pubStatus.hdmiStatus.by3DStructure))
                || (pStatus1->pubStatus.hdmiStatus.b3DFormat && (pStatus1->pubStatus.hdmiStatus.by3DStructure == HDMI14B_3DS_SIDE_BY_SIDE_HALF)
                    && (pStatus1->pubStatus.hdmiStatus.bySideBySideHalfSubSampling != pStatus2->pubStatus.hdmiStatus.bySideBySideHalfSubSampling))
                || IsDifferentHDMIVideoTiming(&pStatus1->pubStatus.hdmiStatus.videoTiming, &pStatus2->pubStatus.hdmiStatus.videoTiming, dwFrameDurationChangeThreshold)
                );

    case MWCAP_VIDEO_INPUT_TYPE_SDI:
        return (pStatus1->pubStatus.sdiStatus.sdiType != pStatus2->pubStatus.sdiStatus.sdiType
                || pStatus1->pubStatus.sdiStatus.sdiScanningFormat != pStatus2->pubStatus.sdiStatus.sdiScanningFormat
                || pStatus1->pubStatus.sdiStatus.sdiBitDepth != pStatus2->pubStatus.sdiStatus.sdiBitDepth
                || pStatus1->pubStatus.sdiStatus.sdiSamplingStruct != pStatus2->pubStatus.sdiStatus.sdiSamplingStruct
                || pStatus1->pubStatus.sdiStatus.bST352DataValid != pStatus2->pubStatus.sdiStatus.bST352DataValid
                || (pStatus1->pubStatus.sdiStatus.bST352DataValid && pStatus1->pubStatus.sdiStatus.dwST352Data != pStatus2->pubStatus.sdiStatus.dwST352Data)
                );

    case MWCAP_VIDEO_INPUT_TYPE_VGA:
    case MWCAP_VIDEO_INPUT_TYPE_COMPONENT:
        return (pStatus1->pubStatus.vgaComponentStatus.bTriLevelSync != pStatus2->pubStatus.vgaComponentStatus.bTriLevelSync
                || pStatus1->wAdjustedHSyncWidth != pStatus2->wAdjustedHSyncWidth
                || IsDifferentSyncInfo(&pStatus1->pubStatus.vgaComponentStatus.syncInfo, &pStatus2->pubStatus.vgaComponentStatus.syncInfo, dwFrameDurationChangeThreshold)
                || IsDifferentVideoTiming(&pStatus1->pubStatus.vgaComponentStatus.videoTiming, &pStatus2->pubStatus.vgaComponentStatus.videoTiming)
                || IsDifferentVideoTimingSettings(&pStatus1->pubStatus.vgaComponentStatus.videoTimingSettings, &pStatus2->pubStatus.vgaComponentStatus.videoTimingSettings)
                );

    case MWCAP_VIDEO_INPUT_TYPE_CVBS:
    case MWCAP_VIDEO_INPUT_TYPE_YC:
        return (os_memcmp(&pStatus1->pubStatus.cvbsYcStatus, &pStatus2->pubStatus.cvbsYcStatus, sizeof(pStatus1->pubStatus.cvbsYcStatus)) != 0);

    default:
        return FALSE;
    }
}

BOOLEAN IsDifferentHDMIVideoTiming(
    const MWCAP_HDMI_VIDEO_TIMING * pTiming1,
    const MWCAP_HDMI_VIDEO_TIMING * pTiming2,
    DWORD dwFrameDurationChangeThreshold
    )
{
    int nFrameDurationMin;
    int nFrameDurationMax;

    if (pTiming1->bInterlaced != pTiming2->bInterlaced
        || pTiming1->wHSyncWidth != pTiming2->wHSyncWidth
        || pTiming1->wHFrontPorch != pTiming2->wHFrontPorch
        || pTiming1->wHBackPorch != pTiming2->wHBackPorch
        || pTiming1->wHActive != pTiming2->wHActive
        || pTiming1->wHTotalWidth != pTiming2->wHTotalWidth
        || pTiming1->wField0VSyncWidth != pTiming2->wField0VSyncWidth
        || pTiming1->wField0VFrontPorch != pTiming2->wField0VFrontPorch
        || pTiming1->wField0VBackPorch != pTiming2->wField0VBackPorch
        || pTiming1->wField0VActive != pTiming2->wField0VActive
        || pTiming1->wField0VTotalHeight != pTiming2->wField0VTotalHeight
        || (pTiming1->bInterlaced && pTiming1->wField1VSyncWidth != pTiming2->wField1VSyncWidth)
        || (pTiming1->bInterlaced && pTiming1->wField1VFrontPorch != pTiming2->wField1VFrontPorch)
        || (pTiming1->bInterlaced && pTiming1->wField1VBackPorch != pTiming2->wField1VBackPorch)
        || (pTiming1->bInterlaced && pTiming1->wField1VActive != pTiming2->wField1VActive)
        || (pTiming1->bInterlaced && pTiming1->wField1VTotalHeight != pTiming2->wField1VTotalHeight)
        )
        return TRUE;

    nFrameDurationMin = (int)pTiming2->dwFrameDuration - (int)dwFrameDurationChangeThreshold;
    nFrameDurationMax = (int)pTiming2->dwFrameDuration + (int)dwFrameDurationChangeThreshold;
    return ((int)pTiming1->dwFrameDuration < nFrameDurationMin) || ((int)pTiming1->dwFrameDuration > nFrameDurationMax);
}

BOOLEAN IsDifferentVideoSignalStatus(
        const VIDEO_SIGNAL_STATUS * pStatus1,
        const VIDEO_SIGNAL_STATUS * pStatus2,
        DWORD dwFrameDurationChangeThreshold
        )
{
    int nFrameDurationMin;
    int nFrameDurationMax;

    if (pStatus1->pubStatus.state != pStatus2->pubStatus.state)
        return TRUE;

    if (pStatus1->pubStatus.state <= MWCAP_VIDEO_SIGNAL_UNSUPPORTED)
        return FALSE;

    if (pStatus1->pubStatus.x != pStatus2->pubStatus.x
            || pStatus1->pubStatus.y != pStatus2->pubStatus.y
            || pStatus1->pubStatus.cx != pStatus2->pubStatus.cx
            || pStatus1->pubStatus.cy != pStatus2->pubStatus.cy
            || pStatus1->pubStatus.cxTotal != pStatus2->pubStatus.cxTotal
            || pStatus1->pubStatus.cyTotal != pStatus2->pubStatus.cyTotal
            || pStatus1->pubStatus.bInterlaced != pStatus2->pubStatus.bInterlaced
            || pStatus1->pubStatus.colorFormat != pStatus2->pubStatus.colorFormat
            || pStatus1->pubStatus.quantRange != pStatus2->pubStatus.quantRange
            || pStatus1->pubStatus.satRange != pStatus2->pubStatus.satRange
            || pStatus1->pubStatus.nAspectX != pStatus2->pubStatus.nAspectX
            || pStatus1->pubStatus.nAspectY != pStatus2->pubStatus.nAspectY
            || pStatus1->pubStatus.bSegmentedFrame != pStatus2->pubStatus.bSegmentedFrame
            || pStatus1->pubStatus.frameType != pStatus2->pubStatus.frameType
            || pStatus1->byInputSelect != pStatus2->byInputSelect
            || pStatus1->sRVBUScale != pStatus2->sRVBUScale
            || pStatus1->sRVBUOffset != pStatus2->sRVBUOffset
            || pStatus1->nEffectiveAspectX != pStatus2->nEffectiveAspectX
            || pStatus1->nEffectiveAspectY != pStatus2->nEffectiveAspectY
            || pStatus1->nEffectiveColorFormat != pStatus2->nEffectiveColorFormat
            || pStatus1->nEffectiveQuantRange != pStatus2->nEffectiveQuantRange
            || pStatus1->bFramePacking != pStatus2->bFramePacking
            || (pStatus1->bFramePacking && pStatus1->yRightFrame != pStatus2->yRightFrame)
            || pStatus1->bInterlacedFramePacking != pStatus2->bInterlacedFramePacking
            || (pStatus1->bInterlacedFramePacking && pStatus1->cyInterlacedFramePacking != pStatus2->cyInterlacedFramePacking)
            )
        return TRUE;

    nFrameDurationMin = (int)pStatus2->pubStatus.dwFrameDuration - (int)dwFrameDurationChangeThreshold;
    nFrameDurationMax = (int)pStatus2->pubStatus.dwFrameDuration + (int)dwFrameDurationChangeThreshold;
    return ((int)pStatus1->pubStatus.dwFrameDuration < nFrameDurationMin)
        || ((int)pStatus1->pubStatus.dwFrameDuration > nFrameDurationMax);
}

BOOLEAN IsDifferentAudioSignalStatus(
        const AUDIO_SIGNAL_STATUS * pStatus1,
        const AUDIO_SIGNAL_STATUS * pStatus2
        )
{
    if (pStatus1->pubStatus.wChannelValid != pStatus2->pubStatus.wChannelValid)
        return TRUE;

    if (pStatus1->pubStatus.bLPCM != pStatus2->pubStatus.bLPCM
            || pStatus1->pubStatus.dwSampleRate != pStatus2->pubStatus.dwSampleRate
            || pStatus1->pubStatus.cBitsPerSample != pStatus2->pubStatus.cBitsPerSample
            || pStatus1->pubStatus.bChannelStatusValid != pStatus2->pubStatus.bChannelStatusValid
            || pStatus1->audioInputMode != pStatus2->audioInputMode
            || pStatus1->bLeftAlign != pStatus2->bLeftAlign
            || pStatus1->bFreeRun != pStatus2->bFreeRun
            || pStatus1->bMSBFirst != pStatus2->bMSBFirst
            || pStatus1->byMSBIndex != pStatus2->byMSBIndex
            || pStatus1->byInputSelect != pStatus2->byInputSelect
            || pStatus1->bAnalogVolumeControl != pStatus2->bAnalogVolumeControl
            || pStatus1->byChannelOffset != pStatus2->byChannelOffset
       )
        return TRUE;

    if (!pStatus1->pubStatus.bChannelStatusValid)
        return FALSE;

    return (os_memcmp(&pStatus1->pubStatus.channelStatus,
                &pStatus2->pubStatus.channelStatus, sizeof(IEC60958_CHANNEL_STATUS)) != 0);
}

BOOLEAN IsDifferentVideoCaptureSettings(
        const VIDEO_CAPTURE_SETTINGS * pSettings1,
        const VIDEO_CAPTURE_SETTINGS * pSettings2
        )
{
    if (pSettings1->bValid != pSettings2->bValid)
        return TRUE;

    if (!pSettings1->bValid)
        return FALSE;

    return (
            pSettings1->x != pSettings2->x
            || pSettings1->y != pSettings2->y
            || pSettings1->cx != pSettings2->cx
            || pSettings1->cy != pSettings2->cy
            || pSettings1->nAspectX != pSettings2->nAspectX
            || pSettings1->nAspectY != pSettings2->nAspectY
            || pSettings1->bInterlaced != pSettings2->bInterlaced
            || pSettings1->bSegmentedFrame != pSettings2->bSegmentedFrame
            || pSettings1->byInputSelect != pSettings2->byInputSelect
            || pSettings1->bFramePaking != pSettings2->bFramePaking
            || (pSettings1->bFramePaking && pSettings1->yRightFrame != pSettings2->yRightFrame)
            || pSettings1->bInterlacedFramePacking != pSettings2->bInterlacedFramePacking
            || (pSettings1->bInterlacedFramePacking
                && pSettings1->cyInterlacedFramePacking != pSettings2->cyInterlacedFramePacking)
            || pSettings1->colorFormat != pSettings2->colorFormat
            || pSettings1->quantRange != pSettings2->quantRange
            || pSettings1->sRVBUScale != pSettings2->sRVBUScale
            || pSettings1->sRVBUOffset != pSettings2->sRVBUOffset
            );
}

BOOLEAN IsDifferentAudioCaptureSettings(
        const AUDIO_CAPTURE_SETTINGS * pSettings1,
        const AUDIO_CAPTURE_SETTINGS * pSettings2
        )
{
    if (pSettings1->wChannelValid != pSettings2->wChannelValid)
        return TRUE;

    return (
            pSettings1->audioInputMode != pSettings2->audioInputMode
            || pSettings1->bFreeRun != pSettings2->bFreeRun
            || pSettings1->bLeftAlign != pSettings2->bLeftAlign
            || pSettings1->bMSBFirst != pSettings2->bMSBFirst
            || pSettings1->byInputSelect != pSettings2->byInputSelect
            || pSettings1->byMSBIndex != pSettings2->byMSBIndex
            || pSettings1->dwSampleRate != pSettings2->dwSampleRate
           );
}

void InitFreeRunAudioSignalStatus(
        AUDIO_SIGNAL_STATUS * pStatus
        )
{
    pStatus->pubStatus.wChannelValid = 0x00;
    pStatus->pubStatus.bLPCM = TRUE;
    pStatus->pubStatus.cBitsPerSample = 24;
    pStatus->pubStatus.dwSampleRate = 48000;
    pStatus->pubStatus.bChannelStatusValid = FALSE;
    pStatus->bFreeRun = TRUE;
    pStatus->audioInputMode = AUDIO_INPUT_SERIAL;
    pStatus->bLeftAlign = TRUE;
    pStatus->bMSBFirst = TRUE;
    pStatus->byMSBIndex = 31;
    pStatus->byInputSelect = 0;
    pStatus->bAnalogVolumeControl = FALSE;
    pStatus->byChannelOffset = 0;
}

void InitFreeRunAudioCaptureSettings(
        AUDIO_CAPTURE_SETTINGS * pSettings
        )
{
    pSettings->wChannelValid = 0x00;
    pSettings->dwSampleRate = 48000;
    pSettings->bFreeRun = TRUE;
    pSettings->audioInputMode = AUDIO_INPUT_SERIAL;
    pSettings->bLeftAlign = TRUE;
    pSettings->bMSBFirst = TRUE;
    pSettings->byMSBIndex = 31;
    pSettings->byInputSelect = 0;
}

BOOLEAN IsSupportInputSource(
        DWORD dwInputSource,
        const DWORD * pdwInputSources,
        int cInputSources
        )
{
    int i;

    for (i = 0; i < cInputSources; i++)
        if (pdwInputSources[i] == dwInputSource)
            return TRUE;

    return FALSE;
}

