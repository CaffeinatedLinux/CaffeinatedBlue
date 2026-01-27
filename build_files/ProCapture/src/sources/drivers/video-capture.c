////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "video-capture.h"

#include "supports/sort.h"

#include "avstream/xi-driver-priv.h"

enum REG_ADDR {
    REG_ADDR_VER_CAPS					= 4 * 0,

    REG_ADDR_INT_ENABLE					= 4 * 1,
    REG_ADDR_INT_STATUS					= 4 * 2,
    REG_ADDR_INT_RAW_STATUS				= 4 * 3,

    REG_ADDR_RESET						= 4 * 4,

    REG_ADDR_CONTROL_PORT				= 4 * 5,
    REG_ADDR_CONTROL_PORT_SET			= 4 * 6,
    REG_ADDR_CONTROL_PORT_CLEAR			= 4 * 7,

    REG_ADDR_INPUT_CONTROL				= 4 * 8,
    REG_ADDR_INPUT_STATUS				= 4 * 8,

    REG_ADDR_INPUT_CLIP_X_CONTROL		= 4 * 9,
    REG_ADDR_INPUT_CLIP_F0_Y_CONTROL	= 4 * 10,
    REG_ADDR_INPUT_CLIP_F1_Y_CONTROL	= 4 * 11,

    REG_ADDR_INPUT_TIMESTAMP_LOW		= 4 * 12,
    REG_ADDR_INPUT_TIMESTAMP_HIGH		= 4 * 13,
    REG_ADDR_INPUT_FRAME_INFO			= 4 * 14,

    REG_ADDR_VFS_FULL_TIMESTAMP_LOW		= 4 * 15,
    REG_ADDR_VFS_FULL_TIMESTAMP_HIGH	= 4 * 16,
    REG_ADDR_VFS_FULL_FRAME_INFO		= 4 * 17,
    REG_ADDR_VFS_FULL_STRIPE_INFO		= 4 * 18,

    REG_ADDR_VFS_QUARTER_TIMESTAMP_LOW	= 4 * 19,
    REG_ADDR_VFS_QUARTER_TIMESTAMP_HIGH	= 4 * 20,
    REG_ADDR_VFS_QUARTER_FRAME_INFO		= 4 * 21,
    REG_ADDR_VFS_QUARTER_STRIPE_INFO	= 4 * 22,

    REG_ADDR_VFS_FULL_LINE_CONTROL		= 4 * 23,
    REG_ADDR_VFS_FULL_MAX_LINE_ID		= 4 * 24,

    REG_ADDR_VFS_QUARTER_LINE_CONTROL	= 4 * 25,
    REG_ADDR_VFS_QUARTER_MAX_LINE_ID	= 4 * 26,

    REG_ADDR_INPUT_CLIP_P1_F0_Y_CONTROL = 4 * 27,
    REG_ADDR_INPUT_CLIP_P1_F1_Y_CONTROL = 4 * 28,

    REG_ADDR_VFS_FULL_HALF_FRAME_OFFSET = 4 * 29,
    REG_ADDR_VFS_QUARTER_HALF_FRAME_OFFSET = 4 * 30,

    REG_ADDR_VPP_WB_CONTROL				= 4 * 32,
    REG_ADDR_VPP_WB_STATUS				= 4 * 32,

    REG_ADDR_VPP_WB_FBWR_CONTROL		= 4 * 33,
    REG_ADDR_VPP_WB_FBWR_ADDRESS		= 4 * 34,
    REG_ADDR_VPP_WB_FBWR_STRIDE			= 4 * 35,
    REG_ADDR_VPP_WB_FBWR_LINE_CONTROL	= 4 * 36,

    REG_ADDR_VPP1_BASE					= 4 * 40,
    REG_ADDR_VPP2_BASE					= 4 * 80
};

enum VPP_REG_OFFSET {
    VPP_REG_OFFSET_CONTROL				= 4 * 0,
    VPP_REG_OFFSET_STATUS				= 4 * 0,

    VPP_REG_OFFSET_FBRD_CONTROL			= 4 * 1,
    VPP_REG_OFFSET_FBRD_ADDRESS			= 4 * 2,
    VPP_REG_OFFSET_FBRD_STRIDE			= 4 * 3,
    VPP_REG_OFFSET_FBRD_LINE_CONTROL	= 4 * 4,

    VPP_REG_OFFSET_SCALER_X_CONTROL		= 4 * 5,
    VPP_REG_OFFSET_SCALER_Y_CONTROL		= 4 * 6,
    VPP_REG_OFFSET_SCALER_INPUT_SIZE	= 4 * 7,

    VPP_REG_OFFSET_BORDER_SIZE			= 4 * 8,
    VPP_REG_OFFSET_BORDER_X_CONTROL		= 4 * 9,
    VPP_REG_OFFSET_BORDER_Y_CONTROL		= 4 * 10,

    VPP_REG_OFFSET_BLANK_CONTROL_1		= 4 * 11,
    VPP_REG_OFFSET_BLANK_CONTROL_2		= 4 * 12,

    VPP_REG_OFFSET_MATRIX_RV_COEFF_1	= 4 * 13,
    VPP_REG_OFFSET_MATRIX_RV_COEFF_2	= 4 * 14,
    VPP_REG_OFFSET_MATRIX_RV_RANGE		= 4 * 15,
    VPP_REG_OFFSET_MATRIX_GY_COEFF_1	= 4 * 16,
    VPP_REG_OFFSET_MATRIX_GY_COEFF_2	= 4 * 17,
    VPP_REG_OFFSET_MATRIX_GY_RANGE		= 4 * 18,
    VPP_REG_OFFSET_MATRIX_BU_COEFF_1	= 4 * 19,
    VPP_REG_OFFSET_MATRIX_BU_COEFF_2	= 4 * 20,
    VPP_REG_OFFSET_MATRIX_BU_RANGE		= 4 * 21,

    VPP_REG_OFFSET_OSD_CONTROL			= 4 * 22,
    VPP_REG_OFFSET_OSD_ADDRESS			= 4 * 23,
    VPP_REG_OFFSET_OSD0_X_CONTROL		= 4 * 24,
    VPP_REG_OFFSET_OSD0_Y_CONTROL		= 4 * 25,
    VPP_REG_OFFSET_OSD1_X_CONTROL		= 4 * 26,
    VPP_REG_OFFSET_OSD1_Y_CONTROL		= 4 * 27,
    VPP_REG_OFFSET_OSD2_X_CONTROL		= 4 * 28,
    VPP_REG_OFFSET_OSD2_Y_CONTROL		= 4 * 29,
    VPP_REG_OFFSET_OSD3_X_CONTROL		= 4 * 30,
    VPP_REG_OFFSET_OSD3_Y_CONTROL		= 4 * 31
};

enum CONTROL_PORT {
    CONTROL_PORT_BIT_VPP1_BLANK			= 0x40000000,
    CONTROL_PORT_BIT_VPP2_BLANK			= 0x80000000
};

#define COLOR_DEPTH			10

static void _InitFrameContext(
        VPP_FRAME_CONTEXT * pFrameContext
        );
static void _SetVideoBlank(
        struct video_capture *vc,
        int iPipeline,
        BOOLEAN bBlank
        );

// 16bit color
static void _SetVideoBlankColor(
        struct video_capture *vc,
        int iPipeline,
        WORD wBlankRV,
        WORD wBlankGY,
        WORD wBlankBU
        );

int video_capture_Initialize(
        struct video_capture *vc,
        volatile void __iomem *reg_base,
        int iChannel,
        int cMaxFrames,
        DWORD dwFullFrameBufferAddr, 
        DWORD cbFullFrame, 
        DWORD dwQuarterFrameBufferAddr, 
        DWORD cbQuarterFrame,
        int nPixelsPerCycle
        )
{
    int i;
    vc->reg_base = reg_base;

    vc->m_iChannel = iChannel;
    vc->m_dwFullFrameBufferAddr = dwFullFrameBufferAddr + cbFullFrame * cMaxFrames * iChannel;
    vc->m_cbFullFrame = cbFullFrame;
    vc->m_dwQuarterFrameBufferAddr = dwQuarterFrameBufferAddr;
    vc->m_cbQuarterFrame = cbQuarterFrame;

    os_memset(&vc->m_bufferState, 0, sizeof(vc->m_bufferState));
    vc->m_bufferState.cMaxFrames = min(MWCAP_MAX_VIDEO_FRAME_COUNT, cMaxFrames);

    _InitFrameContext(&vc->m_frameContextLast);
    for (i = 0; i < MWCAP_MAX_VIDEO_FRAME_COUNT; i++)
        _InitFrameContext(&vc->m_frameContexts[i]);

    vc->m_lock = os_spin_lock_alloc();
    if (vc->m_lock == NULL)
        return OS_RETURN_NOMEM;

    vc->m_nPixelsPerCycle = nPixelsPerCycle;

    return OS_RETURN_SUCCESS;
}

