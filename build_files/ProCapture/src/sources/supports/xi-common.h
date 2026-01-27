////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#ifndef __XI_COMMON_H__
#define __XI_COMMON_H__

#include "ospi/ospi.h"
#include "public/win-types.h"

typedef struct _REGISTER_ITEM {
	u8 devaddr;
	u8 regaddr;
	u8 value;
} REGISTER_ITEM, *PREGISTER_ITEM;

static inline DWORD ValueInRange(DWORD dwValue, DWORD dwCompare, int nThresholdPPM)
{
    DWORD dwThreshold = (DWORD)os_div64((dwCompare * (LONGLONG)nThresholdPPM + 500000), 1000000);
    return (dwValue <= (dwCompare + dwThreshold) && (dwValue + dwThreshold) >= dwCompare);
}

static inline BOOLEAN ValueBestMatch(DWORD *dwValue, int cMatchs, const DWORD *pdwMatchs, int nThresholdPPM)
{
    for (; cMatchs > 0; cMatchs--, pdwMatchs++) {
        if (ValueInRange(*dwValue, *pdwMatchs, nThresholdPPM)) {
            *dwValue = *pdwMatchs;
            return TRUE;
        }
    }

    return FALSE;
}

#endif /* __XI_COMMON_H__ */

