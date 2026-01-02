////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "ospi/ospi.h"
#include "public/win-types.h"

DWORD g_adwAudioSampleRates[] =
{
    8000,
    16000,
    22050,
    24000,
    32000,
    44100,
    48000,
    64000,
    88200,
    96000,
    128000,
    176400,
    192000
};

int g_cAudioSampleRates = ARRAY_SIZE(g_adwAudioSampleRates);