void video_capture_Deinitialize(struct video_capture *vc)
{
    if (vc->m_lock) {
        os_spin_lock_free(vc->m_lock);
        vc->m_lock = NULL;
    }
}


DWORD video_capture_GetFullFrameBaseAddr(
        struct video_capture *vc,
        int iFrameID
        )
{
    return vc->m_dwFullFrameBufferAddr + vc->m_cbFullFrame * (DWORD)iFrameID;
}

DWORD video_capture_GetQuarterFrameBaseAddr(
        struct video_capture *vc,
        int iFrameID
        )
{
    return vc->m_dwQuarterFrameBufferAddr + vc->m_cbQuarterFrame * (DWORD)iFrameID;
}

void video_capture_SetIntEnables(
        struct video_capture *vc,
        DWORD dwEnableBits
        )
{
    pci_write_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_INT_ENABLE), dwEnableBits);
}

DWORD video_capture_GetCaps(struct video_capture *vc)
{
    return pci_read_reg32(vc->reg_base+REG_ADDR_VER_CAPS);
}

DWORD video_capture_GetIntStatus(struct video_capture *vc)
{
    return pci_read_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_INT_STATUS));
}

DWORD video_capture_GetIntRawStatus(struct video_capture *vc)
{
    return pci_read_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_INT_RAW_STATUS));
}

void video_capture_ClearIntRawStatus(struct video_capture *vc, DWORD dwClearBits)
{
    pci_write_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_INT_RAW_STATUS), dwClearBits);
}

void video_capture_SetControlPortValue(struct video_capture *vc, DWORD dwValue)
{
    pci_write_reg32(vc->reg_base+REG_ADDR_CONTROL_PORT, dwValue);
}

void video_capture_SetControlPortBits(struct video_capture *vc, DWORD dwBits)
{
    pci_write_reg32(vc->reg_base+REG_ADDR_CONTROL_PORT_SET, dwBits);
}

void video_capture_ClearControlPortBits(struct video_capture *vc, DWORD dwBits)
{
    pci_write_reg32(vc->reg_base+REG_ADDR_CONTROL_PORT_CLEAR, dwBits);
}

void video_capture_StartInput(
        struct video_capture *vc,
        BOOLEAN bLineAlternative,
        BOOLEAN bInterlacedToTopBottom,
        BOOLEAN bFramePacking,
        int yRightFrame,
        BOOLEAN bInterlacedFramePacking,
        int cyInterlacedFramePacking,
        BOOLEAN bInterlaced,
        BOOLEAN bSegmentedFrame,
        BOOLEAN bInvertField,
        BOOLEAN bTopFieldFirst,
        MWCAP_VIDEO_COLOR_FORMAT colorFormat,
        MWCAP_VIDEO_QUANTIZATION_RANGE quantRange,
        SHORT sRVBUScale,
        SHORT sRVBUOffset,
        int x,
        int y,
        int cx,
        int cy,
        int nAspectX,
        int nAspectY,
        int cyFullStripe,
        int cyQuarterStripe,
        int nStartFrameId
        )
{
    DWORD dwValue;

    int nLeftField0Top, nLeftField0Bottom;
    int nLeftField1Top, nLeftField1Bottom;
    int nRightField0Top = 0, nRightField0Bottom = 0;
    int nRightField1Top = 0, nRightField1Bottom = 0;
    int cField0TotalLines, cField1TotalLines;
    int xRight = 0;

    x = x / vc->m_nPixelsPerCycle * vc->m_nPixelsPerCycle;
    xRight = (x + cx + vc->m_nPixelsPerCycle - 1) / vc->m_nPixelsPerCycle * vc->m_nPixelsPerCycle;
    cx = xRight - x;

    xi_debug(5, "cx:%d,cy:%d,nStartFrameId:%d,x:%d\n", cx, cy, nStartFrameId, x);

	if (cx < 64 || cy < 64)
        return;

	{
        os_spin_lock_bh(vc->m_lock);

        vc->m_frameIDForANC = -1;

        vc->m_frameContextLast.pubInfo.bInterlaced = bInterlaced;
        vc->m_frameContextLast.pubInfo.bSegmentedFrame = bSegmentedFrame;
        vc->m_frameContextLast.pubInfo.cx = cx;
        vc->m_frameContextLast.pubInfo.cy = cy;

        vc->m_frameContextLast.cxQuarterFrame = (cx + 1) / 2;

        vc->m_frameContextLast.cbFullFrameLine = cx * 4;
        vc->m_frameContextLast.cbQuarterFrameLine
                = vc->m_frameContextLast.cxQuarterFrame * 4;

        if (bInterlaced) {
            nLeftField0Top = (y + 1) / 2;
            nLeftField1Top = y / 2;

            if (bFramePacking) {
                nLeftField0Bottom = (y + cy / 2 - 1) / 2;
                nLeftField1Bottom = (y + cy / 2 - 2) / 2;

                nRightField0Top = (yRightFrame + 1) / 2;
                nRightField1Top = yRightFrame / 2;

                nRightField0Bottom = (yRightFrame + cy / 2 - 1) / 2;
                nRightField1Bottom = (yRightFrame + cy / 2 - 2) / 2;

                cField0TotalLines = (nLeftField0Bottom - nLeftField0Top + 1) + (nRightField0Bottom - nRightField0Top + 1);
                cField1TotalLines = (nLeftField1Bottom - nLeftField1Top + 1) + (nRightField1Bottom - nRightField1Top + 1);
            }
            else {
                nLeftField0Bottom = (y + cy - 1) / 2;
                nLeftField1Bottom = (y + cy - 2) / 2;

                cField0TotalLines = (nLeftField0Bottom - nLeftField0Top + 1);
                cField1TotalLines = (nLeftField1Bottom - nLeftField1Top + 1);
            }

            vc->m_frameContextLast.cyQuarterFrame = bTopFieldFirst ? cField0TotalLines : cField1TotalLines;

            if (nLeftField0Top == nLeftField1Top) {
                vc->m_frameContextLast.pubInfo.bTopFieldFirst = bTopFieldFirst;
                vc->m_frameContextLast.pubInfo.bTopFieldInverted = FALSE;
            }
            else {
                vc->m_frameContextLast.pubInfo.bTopFieldFirst = !bTopFieldFirst;
                vc->m_frameContextLast.pubInfo.bTopFieldInverted = TRUE;
            }
        }
        else {
            nLeftField0Top = nLeftField1Top = y;

            if (bFramePacking) {
                nLeftField0Bottom = nLeftField1Bottom = y + (cy / 2) - 1;
                nRightField0Top = nRightField1Top = yRightFrame;
                nRightField0Bottom = nRightField1Bottom = yRightFrame + (cy / 2) - 1;
            }
            else
                nLeftField0Bottom = nLeftField1Bottom = y + cy - 1;

            cField0TotalLines = cField1TotalLines = cy;

            vc->m_frameContextLast.cyQuarterFrame = (cy + 1) / 2;
            vc->m_frameContextLast.pubInfo.bTopFieldFirst = TRUE;
            vc->m_frameContextLast.pubInfo.bTopFieldInverted = FALSE;
        }

        vc->m_frameContextLast.colorFormat = colorFormat;
        vc->m_frameContextLast.quantRange = quantRange;
        vc->m_frameContextLast.sRVBUScale = sRVBUScale;
        vc->m_frameContextLast.sRVBUOffset = sRVBUOffset;

        vc->m_frameContextLast.pubInfo.nAspectX = nAspectX;
        vc->m_frameContextLast.pubInfo.nAspectY = nAspectY;

        os_spin_unlock_bh(vc->m_lock);
	}

    pci_write_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_INPUT_CLIP_X_CONTROL), x | ((xRight - 1) << 16));

    pci_write_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_INPUT_CLIP_F0_Y_CONTROL), nLeftField0Top | (nLeftField0Bottom << 16));
    pci_write_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_INPUT_CLIP_F1_Y_CONTROL), nLeftField1Top | (nLeftField1Bottom << 16));

    if (bFramePacking) {
        pci_write_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_INPUT_CLIP_P1_F0_Y_CONTROL), nRightField0Top | (nRightField0Bottom << 16));
        pci_write_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_INPUT_CLIP_P1_F1_Y_CONTROL), nRightField1Top | (nRightField1Bottom << 16));
    }

    pci_write_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_VFS_FULL_LINE_CONTROL), vc->m_frameContextLast.cbFullFrameLine | (cyFullStripe << 20));
    pci_write_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_VFS_FULL_MAX_LINE_ID), (cField0TotalLines - 1) | ((cField1TotalLines - 1) << 16));

    pci_write_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_VFS_QUARTER_LINE_CONTROL), vc->m_frameContextLast.cbQuarterFrameLine | (cyQuarterStripe << 20));
    pci_write_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_VFS_QUARTER_MAX_LINE_ID), (vc->m_frameContextLast.cyQuarterFrame - 1));

    pci_write_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_VFS_FULL_HALF_FRAME_OFFSET), vc->m_frameContextLast.cbFullFrameLine * (cy / 2));
    pci_write_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_VFS_QUARTER_HALF_FRAME_OFFSET),
                    vc->m_frameContextLast.cbQuarterFrameLine * (vc->m_frameContextLast.cyQuarterFrame / 2));

	// Control register
	dwValue = 0x01;
	if (bInterlaced) dwValue |= 0x02;
	if (bInvertField) dwValue |= 0x04;
    if (bTopFieldFirst) dwValue |= 0x08;
    if (bLineAlternative) dwValue |= 0x10;
    if (bInterlacedToTopBottom) dwValue |= 0x20;
    if (bInterlacedFramePacking) dwValue |= 0x40;
    dwValue |= ((nStartFrameId & 0x0F) << 8);
    dwValue |= (cyInterlacedFramePacking << 16);

	pci_write_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_INPUT_CONTROL), dwValue);
}

