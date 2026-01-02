////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "osd-bound-rect.h"

void ScaleRect(RECT * pRectBound, int cxSrc, int cySrc, int cxDest, int cyDest)
{
    pRectBound->left = pRectBound->left * cxDest / cxSrc;
    pRectBound->top = pRectBound->top * cyDest / cySrc;
    pRectBound->right = (pRectBound->right * cxDest + cxSrc - 1) / cxSrc;
    pRectBound->bottom = (pRectBound->bottom * cyDest + cySrc - 1) / cySrc;
}

static void GetBoundRect(const BYTE * pbyData, int cx, int cy, int cbStride, RECT * pRectBound)
{
    int32_t x, y;

    pRectBound->top = cy;
    pRectBound->bottom = -1;
    pRectBound->left = cx;
    pRectBound->right = -1;

    for (y = 0; y < cy; y++) {
        for (x = 0; x < cx; x++) {
            if (pbyData[x * 4 + 3] != 0x00) {
                pRectBound->left = min(pRectBound->left, x);
                pRectBound->top = min(pRectBound->top, y);
                pRectBound->right = max(pRectBound->right, x + 1);
                pRectBound->bottom = max(pRectBound->bottom, y + 1);
            }
        }

        pbyData += cbStride;
    }
}

static int GetOSDBoundRectsDivXY(const BYTE * pbyRGBA, int cx, int cy, int cbStride, RECT * pOSDRects, int * pnArea)
{
    int cxDiv = cx / 2;
    int cyDiv = cy / 2;
    const BYTE * pbyRectLeftTop = pbyRGBA;
    const BYTE * pbyRectRightTop = pbyRectLeftTop + cxDiv * 4;
    const BYTE * pbyRectLeftBottom = pbyRGBA + cbStride * cyDiv;
    const BYTE * pbyRectRightBottom = pbyRectLeftBottom + cxDiv * 4;

    RECT rectBound;
    int cOSDRects = 0;
    *pnArea = 0;

    GetBoundRect(pbyRectLeftTop, cxDiv, cyDiv, cbStride, &rectBound);
    if (!IsRectEmpty(&rectBound)) {
        *pnArea = (rectBound.right - rectBound.left) * (rectBound.bottom - rectBound.top);
        os_memcpy(&pOSDRects[cOSDRects++], &rectBound, sizeof(RECT));
    }

    GetBoundRect(pbyRectRightTop, cx - cxDiv, cyDiv, cbStride, &rectBound);
    OffsetRect(&rectBound, cxDiv, 0);
    if (!IsRectEmpty(&rectBound)) {
        *pnArea += (rectBound.right - rectBound.left) * (rectBound.bottom - rectBound.top);
        os_memcpy(&pOSDRects[cOSDRects++], &rectBound, sizeof(RECT));
    }

    GetBoundRect(pbyRectLeftBottom, cxDiv, cy - cyDiv, cbStride, &rectBound);
    OffsetRect(&rectBound, 0, cyDiv);
    if (!IsRectEmpty(&rectBound)) {
        *pnArea += (rectBound.right - rectBound.left) * (rectBound.bottom - rectBound.top);
        os_memcpy(&pOSDRects[cOSDRects++], &rectBound, sizeof(RECT));
    }

    GetBoundRect(pbyRectRightBottom, cx - cxDiv, cy - cyDiv, cbStride, &rectBound);
    OffsetRect(&rectBound, cxDiv, cyDiv);
    if (!IsRectEmpty(&rectBound)) {
        *pnArea += (rectBound.right - rectBound.left) * (rectBound.bottom - rectBound.top);
        os_memcpy(&pOSDRects[cOSDRects++], &rectBound, sizeof(RECT));
    }

    return cOSDRects;
}

static int GetOSDBoundRectsDivX(const BYTE * pbyRGBA, int cx, int cy, int cbStride, RECT * pOSDRects, int * pnArea)
{
    int cxDiv = cx / 4;
    const BYTE * pbyRect1 = pbyRGBA;
    const BYTE * pbyRect2 = pbyRect1 + cxDiv * 4;
    const BYTE * pbyRect3 = pbyRect2 + cxDiv * 4;
    const BYTE * pbyRect4 = pbyRect3 + cxDiv * 4;

    RECT rectBound;
    int cOSDRects = 0;
    *pnArea = 0;

    GetBoundRect(pbyRect1, cxDiv, cy, cbStride, &rectBound);
    if (!IsRectEmpty(&rectBound)) {
        *pnArea = (rectBound.right - rectBound.left) * (rectBound.bottom - rectBound.top);
        os_memcpy(&pOSDRects[cOSDRects++], &rectBound, sizeof(RECT));
    }

    GetBoundRect(pbyRect2, cxDiv, cy, cbStride, &rectBound);
    OffsetRect(&rectBound, cxDiv, 0);
    if (!IsRectEmpty(&rectBound)) {
        *pnArea += (rectBound.right - rectBound.left) * (rectBound.bottom - rectBound.top);
        os_memcpy(&pOSDRects[cOSDRects++], &rectBound, sizeof(RECT));
    }

    GetBoundRect(pbyRect3, cxDiv, cy, cbStride, &rectBound);
    OffsetRect(&rectBound, cxDiv * 2, 0);
    if (!IsRectEmpty(&rectBound)) {
        *pnArea += (rectBound.right - rectBound.left) * (rectBound.bottom - rectBound.top);
        os_memcpy(&pOSDRects[cOSDRects++], &rectBound, sizeof(RECT));
    }

    GetBoundRect(pbyRect4, cx - cxDiv * 3, cy, cbStride, &rectBound);
    OffsetRect(&rectBound, cxDiv * 3, 0);
    if (!IsRectEmpty(&rectBound)) {
        *pnArea += (rectBound.right - rectBound.left) * (rectBound.bottom - rectBound.top);
        os_memcpy(&pOSDRects[cOSDRects++], &rectBound, sizeof(RECT));
    }

    return cOSDRects;
}

