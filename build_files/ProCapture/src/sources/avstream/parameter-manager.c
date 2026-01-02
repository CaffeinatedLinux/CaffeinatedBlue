////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#include "parameter-manager.h"
#include "xi-driver.h"

#include "edid.h"

static void _LoadDefaultEDID(struct parameter_t *par);

int parameter_manager_init(struct parameter_t *par, void *driver, PARAMETER_EDID_TYPE nEDIDType)
{
    int i;

    par->driver = driver;

    par->m_byFanDuty = 0;
    par->m_bEnablePreview = 0;
    par->m_dwTimestampOffsetVideo = 0;
    par->m_dwTimestampOffsetAudio = 0;

    par->m_dwLEDMode = MWCAP_LED_AUTO;

    par->m_nEDIDType = nEDIDType;

	par->m_bEnableXBar = TRUE;
    par->m_bTopFieldFirst = TRUE;
	par->m_bEnableHDCP = FALSE;

    // Load EDID
    _LoadDefaultEDID(par);
    par->m_byEDIDMode = MWCAP_EDID_MODE_ADD_AUDIO;

    // Signal detection
    par->m_bEnableHPSyncMeter = TRUE;
    par->m_dwFrameDurationChangeThreshold = 100;

    par->m_dwVideoSyncSliceLevel = 365;

    // Auto scan
    par->m_bInputSourceScan = TRUE;
    par->m_dwScanPeriod = 200;
    par->m_dwScanKeepDuration = 500;
    karray_init(&par->m_arrScanInputSources, sizeof(DWORD));

    par->m_bAVInputSourceLink = TRUE;

    par->m_dwVideoInputSource = INPUT_SOURCE(MWCAP_VIDEO_INPUT_TYPE_NONE, 0);
    par->m_dwAudioInputSource = INPUT_SOURCE(MWCAP_AUDIO_INPUT_TYPE_NONE, 0);

    par->m_byVADThreshold = 64;

    // Audio controls
    par->m_abMute[0] = FALSE;
    par->m_abMute[1] = FALSE;
    par->m_anVolume[0] = 0;
    par->m_anVolume[1] = 0;
    for (i = 0; i < 8 * MAX_CHANNELS_PER_DEVICE; i++) {
        par->m_abMute[i] = FALSE;
        par->m_anVolume[i] = 0;
    }

    par->m_nSampleRateAdjustment = 0;

	// Video controls
	par->m_nVideoInputAspectX = 0;
	par->m_nVideoInputAspectY = 0;
	par->m_colorFormatVideoInput = MWCAP_VIDEO_COLOR_FORMAT_UNKNOWN;
	par->m_quantRangeVideoInput = MWCAP_VIDEO_QUANTIZATION_UNKNOWN;

    // Timings
    par->m_bAutoHAlign = TRUE;
    par->m_bySamplingPhase = 32;
    par->m_bAutoSamplingPhaseAdjust = TRUE;

    karray_init(&par->m_arrCustomVideoTimings,
                sizeof(MWCAP_VIDEO_CUSTOM_TIMING));
    karray_init(&par->m_arrVideoResolutions, sizeof(MWCAP_SIZE));

    par->m_nLowLatencyStripeHeight = 64;

    par->m_nTimestampMode = TIMESTAMP_MODE_NORMAL;

    par->m_bQuadSDIForceConvMode2SI = FALSE;

	return 0;
}

void parameter_manager_deinit(struct parameter_t *par)
{
    karray_free(&par->m_arrScanInputSources);
    karray_free(&par->m_arrCustomVideoTimings);
    karray_free(&par->m_arrVideoResolutions);
}

static void _LoadDefaultEDID(struct parameter_t *par)
{
    const BYTE * pbyEDID = NULL;

    switch (par->m_nEDIDType) {
    case PARAMETER_EDID_2K:
        pbyEDID = g_abyEDID;
        par->m_cbEDID = sizeof(g_abyEDID);
        break;
    case PARAMETER_EDID_4K:
        pbyEDID = g_abyEDID_4K;
        par->m_cbEDID = sizeof(g_abyEDID_4K);
        break;
    case PARAMETER_EDID_4Kp:
        pbyEDID = g_abyEDID_4Kp;
        par->m_cbEDID = sizeof(g_abyEDID_4Kp);
        break;
    case PARAMETER_EDID_DVI_4K:
        pbyEDID = g_abyEDID_DVI_4K;
        par->m_cbEDID = sizeof(g_abyEDID_DVI_4K);
        break;
    default:
        return;
    }

    os_memcpy(par->m_abyEDID, pbyEDID, par->m_cbEDID);
}

void parameter_SetEDID(struct parameter_t *par, const BYTE * pbyEDID, WORD cbEDID)
{
    if (cbEDID == 0)
        parameter_ResetEDID(par);
    else {
        par->m_cbEDID = min(sizeof(par->m_abyEDID), (size_t)cbEDID);
        os_memcpy(par->m_abyEDID, pbyEDID, par->m_cbEDID);
    }
}

void parameter_ResetEDID(struct parameter_t *par)
{
    _LoadDefaultEDID(par);
}

BOOLEAN parameter_SetScanInputSources(struct parameter_t *par,
        const DWORD * pdwScanInputSources, int cScanInputSources)
{
    return karray_copy(&par->m_arrScanInputSources, (void *)pdwScanInputSources,
                       cScanInputSources);
}

const DWORD *parameter_GetScanInputSources(struct parameter_t *par)
{
    if (!karray_is_empty(&par->m_arrScanInputSources))
        return karray_get_data(&par->m_arrScanInputSources);

    return xi_driver_get_supported_video_input_sources(par->driver, NULL);
}

int parameter_GetScanInputSourceCount(struct parameter_t *par)
{
    int cSupportedVideoInputSources = 0;

    if (!karray_is_empty(&par->m_arrScanInputSources))
        return (int)karray_get_count(&par->m_arrScanInputSources);

    xi_driver_get_supported_video_input_sources(
                par->driver, &cSupportedVideoInputSources);
	return cSupportedVideoInputSources;
}

BOOLEAN parameter_SetCustomVideoTimings(struct parameter_t *par,
        const MWCAP_VIDEO_CUSTOM_TIMING *pVideoTimings, int cVideoTimings)
{
    return karray_copy(&par->m_arrCustomVideoTimings, (void *)pVideoTimings,
                       cVideoTimings);
}

const MWCAP_VIDEO_CUSTOM_TIMING *parameter_GetCustomVideoTimings(struct parameter_t *par)
{
    return karray_get_data(&par->m_arrCustomVideoTimings);
}

int parameter_GetCustomVideoTimingCount(struct parameter_t *par)
{
    return (int)karray_get_count(&par->m_arrCustomVideoTimings);
}

BOOLEAN parameter_SetVideoResolutions(struct parameter_t *par,
        const MWCAP_SIZE * pVideoResolutions, int cVideoResolutions) 
{
    return karray_copy(&par->m_arrVideoResolutions, (void *)pVideoResolutions,
                       cVideoResolutions);
}

const MWCAP_SIZE *parameter_GetVideoResolutions(struct parameter_t *par)
{
    return karray_get_data(&par->m_arrVideoResolutions);
}

int parameter_GetVideoResolutionCount(struct parameter_t *par)
{
    return (int)karray_get_count(&par->m_arrVideoResolutions);
}