ULONGLONG video_capture_StopInput(struct video_capture *vc)
{
    int nFrameID;
    VPP_FRAME_CONTEXT * pFrameContext;

    pci_write_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_INPUT_CONTROL), 0x00);

    os_spin_lock_bh(vc->m_lock);

    nFrameID = vc->m_bufferState.iNewestBuffering;
    pFrameContext = &vc->m_frameContexts[nFrameID];
    if (pFrameContext->pubInfo.state != MWCAP_VIDEO_FRAME_STATE_BUFFERED) {
        _InitFrameContext(pFrameContext);
        os_spin_unlock_bh(vc->m_lock);
        return NOTIFY_EVENT_VIDEO_CAPTURE_FRAME_CHANGE(nFrameID);
    }
    os_spin_unlock_bh(vc->m_lock);

    return 0;
}

static ULONGLONG _InputNewFieldHandler(struct video_capture *vc);
static ULONGLONG _VFSFullFieldDoneHandler(struct video_capture *vc);
static ULONGLONG _VFSQuarterFrameDoneHandler(struct video_capture *vc);
static ULONGLONG _VFSFullStripeDoneHandler(struct video_capture *vc);
static ULONGLONG _VFSQuarterStripeDoneHandler(struct video_capture *vc);

ULONGLONG video_capture_InterruptHandler(struct video_capture *vc, DWORD dwStatus)
{
    ULONGLONG llNotifyMasks = 0;

    xi_debug(20, "dwStatus:0x%llx\n", dwStatus);

    if (dwStatus & VPP_INT_MASK_INPUT_NEW_FIELD)
        llNotifyMasks |= _InputNewFieldHandler(vc);

    if (dwStatus & VPP_INT_MASK_VFS_FULL_STRIPE_DONE)
        llNotifyMasks |= _VFSFullStripeDoneHandler(vc);

    if (dwStatus & VPP_INT_MASK_VFS_FULL_FIELD_DONE)
        llNotifyMasks |= _VFSFullFieldDoneHandler(vc);

    if (dwStatus & VPP_INT_MASK_VFS_QUARTER_STRIPE_DONE)
        llNotifyMasks |= _VFSQuarterStripeDoneHandler(vc);

    if (dwStatus & VPP_INT_MASK_VFS_QUARTER_FRAME_DONE)
        llNotifyMasks |= _VFSQuarterFrameDoneHandler(vc);

    return llNotifyMasks;
}

static ULONGLONG _InputNewFieldHandler(struct video_capture *vc)
{
    int nFrameID;
    int nField, nFieldIndex;
    BOOLEAN bInfoValid = video_capture_GetInputFrameInfo(
                vc, &nField, &nFieldIndex, &nFrameID);
    LONGLONG llTime = video_capture_GetInputFieldTime(vc);

    if (bInfoValid && nFrameID < (int)vc->m_bufferState.cMaxFrames) {
        VPP_FRAME_CONTEXT * pFrameContext = &vc->m_frameContexts[nFrameID];
        BOOLEAN bStartOfFrame;

        os_spin_lock_bh(vc->m_lock);

        bStartOfFrame = (!vc->m_frameContextLast.pubInfo.bInterlaced
                         || (nFieldIndex == 0));

        vc->m_bufferState.iNewestBuffering = nFrameID;
        vc->m_bufferState.iBufferingFieldIndex = nFieldIndex;

        if (bStartOfFrame) {
            if (pFrameContext->pubInfo.state == MWCAP_VIDEO_FRAME_STATE_BUFFERED)
                vc->m_bufferState.cBufferedFullFrames--;

            os_memcpy(pFrameContext, &vc->m_frameContextLast, sizeof(VPP_FRAME_CONTEXT));
        }

        pFrameContext->pubInfo.state = (nFieldIndex == 0)
            ? MWCAP_VIDEO_FRAME_STATE_F0_BUFFERING
            : MWCAP_VIDEO_FRAME_STATE_F1_BUFFERING;

        pFrameContext->pubInfo.allFieldStartTimes[nFieldIndex] = llTime;

        os_spin_unlock_bh(vc->m_lock);

        return (MWCAP_NOTIFY_VIDEO_FIELD_BUFFERING
                | NOTIFY_EVENT_VIDEO_CAPTURE_FRAME_CHANGE(nFrameID)
                | (bStartOfFrame ? MWCAP_NOTIFY_VIDEO_FRAME_BUFFERING : 0));
    }

    return 0;
}

static ULONGLONG _VFSFullFieldDoneHandler(struct video_capture *vc)
{
    int nFrameID;
    int nField, nFieldIndex;
    BOOL bInfoValid = video_capture_GetVFSFullFrameInfo(
                vc, &nField, &nFieldIndex, &nFrameID);
    LONGLONG llTime = video_capture_GetVFSFullFrameTime(vc);

    xi_debug(20, "bInfoValid:%d,nFrameID:%d,cMaxFrames:%d\n",
        bInfoValid, nFrameID, (int)vc->m_bufferState.cMaxFrames);

    if (bInfoValid && nFrameID < (int)vc->m_bufferState.cMaxFrames) {
        VPP_FRAME_CONTEXT * pFrameContext = &vc->m_frameContexts[nFrameID];
        BOOLEAN bEndOfFrame;

        os_spin_lock_bh(vc->m_lock);

        bEndOfFrame = (!vc->m_frameContextLast.pubInfo.bInterlaced
                       || (nFieldIndex == 1));

        vc->m_bufferState.iNewestBuffered = nFrameID;
        vc->m_bufferState.iBufferedFieldIndex = nFieldIndex;
        pFrameContext->pubInfo.allFieldBufferedTimes[nFieldIndex] = llTime;

        xi_debug(20, "bInfoValid:%d,nFieldIndex:%d\n",
            pFrameContext->pubInfo.bInterlaced, nFieldIndex);

        if (!pFrameContext->pubInfo.bInterlaced || nFieldIndex == 1) {
            vc->m_bufferState.iNewestBufferedFullFrame = nFrameID;
            vc->m_bufferState.cBufferedFullFrames++;

            pFrameContext->pubInfo.state = MWCAP_VIDEO_FRAME_STATE_BUFFERED;
            pFrameContext->cyFullFrameBuffered = pFrameContext->pubInfo.cy;
            pFrameContext->cyTopFieldBuffered = (pFrameContext->pubInfo.cy + 1) / 2;
            pFrameContext->cyBottomFieldBuffered = pFrameContext->pubInfo.cy / 2;
        }
        else
            pFrameContext->pubInfo.state = MWCAP_VIDEO_FRAME_STATE_F1_BUFFERING;

        if (bEndOfFrame) {
            ANC_PACKETS *pFrameANCPackets;

            // bind ACN data to next frame
            vc->m_frameIDForANC = (vc->m_bufferState.iNewestBuffered + 1);
            if (vc->m_frameIDForANC >= vc->m_bufferState.cMaxFrames)
                vc->m_frameIDForANC = 0;

            pFrameANCPackets = &vc->m_frameANCPackets[vc->m_frameIDForANC];
            pFrameANCPackets->cAncPacketCount = 0;
        }

        os_spin_unlock_bh(vc->m_lock);

        return (MWCAP_NOTIFY_VIDEO_FIELD_BUFFERED
                | NOTIFY_EVENT_VIDEO_CAPTURE_FRAME_CHANGE(nFrameID)
                | (bEndOfFrame ? MWCAP_NOTIFY_VIDEO_FRAME_BUFFERED : 0));
    }

    return 0;
}

