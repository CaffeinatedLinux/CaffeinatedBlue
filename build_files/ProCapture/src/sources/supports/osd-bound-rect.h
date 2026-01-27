////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ospi/ospi.h"

#include "mw-procapture-extension.h"

static inline BOOLEAN IsRectEmpty(const RECT * pRect)
{
    return (pRect->left >= pRect->right || pRect->top >= pRect->bottom);
}

static inline void OffsetRect(RECT * pRect, int xOffset, int yOffset)
{
    pRect->left += xOffset;
    pRect->right += xOffset;
    pRect->top += yOffset;
    pRect->bottom += yOffset;
}

void ScaleRect(RECT * pRectBound, int cxSrc, int cySrc, int cxDest, int cyDest);

int GetOSDBoundRects(const BYTE * pbyRGBA, int cx, int cy, int cbStride, RECT * pOSDRects);