static int GetOSDBoundRectsDivY(const BYTE * pbyRGBA, int cx, int cy, int cbStride, RECT * pOSDRects, int * pnArea)
{
    int cyDiv = cy / 4;
    const BYTE * pbyRect1 = pbyRGBA;
    const BYTE * pbyRect2 = pbyRect1 + cyDiv * cbStride;
    const BYTE * pbyRect3 = pbyRect2 + cyDiv * cbStride;
    const BYTE * pbyRect4 = pbyRect3 + cyDiv * cbStride;

    RECT rectBound;
    int cOSDRects = 0;
    *pnArea = 0;

    GetBoundRect(pbyRect1, cx, cyDiv, cbStride, &rectBound);
    if (!IsRectEmpty(&rectBound)) {
        *pnArea = (rectBound.right - rectBound.left) * (rectBound.bottom - rectBound.top);
        os_memcpy(&pOSDRects[cOSDRects++], &rectBound, sizeof(RECT));
    }

    GetBoundRect(pbyRect2, cx, cyDiv, cbStride, &rectBound);
    OffsetRect(&rectBound, 0, cyDiv);
    if (!IsRectEmpty(&rectBound)) {
        *pnArea += (rectBound.right - rectBound.left) * (rectBound.bottom - rectBound.top);
        os_memcpy(&pOSDRects[cOSDRects++], &rectBound, sizeof(RECT));
    }

    GetBoundRect(pbyRect3, cx, cyDiv, cbStride, &rectBound);
    OffsetRect(&rectBound, 0, cyDiv * 2);
    if (!IsRectEmpty(&rectBound)) {
        *pnArea += (rectBound.right - rectBound.left) * (rectBound.bottom - rectBound.top);
        os_memcpy(&pOSDRects[cOSDRects++], &rectBound, sizeof(RECT));
    }

    GetBoundRect(pbyRect4, cx, cy - cyDiv * 3, cbStride, &rectBound);
    OffsetRect(&rectBound, 0, cyDiv * 3);
    if (!IsRectEmpty(&rectBound)) {
        *pnArea += (rectBound.right - rectBound.left) * (rectBound.bottom - rectBound.top);
        os_memcpy(&pOSDRects[cOSDRects++], &rectBound, sizeof(RECT));
    }

    return cOSDRects;
}

int GetOSDBoundRects(const BYTE * pbyRGBA, int cx, int cy, int cbStride, RECT * pOSDRects)
{
    int nArea;
    int nAreaDivX;
    int nAreaDivY;
    RECT aOSDRectsDivX[4];
    RECT aOSDRectsDivY[4];
    int cOSDRectsDivX;
    int cOSDRectsDivY;

    int cOSDRects = GetOSDBoundRectsDivXY(pbyRGBA, cx, cy, cbStride, pOSDRects, &nArea);
    if (cOSDRects == 0)
        return 0;

    cOSDRectsDivX = GetOSDBoundRectsDivX(pbyRGBA, cx, cy, cbStride, aOSDRectsDivX, &nAreaDivX);
    if (nAreaDivX < nArea) {
        os_memcpy(pOSDRects, aOSDRectsDivX, sizeof(RECT) * cOSDRectsDivX);
        cOSDRects = cOSDRectsDivX;
        nArea = nAreaDivX;
    }

    cOSDRectsDivY = GetOSDBoundRectsDivY(pbyRGBA, cx, cy, cbStride, aOSDRectsDivY, &nAreaDivY);
    if (nAreaDivY < nArea) {
        os_memcpy(pOSDRects, aOSDRectsDivY, sizeof(RECT) * cOSDRectsDivY);
        cOSDRects = cOSDRectsDivY;
        nArea = nAreaDivY;
    }

    return cOSDRects;
}