static ULONGLONG _VFSQuarterFrameDoneHandler(struct video_capture *vc)
{
    int nFrameID;
    BOOL bInfoValid = video_capture_GetVFSQuarterFrameInfo(vc, &nFrameID);

    if (bInfoValid && nFrameID < (int)vc->m_bufferState.cMaxFrames) {
        VPP_FRAME_CONTEXT * pFrameContext = &vc->m_frameContexts[nFrameID];

        os_spin_lock_bh(vc->m_lock);
        pFrameContext->cyQuarterFrameBuffered = pFrameContext->cyQuarterFrame;
        os_spin_unlock_bh(vc->m_lock);

        return (NOTIFY_EVENT_VIDEO_CAPTURE_FRAME_CHANGE(nFrameID));
    }

    return 0;
}

static ULONGLONG _VFSFullStripeDoneHandler(struct video_capture *vc)
{
    int nField, nFieldIndex, nLineID, nFrameID;
    BOOL bInfoValid = video_capture_GetVFSFullStripeInfo(
                vc, &nField, &nFieldIndex, &nLineID, &nFrameID);

    if (bInfoValid && nFrameID < (int)vc->m_bufferState.cMaxFrames) {
        VPP_FRAME_CONTEXT * pFrameContext = &vc->m_frameContexts[nFrameID];

        os_spin_lock_bh(vc->m_lock);

        if (pFrameContext->pubInfo.bInterlaced) {
            if (nField == 0)
                pFrameContext->cyTopFieldBuffered = (nLineID + 1);
            else
                pFrameContext->cyBottomFieldBuffered = (nLineID + 1);

            if (nFieldIndex == 0) {
                // 1st field
                pFrameContext->cyFullFrameBuffered = 0;
            }
            else if (nFieldIndex == 1) {
                // second field
                if (nField == 0) {
                    // 2nd field is top field
                    if (pFrameContext->cyTopFieldBuffered <= pFrameContext->cyBottomFieldBuffered)
                        pFrameContext->cyFullFrameBuffered = pFrameContext->cyTopFieldBuffered * 2;
                    else
                        pFrameContext->cyFullFrameBuffered = pFrameContext->cyTopFieldBuffered * 2 - 1;
                }
                else {
                    // 2nd field is bottom field
                    if (pFrameContext->cyTopFieldBuffered <= pFrameContext->cyBottomFieldBuffered)
                        pFrameContext->cyFullFrameBuffered = pFrameContext->cyBottomFieldBuffered * 2;
                    else
                        pFrameContext->cyFullFrameBuffered = pFrameContext->cyBottomFieldBuffered * 2 + 1;
                }
            }
        }
        else {
            // progressive
            pFrameContext->cyFullFrameBuffered = (nLineID + 1);
        }

        os_spin_unlock_bh(vc->m_lock);

        return (NOTIFY_EVENT_VIDEO_CAPTURE_FRAME_CHANGE(nFrameID));
    }

    return 0;
}

static ULONGLONG _VFSQuarterStripeDoneHandler(struct video_capture *vc)
{
    int nLineID, nFrameID;
    BOOL bInfoValid = video_capture_GetVFSQuarterStripeInfo(
                vc, &nLineID, &nFrameID);

    if (bInfoValid && nFrameID < (int)vc->m_bufferState.cMaxFrames) {
        VPP_FRAME_CONTEXT * pFrameContext = &vc->m_frameContexts[nFrameID];

        os_spin_lock_bh(vc->m_lock);
        pFrameContext->cyQuarterFrameBuffered = (nLineID + 1);
        os_spin_unlock_bh(vc->m_lock);

        return (NOTIFY_EVENT_VIDEO_CAPTURE_FRAME_CHANGE(nFrameID));
    }

    return 0;
}

ULONGLONG video_capture_SMPTETimeCodeHandler(
        struct video_capture *vc,
        const MWCAP_SMPTE_TIMECODE * pTimeCode
        )
{
    int nFrameID;
    int nFieldIndex;

    os_spin_lock_bh(vc->m_lock);

    nFrameID = vc->m_bufferState.iNewestBuffering;
    nFieldIndex = vc->m_bufferState.iBufferingFieldIndex;

    if (nFrameID >= 0 && nFrameID < (int)vc->m_bufferState.cMaxFrames) {
        VPP_FRAME_CONTEXT * pFrameContext = &vc->m_frameContexts[nFrameID];
        os_memcpy(&pFrameContext->pubInfo.aSMPTETimeCodes[nFieldIndex],
               pTimeCode, sizeof(MWCAP_SMPTE_TIMECODE));

        os_spin_unlock_bh(vc->m_lock);

        return MWCAP_NOTIFY_VIDEO_SMPTE_TIME_CODE;
    }
    os_spin_unlock_bh(vc->m_lock);

    return 0;
}

void video_capture_ANCPacketHandler(struct video_capture *vc,
                                    MWCAP_SDI_ANC_PACKET *pPacket)
{
    int nFrameID;

    os_spin_lock_bh(vc->m_lock);

    if (vc->m_frameIDForANC < 0)
        vc->m_frameIDForANC = vc->m_bufferState.iNewestBuffering;

    nFrameID = vc->m_frameIDForANC;

    if (nFrameID >= 0 && nFrameID < (int)vc->m_bufferState.cMaxFrames) {
        ANC_PACKETS *pFrameANCPacket = &vc->m_frameANCPackets[nFrameID];
        if (pFrameANCPacket->cAncPacketCount < MWCAP_MAX_ANC_PACKETS_PER_FRAME)
            os_memcpy(&pFrameANCPacket->arANCPackets[pFrameANCPacket->cAncPacketCount++],
                    pPacket, sizeof(MWCAP_SDI_ANC_PACKET));
    }

    os_spin_unlock_bh(vc->m_lock);
}

int video_capture_GetInputFrameID(struct video_capture *vc)
{
    int nFrameID;
    VPP_FRAME_CONTEXT * pFrameContext;

    os_spin_lock_bh(vc->m_lock);

    nFrameID = vc->m_bufferState.iNewestBuffering;
    pFrameContext = &vc->m_frameContexts[nFrameID];
    if (pFrameContext->pubInfo.state == MWCAP_VIDEO_FRAME_STATE_BUFFERED)
        nFrameID = (nFrameID + 1) % vc->m_bufferState.cMaxFrames;

    os_spin_unlock_bh(vc->m_lock);

    return nFrameID;
}

void video_capture_ResetInput(struct video_capture *vc)
{
    pci_write_reg32(vc->reg_base+REG_ADDR_RESET, 0x01);
}

void video_capture_SetHPosition(struct video_capture *vc, int x)
{
    pci_write_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_INPUT_CLIP_X_CONTROL),
                    x | ((x + vc->m_frameContextLast.pubInfo.cx - 1) << 16));
}

LONGLONG video_capture_GetInputFieldTime(struct video_capture *vc)
{
    DWORD dwHigh = pci_read_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_INPUT_TIMESTAMP_HIGH));
    DWORD dwLow = pci_read_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_INPUT_TIMESTAMP_LOW));

    return ((LONGLONG)dwHigh << 32) | dwLow;
}

BOOLEAN video_capture_GetInputFrameInfo(
        struct video_capture *vc,
        int * pnField,
        int * pnFieldIndex,
        int * pnFrameID
        )
{
    DWORD dwFrameStatus = pci_read_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_INPUT_FRAME_INFO));

    if (pnField) *pnField = ((dwFrameStatus & 0x40000000) == 0) ? 0 : 1;
    if (pnFieldIndex) *pnFieldIndex = ((dwFrameStatus & 0x20000000) == 0) ? 0 : 1;
    if (pnFrameID) *pnFrameID = ((dwFrameStatus >> 16) & 0x000000FF);

    return ((dwFrameStatus & 0x80000000) != 0);
}

LONGLONG video_capture_GetVFSFullFrameTime(struct video_capture *vc)
{
    DWORD dwHigh = pci_read_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_VFS_FULL_TIMESTAMP_HIGH));
    DWORD dwLow = pci_read_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_VFS_FULL_TIMESTAMP_LOW));

    return ((LONGLONG)dwHigh << 32) | dwLow;	
}

BOOLEAN video_capture_GetVFSFullFrameInfo(
        struct video_capture *vc,
        int * pnField,
        int * pnFieldIndex,
        int * pnFrameID)
{
    DWORD dwFrameStatus = pci_read_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_VFS_FULL_FRAME_INFO));

    if (pnField) *pnField = ((dwFrameStatus & 0x40000000) == 0) ? 0 : 1;
    if (pnFieldIndex) *pnFieldIndex = ((dwFrameStatus & 0x20000000) == 0) ? 0 : 1;
    if (pnFrameID) *pnFrameID = ((dwFrameStatus >> 16) & 0x000000FF);

    return ((dwFrameStatus & 0x80000000) != 0);
}

BOOLEAN video_capture_GetVFSFullStripeInfo(
        struct video_capture *vc,
        int * pnField,
        int * pnFieldIndex,
        int * pnLineID,
        int * pnFrameID
        )
{
    DWORD dwStripeStatus = pci_read_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_VFS_FULL_STRIPE_INFO));

    if (pnLineID) *pnLineID = (dwStripeStatus & 0xFFFF);
    if (pnField) *pnField = ((dwStripeStatus & 0x40000000) == 0) ? 0 : 1;
    if (pnFieldIndex) *pnFieldIndex = ((dwStripeStatus & 0x20000000) == 0) ? 0 : 1;
    if (pnFrameID) *pnFrameID = ((dwStripeStatus >> 16) & 0x000000FF);

    return ((dwStripeStatus & 0x80000000) != 0);
}

LONGLONG video_capture_GetVFSQuarterFrameTime(struct video_capture *vc)
{
    DWORD dwHigh = pci_read_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_VFS_QUARTER_TIMESTAMP_HIGH));
    DWORD dwLow = pci_read_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_VFS_QUARTER_TIMESTAMP_LOW));

    return ((LONGLONG)dwHigh << 32) | dwLow;
}

BOOLEAN video_capture_GetVFSQuarterFrameInfo(struct video_capture *vc, int * pnFrameID)
{
    DWORD dwFrameStatus = pci_read_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_VFS_QUARTER_FRAME_INFO));

    if (pnFrameID) *pnFrameID = ((dwFrameStatus >> 16) & 0x000000FF);

    return ((dwFrameStatus & 0x80000000) != 0);
}

BOOLEAN video_capture_GetVFSQuarterStripeInfo(
        struct video_capture *vc,
        int * pnLineID,
        int * pnFrameID
        )
{
    DWORD dwStripeStatus = pci_read_reg32(vc->reg_base+VCAP_CH_REG_ADDR(vc->m_iChannel, REG_ADDR_VFS_QUARTER_STRIPE_INFO));

    if (pnLineID) *pnLineID = (dwStripeStatus & 0xFFFF);
    if (pnFrameID) *pnFrameID = ((dwStripeStatus >> 16) & 0x000000FF);

    return ((dwStripeStatus & 0x80000000) != 0);
}

void video_capture_ReadFrame(
        struct video_capture *vc,
        int iPipeline,
        int iFrame,
        BOOLEAN bFullSize,
        BOOLEAN bBottomUp,
        int x,
        int y,
        int cx,
        int cy,
        BOOLEAN bLastStripe,
        BOOLEAN * pnTopFieldFirst
        )
{
    VPP_FRAME_CONTEXT * pFrameContext = &vc->m_frameContexts[iFrame];

    DWORD dwFrameAddr;
    int cbStride;
    int xRight;

    os_spin_lock_bh(vc->m_lock);
    if (bFullSize) {
        dwFrameAddr = video_capture_GetFullFrameBaseAddr(vc, iFrame);
        cbStride = (int)pFrameContext->cbFullFrameLine;
    }
    else {
        dwFrameAddr = video_capture_GetQuarterFrameBaseAddr(vc, iFrame);
        cbStride = (int)pFrameContext->cbQuarterFrameLine;
    }

    if (pnTopFieldFirst != NULL)
        *pnTopFieldFirst = (pFrameContext->pubInfo.bInterlaced
                            && (y & 1) != 0) ? !pFrameContext->pubInfo.bTopFieldFirst : pFrameContext->pubInfo.bTopFieldFirst;

    os_spin_unlock_bh(vc->m_lock);

    x = x / vc->m_nPixelsPerCycle * vc->m_nPixelsPerCycle;
    xRight = (x + cx + vc->m_nPixelsPerCycle - 1) / vc->m_nPixelsPerCycle * vc->m_nPixelsPerCycle;
    cx = xRight - x;

    if (bBottomUp) {
        dwFrameAddr += (y + cy - 1) * cbStride + x * 4;
        cbStride = -cbStride;
    }
    else
        dwFrameAddr += y * cbStride + x * 4;

    pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_FBRD_ADDRESS), dwFrameAddr);
    pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_FBRD_STRIDE), cbStride);
    pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_FBRD_LINE_CONTROL),
                    (cx * 4) | ((cy - 1) << 16) | (bLastStripe ? 0 : 0x80000000));
    pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_FBRD_CONTROL), 0x01);
}

void video_capture_ReadImage(
        struct video_capture *vc,
        int iPipeline,
        DWORD dwImageAddr,
        int cbStride,
        BOOLEAN bBottomUp,
        int x,
        int y,
        int cx,
        int cy,
        BOOLEAN bLastStripe
        )
{
    int xRight;

    x = x / vc->m_nPixelsPerCycle * vc->m_nPixelsPerCycle;
    xRight = (x + cx + vc->m_nPixelsPerCycle - 1) / vc->m_nPixelsPerCycle * vc->m_nPixelsPerCycle;
    cx = xRight - x;

	if (bBottomUp) {
        dwImageAddr += (y + cy - 1) * cbStride + x * 4;
		cbStride = -cbStride;
	}
	else
        dwImageAddr += y * cbStride + x * 4;

    pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_FBRD_ADDRESS), dwImageAddr);
	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_FBRD_STRIDE), cbStride);
    pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_FBRD_LINE_CONTROL),
                    (cx * 4) | ((cy - 1) << 16) | (bLastStripe ? 0: 0x80000000));
	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_FBRD_CONTROL), 0x01);
}

void video_capture_ReadField(
        struct video_capture *vc,
        int iPipeline,
        int iFrame,
        BOOLEAN bTopField,
        BOOLEAN bBottomUp,
        int x,
        int y,
        int cx,
        int cy,
        BOOLEAN bLastStripe
        )
{
    DWORD dwFrameAddr;
    int cbStride;
    int xRight;
    VPP_FRAME_CONTEXT * pFrameContext = &vc->m_frameContexts[iFrame];

    os_spin_lock_bh(vc->m_lock);
    dwFrameAddr = video_capture_GetFullFrameBaseAddr(vc, iFrame);
    if (!bTopField)
        dwFrameAddr += pFrameContext->cbFullFrameLine;
    cbStride = (int)pFrameContext->cbFullFrameLine * 2;
    os_spin_unlock_bh(vc->m_lock);

    x = x / vc->m_nPixelsPerCycle * vc->m_nPixelsPerCycle;
    xRight = (x + cx + vc->m_nPixelsPerCycle - 1) / vc->m_nPixelsPerCycle * vc->m_nPixelsPerCycle;
    cx = xRight - x;

	if (bBottomUp) {
		dwFrameAddr += (y + cy - 1) * cbStride + x * 4;
		cbStride = -cbStride;
	}
	else
		dwFrameAddr += y * cbStride + x * 4;

	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_FBRD_ADDRESS), dwFrameAddr);
	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_FBRD_STRIDE), cbStride);
    pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_FBRD_LINE_CONTROL),
                    (cx * 4) | ((cy - 1) << 16) | (bLastStripe ? 0 : 0x80000000));
	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_FBRD_CONTROL), 0x01);
}

BOOLEAN video_capture_IsFrameReaderBusy(
        struct video_capture *vc,
        int iPipeline,
        BOOLEAN * pbPendingReadCommand,
        BOOLEAN * pbOSDBusy,
        BOOLEAN * pbMemoryBusy
        )
{
    DWORD dwStatus = pci_read_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_STATUS));

    if (pbPendingReadCommand) *pbPendingReadCommand = ((dwStatus & 0x01) != 0);
    if (pbOSDBusy) *pbOSDBusy = ((dwStatus & 0x04) != 0);
    if (pbMemoryBusy) *pbMemoryBusy = ((dwStatus & 0x08) != 0);

    return ((dwStatus & 0x02) != 0);
}

static int CompareRect(
    const void * pvRect1,
    const void * pvRect2
    )
{
    const RECT * pRect1 = (const RECT *)pvRect1;
    const RECT * pRect2 = (const RECT *)pvRect2;

    return (pRect1->left - pRect2->left);
}

void video_capture_ReadOSDFrame(
        struct video_capture *vc,
        int iPipeline,
        BOOLEAN bBottomUp,
        DWORD dwFrameAddr,
        int cbStride,
        int cy,
        const RECT * prcOSDRects,
        int cOSDRects
        )
{
    DWORD dwOSDRectEnable;
    int i;
    RECT arcOSDRects[4];

    for (i = 0; i < cOSDRects; i++) {
        arcOSDRects[i].left = prcOSDRects[i].left / vc->m_nPixelsPerCycle * vc->m_nPixelsPerCycle;
        arcOSDRects[i].top = prcOSDRects[i].top;
        arcOSDRects[i].right = (prcOSDRects[i].right + vc->m_nPixelsPerCycle - 1) / vc->m_nPixelsPerCycle * vc->m_nPixelsPerCycle - 1;
        arcOSDRects[i].bottom = prcOSDRects[i].bottom - 1;
    }

    qsort(arcOSDRects, cOSDRects, sizeof(RECT), CompareRect);

    if (bBottomUp) {
        dwFrameAddr += (cy - 1) * cbStride;
        cbStride = -cbStride;

        for (i = 0; i < cOSDRects; i++) {
            int32_t lTop = arcOSDRects[i].top;
            int32_t lBottom = arcOSDRects[i].bottom;
            arcOSDRects[i].top = (cy - 1) - lBottom;
            arcOSDRects[i].bottom = (cy - 1) - lTop;
        }
    }

    pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_OSD_ADDRESS), dwFrameAddr);

    if (cOSDRects >= 1) {
        pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_OSD0_X_CONTROL), arcOSDRects[0].left | (arcOSDRects[0].right << 16));
        pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_OSD0_Y_CONTROL), arcOSDRects[0].top | (arcOSDRects[0].bottom << 16));
    }

    if (cOSDRects >= 2) {
        pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_OSD1_X_CONTROL), arcOSDRects[1].left | (arcOSDRects[1].right << 16));
        pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_OSD1_Y_CONTROL), arcOSDRects[1].top | (arcOSDRects[1].bottom << 16));
    }

    if (cOSDRects >= 3) {
        pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_OSD2_X_CONTROL), arcOSDRects[2].left | (arcOSDRects[2].right << 16));
        pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_OSD2_Y_CONTROL), arcOSDRects[2].top | (arcOSDRects[2].bottom << 16));
    }

    if (cOSDRects >= 4) {
        pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_OSD3_X_CONTROL), arcOSDRects[3].left | (arcOSDRects[3].right << 16));
        pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_OSD3_Y_CONTROL), arcOSDRects[3].top | (arcOSDRects[3].bottom << 16));
    }

    dwOSDRectEnable = (1 << cOSDRects) - 1;
    pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_OSD_CONTROL), 0x01 | (dwOSDRectEnable << 4) | (cbStride << 8));
}

void video_capture_EnableWriteBack(struct video_capture *vc, BOOLEAN bEnable)
{
	pci_write_reg32(vc->reg_base+REG_ADDR_VPP_WB_CONTROL, bEnable ? 0x01 : 0x00);
}

void video_capture_WriteFrame(
        struct video_capture *vc,
        DWORD dwFrameAddr,
        int cbStride,
        int cx,
        int cy
	)
{
	pci_write_reg32(vc->reg_base+REG_ADDR_VPP_WB_FBWR_ADDRESS, dwFrameAddr);
	pci_write_reg32(vc->reg_base+REG_ADDR_VPP_WB_FBWR_STRIDE, cbStride);
	pci_write_reg32(vc->reg_base+REG_ADDR_VPP_WB_FBWR_LINE_CONTROL, (cx * 4) | ((cy - 1) << 16));
	pci_write_reg32(vc->reg_base+REG_ADDR_VPP_WB_FBWR_CONTROL, 0x01);
}

void video_capture_StartVideoPipeline(
        struct video_capture *vc,
        int iPipeline,

        // Options
        BOOLEAN b8BitsInput,
        BOOLEAN bDeinterlace,
        BOOLEAN bEnableOSD,
        BOOLEAN bAlphaEnable,
        BOOLEAN bInputSelect,
        BOOLEAN bOutputSelect,
        BOOLEAN bHostVideoBGR,

        // Scale parameters
        int cxSrc,
        int cySrc,

        int xDest,
        int yDest,
        int cxDest,
        int cyDest,

        int cxOut,
        int cyOut,

        // Color adjust parameters
        SHORT sContrast,
        SHORT sBrightness,
        SHORT sSaturation,
        SHORT sHue,

        // Blank & border
        BOOLEAN bBlank,

        WORD wBlankRV,
        WORD wBlankGY,
        WORD wBlankBU,

        // Color space convertion parameters
        DWORD dwFOURCC,

        MWCAP_VIDEO_COLOR_FORMAT cfInput,
        MWCAP_VIDEO_QUANTIZATION_RANGE qrInput,

        SHORT sRVBUScale,
        SHORT sRVBUOffset,

        MWCAP_VIDEO_COLOR_FORMAT cfOutput,
        MWCAP_VIDEO_QUANTIZATION_RANGE qrOutput,
        MWCAP_VIDEO_SATURATION_RANGE srOutput
        )
{
    int xScalePitch;
    int yScalePitch;
    int xInitialOffset;
    int yInitialOffset;
	DWORD dwControlValue;
    int xDestRight;

    if (iPipeline >= NUM_VPPS)
        return;

    xi_debug(20, "cxSrc: %d, cySrc: %d, cxDest: %d, cyDest: %d, cxOut: %d, cyOut: %d, dwFOURCC: %08X\n",
            cxSrc, cySrc, cxDest, cyDest, cxOut, cyOut, dwFOURCC);

    video_capture_UpdateColorMatrix(
                vc,
                iPipeline,

                // Color adjust parameters
                sContrast,
                sBrightness,
                sSaturation,
                sHue,

                // Color space convertion parameters
                cfInput,
                qrInput,

                sRVBUScale,
                sRVBUOffset,

                cfOutput,
                qrOutput,
                srOutput
                );

    xDest = xDest / vc->m_nPixelsPerCycle * vc->m_nPixelsPerCycle;
    xDestRight = (xDest + cxDest + vc->m_nPixelsPerCycle - 1) / vc->m_nPixelsPerCycle * vc->m_nPixelsPerCycle;
    cxDest = xDestRight - xDest;

	// Setup scaler
	xScalePitch = cxSrc * 4096 / cxDest;
	yScalePitch = cySrc * 4096 / cyDest;
	xInitialOffset = cxSrc < cxDest ? 0 : (cxSrc - cxDest) * 2048 / cxDest;
	yInitialOffset = cySrc < cyDest ? 0 : (cySrc - cyDest) * 2048 / cyDest;

	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_SCALER_X_CONTROL), (xScalePitch & 0xFFFFF) | ((xInitialOffset & 0xFFF) << 20));
	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_SCALER_Y_CONTROL), (yScalePitch & 0xFFFFF) | ((yInitialOffset & 0xFFF) << 20));
	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_SCALER_INPUT_SIZE), (cxSrc - 1) | ((cySrc - 1) << 16));

	// Setup border
    _SetVideoBlank(vc, iPipeline, bBlank);
    if (bBlank || cxOut != cxDest || cyOut != cyDest)
        _SetVideoBlankColor(vc, iPipeline, wBlankRV, wBlankGY, wBlankBU);

	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_BORDER_SIZE), ((cyOut - 1) << 16) | (cxOut - 1));
	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_BORDER_X_CONTROL), xDest | ((xDest + cxDest - 1) << 16));
	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_BORDER_Y_CONTROL), yDest | ((yDest + cyDest - 1) << 16));

	dwControlValue = 0x01;
	if (bDeinterlace) dwControlValue |= 0x02;
	if (b8BitsInput) dwControlValue |= 0x04;
	if (bEnableOSD) dwControlValue |= 0x08;
	if (bAlphaEnable) dwControlValue |= 0x10;
	if (bInputSelect) dwControlValue |= 0x20;
	if (bOutputSelect) dwControlValue |= 0x40;
	if (bHostVideoBGR) dwControlValue |= 0x80;
	dwControlValue |= (video_capture_GetPixelFormat(dwFOURCC) << 24);

	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_CONTROL), dwControlValue);
}

void video_capture_StopVideoPipeline(struct video_capture *vc, int iPipeline)
{
	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_CONTROL), 0x00);
}

static void _SetVideoBlank(
        struct video_capture *vc,
        int iPipeline,
        BOOLEAN bBlank
        )
{
    pci_write_reg32(bBlank ? vc->reg_base+REG_ADDR_CONTROL_PORT_SET : vc->reg_base+REG_ADDR_CONTROL_PORT_CLEAR,
            (iPipeline == 0) ? CONTROL_PORT_BIT_VPP1_BLANK : CONTROL_PORT_BIT_VPP2_BLANK);
}

// 16bit color
static void _SetVideoBlankColor(
        struct video_capture *vc,
        int iPipeline,
        WORD wBlankRV,
        WORD wBlankGY,
        WORD wBlankBU
        )
{
    pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_BLANK_CONTROL_1), wBlankRV);
    pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_BLANK_CONTROL_2), (wBlankBU << 16) | wBlankGY);
}

PIXEL_FORMAT video_capture_GetPixelFormat(DWORD dwFOURCC) 
{
    switch (dwFOURCC) {
    // 8bits grey
    case MWFOURCC_GREY:
    case MWFOURCC_Y800:
    case MWFOURCC_Y8:		return PIXEL_FORMAT_Y8;

    case MWFOURCC_Y16:		return PIXEL_FORMAT_Y16;

    // RGB 15-32bits
    case MWFOURCC_RGB15:	return PIXEL_FORMAT_RGB15;
    case MWFOURCC_BGR15:	return PIXEL_FORMAT_BGR15;
    case MWFOURCC_RGB16:	return PIXEL_FORMAT_RGB16;
    case MWFOURCC_BGR16:	return PIXEL_FORMAT_BGR16;
    case MWFOURCC_RGB24:	return PIXEL_FORMAT_RGB24;
    case MWFOURCC_BGR24:	return PIXEL_FORMAT_BGR24;
    case MWFOURCC_RGBA:		return PIXEL_FORMAT_RGBA;
    case MWFOURCC_BGRA:		return PIXEL_FORMAT_BGRA;
    case MWFOURCC_ARGB:		return PIXEL_FORMAT_ARGB;
    case MWFOURCC_ABGR:		return PIXEL_FORMAT_ABGR;

    // Packed YUV 8bits 4:2:2 (16bits)
    case MWFOURCC_YUY2:
    case MWFOURCC_YUYV:		return PIXEL_FORMAT_YUYV;
    case MWFOURCC_YVYU:		return PIXEL_FORMAT_YVYU;
    case MWFOURCC_UYVY:		return PIXEL_FORMAT_UYVY;
    case MWFOURCC_VYUY:		return PIXEL_FORMAT_VYUY;

    // Packed YUV 16bits 4:2:2 (32bits)
    case MWFOURCC_YUYV16:	return PIXEL_FORMAT_YUYV16;
    case MWFOURCC_YVYU16:	return PIXEL_FORMAT_YVYU16;
    case MWFOURCC_UYVY16:	return PIXEL_FORMAT_UYVY16;
    case MWFOURCC_VYUY16:	return PIXEL_FORMAT_VYUY16;

    // Packet YUV 10bits 4:2:2 (24bits)
    case MWFOURCC_V210:		return PIXEL_FORMAT_V210;
    case MWFOURCC_U210:		return PIXEL_FORMAT_U210;

    // Planar YUV 8bits 4:2:0 (12bits)
    case MWFOURCC_I420:
    case MWFOURCC_IYUV:		return PIXEL_FORMAT_I420;
    case MWFOURCC_YV12:		return PIXEL_FORMAT_YV12;
    case MWFOURCC_NV12:		return PIXEL_FORMAT_NV12;
    case MWFOURCC_NV21:		return PIXEL_FORMAT_NV21;

    // Planar YUV 8bits 4:2:2 (16bits)
    case MWFOURCC_I422:
    case MWFOURCC_YV16:		return PIXEL_FORMAT_I422;
    case MWFOURCC_NV16:
    case MWFOURCC_NV61:		return PIXEL_FORMAT_NV16;

   // Planar YUV 10bits 4:2:0 (24bits)
    case MWFOURCC_P010:		return PIXEL_FORMAT_P010;

   // Planar YUV 10bits 4:2:0 (32bits)
    case MWFOURCC_P210:		return PIXEL_FORMAT_P210;


    // Packed YUV 8bits 4:4:4 (24bits)
    case MWFOURCC_IYU2:		return PIXEL_FORMAT_BGR24;
    case MWFOURCC_V308:		return PIXEL_FORMAT_RGB24;

    // Packed YUV 8bits 4:4:4 (32bits)
    case MWFOURCC_AYUV:		return PIXEL_FORMAT_AYUV;
    case MWFOURCC_VYUA:		return PIXEL_FORMAT_RGBA;
    case MWFOURCC_UYVA:
    case MWFOURCC_V408:		return PIXEL_FORMAT_BGRA;

    // Packed YUV 10bits 4:4:4 (32bits)
    case MWFOURCC_Y410:		return PIXEL_FORMAT_BGRA10;
    case MWFOURCC_V410:		return PIXEL_FORMAT_ABGR10;

    // Packed RGB 10bits 4:4:4 (32bits)
    case MWFOURCC_BGR10:	return PIXEL_FORMAT_BGRA10;
    case MWFOURCC_RGB10:	return PIXEL_FORMAT_RGBA10;

    default:				return PIXEL_FORMAT_BGRA;
    }
}

void video_capture_UpdateColorMatrix(
        struct video_capture *vc,
        int iPipeline,

        // Color adjust parameters
        SHORT sContrast,
        SHORT sBrightness,
        SHORT sSaturation,
        SHORT sHue,

        // Color space convertion parameters
        MWCAP_VIDEO_COLOR_FORMAT cfInput,
        MWCAP_VIDEO_QUANTIZATION_RANGE qrInput,

        SHORT sRVBUScale,
        SHORT sRVBUOffset,

        MWCAP_VIDEO_COLOR_FORMAT cfOutput,
        MWCAP_VIDEO_QUANTIZATION_RANGE qrOutput,
        MWCAP_VIDEO_SATURATION_RANGE srOutput
        )
{
	matrix_t matrix;
	int anInput[3];
	int anOutput[3];
	color_matrix_params_t params;

	// Add color adjust steps in reverse order
	clear_matrix(matrix);
    csc_set_conversion(matrix, cfInput, qrInput, cfOutput, qrOutput, COLOR_DEPTH);

    if (cfInput == MWCAP_VIDEO_COLOR_FORMAT_YUV601
            || cfInput == MWCAP_VIDEO_COLOR_FORMAT_YUV709
            || cfInput == MWCAP_VIDEO_COLOR_FORMAT_YUV2020) {
		anInput[0] = COLOR_MATRIX_CH_V;
		anInput[1] = COLOR_MATRIX_CH_Y;
		anInput[2] = COLOR_MATRIX_CH_U;

        yuv_set_sat_hue(matrix, sSaturation, sHue, COLOR_DEPTH);
        yuv_set_contrast(matrix, sContrast, COLOR_DEPTH);
        yuv_set_brightness(matrix, sBrightness);
        yuv_set_scale_offset(matrix, 1, 0,
                             sRVBUScale, sRVBUOffset, sRVBUScale, sRVBUOffset);
	}
	else {
		anInput[0] = COLOR_MATRIX_CH_R;
		anInput[1] = COLOR_MATRIX_CH_G;
		anInput[2] = COLOR_MATRIX_CH_B;

        rgb_set_hue(matrix, sHue);
        rgb_set_saturation(matrix, sSaturation);
        rgb_set_contrast(matrix, sContrast, sContrast, sContrast);
        rgb_set_brightness(matrix, sBrightness, sBrightness, sBrightness);
        rgb_set_scale_offset(matrix, sRVBUScale, sRVBUOffset,
                             1, 0, sRVBUScale, sRVBUOffset);
	}

    if (cfOutput == MWCAP_VIDEO_COLOR_FORMAT_YUV601
            || cfOutput == MWCAP_VIDEO_COLOR_FORMAT_YUV709
            || cfOutput == MWCAP_VIDEO_COLOR_FORMAT_YUV2020) {
		anOutput[0] = COLOR_MATRIX_CH_V;
		anOutput[1] = COLOR_MATRIX_CH_Y;
		anOutput[2] = COLOR_MATRIX_CH_U;
	}
	else {
		anOutput[0] = COLOR_MATRIX_CH_R;
		anOutput[1] = COLOR_MATRIX_CH_G;
		anOutput[2] = COLOR_MATRIX_CH_B;
	}

    get_color_matrix_params(matrix, cfOutput, srOutput, COLOR_DEPTH, 8, anInput, anOutput, &params);
	video_capture_SetupColorMatrix(vc, iPipeline, &params);
}

void video_capture_SetupColorMatrix(
        struct video_capture *vc,
        int iPipeline,
        const color_matrix_params_t * pParams
        )
{
	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_MATRIX_RV_COEFF_1), (pParams->ch0.coeff0 & 0xFFFF) | (pParams->ch0.coeff1 << 16));
	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_MATRIX_RV_COEFF_2), (pParams->ch0.coeff2 & 0xFFFF) | (pParams->ch0.addon << 16));
	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_MATRIX_RV_RANGE), (pParams->ch0.min << (16 - COLOR_DEPTH)) | (pParams->ch0.max << (32 - COLOR_DEPTH)));

	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_MATRIX_GY_COEFF_1), (pParams->ch1.coeff0 & 0xFFFF) | (pParams->ch1.coeff1 << 16));
	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_MATRIX_GY_COEFF_2), (pParams->ch1.coeff2 & 0xFFFF) | (pParams->ch1.addon << 16));
	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_MATRIX_GY_RANGE), (pParams->ch1.min << (16 - COLOR_DEPTH)) | (pParams->ch1.max << (32 - COLOR_DEPTH)));

	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_MATRIX_BU_COEFF_1), (pParams->ch2.coeff0 & 0xFFFF) | (pParams->ch2.coeff1 << 16));
	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_MATRIX_BU_COEFF_2), (pParams->ch2.coeff2 & 0xFFFF) | (pParams->ch2.addon << 16));
	pci_write_reg32(vc->reg_base+VCAP_VPP_REG_ADDR(iPipeline, VPP_REG_OFFSET_MATRIX_BU_RANGE), (pParams->ch2.min << (16 - COLOR_DEPTH)) | (pParams->ch2.max << (32 - COLOR_DEPTH)));
}

int video_capture_GetMaxFrames(struct video_capture *vc)
{
    return vc->m_bufferState.cMaxFrames;
}

void video_capture_GetBufferInfo(struct video_capture *vc,
    MWCAP_VIDEO_BUFFER_INFO * pInfo
    )
{
    os_spin_lock_bh(vc->m_lock);
    os_memcpy(pInfo, &vc->m_bufferState, sizeof(MWCAP_VIDEO_BUFFER_INFO));
    os_spin_unlock_bh(vc->m_lock);
}

BOOLEAN video_capture_GetFrameInfo(
        struct video_capture *vc,
        int iFrame,
        MWCAP_VIDEO_FRAME_INFO * pFrameInfo
    )
{
    if (iFrame < 0 || iFrame >= (int)vc->m_bufferState.cMaxFrames)
        return FALSE;

    os_spin_lock_bh(vc->m_lock);
    os_memcpy(pFrameInfo, &vc->m_frameContexts[iFrame], sizeof(MWCAP_VIDEO_FRAME_INFO));
    os_spin_unlock_bh(vc->m_lock);
    return TRUE;
}

BOOLEAN video_capture_GetFrameContext(
        struct video_capture *vc,
        int iFrame,
        VPP_FRAME_CONTEXT * pFrameContext
    )
{
    if (iFrame < 0 || iFrame >= (int)vc->m_bufferState.cMaxFrames)
        return FALSE;

    os_spin_lock_bh(vc->m_lock);
    os_memcpy(pFrameContext, &vc->m_frameContexts[iFrame], sizeof(VPP_FRAME_CONTEXT));
    os_spin_unlock_bh(vc->m_lock);
    return TRUE;
}

int video_capture_GetSDIANCPackets(
        struct video_capture *vc,
        int iFrame,
        int iCount,
        MWCAP_SDI_ANC_PACKET *pPackets
    )
{
    int iRealCount;

    if (iFrame < 0 ||
        iFrame >= (int)vc->m_bufferState.cMaxFrames ||
        iCount <= 0 ||
        pPackets == NULL)
        return OS_RETURN_INVAL;

    os_spin_lock_bh(vc->m_lock);
    iRealCount = (iCount < vc->m_frameANCPackets[iFrame].cAncPacketCount) ?
                     iCount : vc->m_frameANCPackets[iFrame].cAncPacketCount;
    if (iRealCount > 0)
        os_memcpy(pPackets, vc->m_frameANCPackets[iFrame].arANCPackets,
                  iRealCount * sizeof(MWCAP_SDI_ANC_PACKET));
    os_spin_unlock_bh(vc->m_lock);

    return iRealCount;
}

static void _InitFrameContext(
        VPP_FRAME_CONTEXT * pFrameContext
        )
{
    pFrameContext->pubInfo.state = MWCAP_VIDEO_FRAME_STATE_INITIAL;

    pFrameContext->pubInfo.bInterlaced = FALSE;
    pFrameContext->pubInfo.bSegmentedFrame = FALSE;
    pFrameContext->pubInfo.bTopFieldFirst = TRUE;
    pFrameContext->pubInfo.bTopFieldInverted = FALSE;

    pFrameContext->pubInfo.cx = 720;
    pFrameContext->pubInfo.cy = 480;
    pFrameContext->pubInfo.nAspectX = 4;
    pFrameContext->pubInfo.nAspectY = 3;

    os_memset(&pFrameContext->pubInfo.allFieldStartTimes, 0xFF,
           sizeof(pFrameContext->pubInfo.allFieldStartTimes));
    os_memset(&pFrameContext->pubInfo.allFieldBufferedTimes, 0xFF,
           sizeof(pFrameContext->pubInfo.allFieldBufferedTimes));
    os_memset(&pFrameContext->pubInfo.aSMPTETimeCodes, 0xFF,
           sizeof(pFrameContext->pubInfo.aSMPTETimeCodes));

    pFrameContext->colorFormat = MWCAP_VIDEO_COLOR_FORMAT_RGB;
    pFrameContext->quantRange = MWCAP_VIDEO_QUANTIZATION_FULL;
    pFrameContext->sRVBUScale = 1;
    pFrameContext->sRVBUOffset = 0;

    pFrameContext->cxQuarterFrame = 360;
    pFrameContext->cyQuarterFrame = 240;
    pFrameContext->cbFullFrameLine = 720 * 4;
    pFrameContext->cbQuarterFrameLine = 360 * 4;

    pFrameContext->cyTopFieldBuffered = 0;
    pFrameContext->cyBottomFieldBuffered = 0;
    pFrameContext->cyFullFrameBuffered = 0;
    pFrameContext->cyQuarterFrameBuffered = 0;
}
