////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#ifndef __VIDEO_CAPTURE_H__
#define __VIDEO_CAPTURE_H__

#include "ospi/ospi.h"
#include "win-types.h"
#include "mw-procapture-extension.h"
#include "mw-fourcc.h"
#include "supports/color-matrix.h"

#define VCAP_CH_REG_ADDR(i, off)			(((i) == 0 ? 0 : 512) + (i) * 128 + (off))
#define VCAP_VPP_REG_ADDR(i, off)			(((i) == 0 ? REG_ADDR_VPP1_BASE : REG_ADDR_VPP2_BASE) + (off))

#define NUM_VPPS							2

enum VPP_INT_MASK {
    VPP_INT_MASK_INPUT_NEW_FIELD			= 1 << 0,
    VPP_INT_MASK_INPUT_LOST_SYNC			= 1 << 1,
    VPP_INT_MASK_VFS_OVERFLOW				= 1 << 2,
    VPP_INT_MASK_VFS_FULL_FIELD_DONE		= 1 << 3,
    VPP_INT_MASK_VFS_FULL_STRIPE_DONE		= 1 << 4,
    VPP_INT_MASK_VFS_QUARTER_FRAME_DONE		= 1 << 5,
    VPP_INT_MASK_VFS_QUARTER_STRIPE_DONE	= 1 << 6,
    VPP_INT_MASK_VPP1_FBRD_DONE				= 1 << 7,
    VPP_INT_MASK_VPP2_FBRD_DONE				= 1 << 8,
    VPP_INT_MASK_VPP_WB_FBWR_DONE			= 1 << 9
};

enum VPP_CAPS {
    VPP_CAPS_VPP2							= 0x0001,
    VPP_CAPS_GREY							= 0x0002,
    VPP_CAPS_RGB5x5							= 0x0004,
    VPP_CAPS_PLANAR							= 0x0008,
    VPP_CAPS_OSD_DISABLED					= 0x0010,
    VPP_CAPS_QUARTER_FB_DISABLED			= 0x0020
};

typedef enum _PIXEL_FORMAT {
    PIXEL_FORMAT_Y8						= 0x00,
    PIXEL_FORMAT_Y16					= 0x01,

    PIXEL_FORMAT_RGB15					= 0x20,
    PIXEL_FORMAT_RGB16					= 0x21,
    PIXEL_FORMAT_RGB24					= 0x22,
    PIXEL_FORMAT_RGBA   				= 0x23,
    PIXEL_FORMAT_ARGB					= 0x24,
    PIXEL_FORMAT_YUYV					= 0x25,
    PIXEL_FORMAT_UYVY					= 0x26,
    PIXEL_FORMAT_AYUV					= 0x27,
    PIXEL_FORMAT_RGBA10					= 0x28,
    PIXEL_FORMAT_ARGB10					= 0x29,
    PIXEL_FORMAT_YUYV16					= 0x2A,
    PIXEL_FORMAT_UYVY16					= 0x2B,
    PIXEL_FORMAT_V210					= 0x2C,

    PIXEL_FORMAT_BGR15					= 0xA0,
    PIXEL_FORMAT_BGR16					= 0xA1,
    PIXEL_FORMAT_BGR24  				= 0xA2,
    PIXEL_FORMAT_BGRA   				= 0xA3,
    PIXEL_FORMAT_ABGR					= 0xA4,
    PIXEL_FORMAT_YVYU					= 0xA5,
    PIXEL_FORMAT_VYUY					= 0xA6,
    PIXEL_FORMAT_AYVU					= 0xA7,
    PIXEL_FORMAT_BGRA10					= 0xA8,
    PIXEL_FORMAT_ABGR10					= 0xA9,
    PIXEL_FORMAT_YVYU16					= 0xAA,
    PIXEL_FORMAT_VYUY16					= 0xAB,
    PIXEL_FORMAT_U210					= 0xAC,

    PIXEL_FORMAT_I420					= 0x40,
    PIXEL_FORMAT_NV12					= 0x41,
    PIXEL_FORMAT_I422					= 0x42,
    PIXEL_FORMAT_NV16					= 0X43,
    PIXEL_FORMAT_P010					= 0x45,
    PIXEL_FORMAT_P210					= 0x47,

    PIXEL_FORMAT_YV12					= 0xC0,
    PIXEL_FORMAT_NV21					= 0xC1,
    PIXEL_FORMAT_YV16					= 0xC2,
    PIXEL_FORMAT_NV61					= 0XC3
} PIXEL_FORMAT;

typedef struct _VPP_FRAME_CONTEXT {
    MWCAP_VIDEO_FRAME_INFO          pubInfo;

    MWCAP_VIDEO_COLOR_FORMAT		colorFormat;
    MWCAP_VIDEO_QUANTIZATION_RANGE	quantRange;
    SHORT							sRVBUScale;
    SHORT							sRVBUOffset;

    int								cxQuarterFrame;
    int								cyQuarterFrame;
    DWORD							cbFullFrameLine;
    DWORD							cbQuarterFrameLine;

    int								cyTopFieldBuffered;
    int								cyBottomFieldBuffered;
    int								cyFullFrameBuffered;
    int								cyQuarterFrameBuffered;
} VPP_FRAME_CONTEXT;

typedef struct _ANC_PACKETS {
    int                             cAncPacketCount;
    MWCAP_SDI_ANC_PACKET            arANCPackets[MWCAP_MAX_ANC_PACKETS_PER_FRAME];
} ANC_PACKETS;

struct video_capture {
    volatile void __iomem *reg_base;

    int                                 m_iChannel;

    // Frame buffer information
    DWORD                               m_dwFullFrameBufferAddr;
    DWORD                               m_cbFullFrame;
    DWORD                               m_dwQuarterFrameBufferAddr;
    DWORD                               m_cbQuarterFrame;

    int                                 m_nPixelsPerCycle;

    // Frame buffer state
    os_spinlock_t                       m_lock;

    MWCAP_VIDEO_BUFFER_INFO             m_bufferState;
    VPP_FRAME_CONTEXT                   m_frameContextLast;
    VPP_FRAME_CONTEXT                   m_frameContexts[MWCAP_MAX_VIDEO_FRAME_COUNT];
    int                                 m_frameIDForANC;
    ANC_PACKETS                         m_frameANCPackets[MWCAP_MAX_VIDEO_FRAME_COUNT];
};

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
        );

void video_capture_Deinitialize(struct video_capture *vc);

DWORD video_capture_GetFullFrameBaseAddr(
        struct video_capture *vc,
        int iFrameID
        );

DWORD video_capture_GetQuarterFrameBaseAddr(
        struct video_capture *vc,
        int iFrameID
        );

void video_capture_SetIntEnables(
        struct video_capture *vc,
        DWORD dwEnableBits
        );

DWORD video_capture_GetCaps(struct video_capture *vc);

DWORD video_capture_GetIntStatus(struct video_capture *vc);

DWORD video_capture_GetIntRawStatus(struct video_capture *vc);

void video_capture_ClearIntRawStatus(struct video_capture *vc, DWORD dwClearBits);

void video_capture_SetControlPortValue(struct video_capture *vc, DWORD dwValue);

void video_capture_SetControlPortBits(struct video_capture *vc, DWORD dwBits);

void video_capture_ClearControlPortBits(struct video_capture *vc, DWORD dwBits);

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
        );

ULONGLONG video_capture_StopInput(struct video_capture *vc);

ULONGLONG video_capture_InterruptHandler(struct video_capture *vc, DWORD dwStatus);

ULONGLONG video_capture_SMPTETimeCodeHandler(
        struct video_capture *vc,
        const MWCAP_SMPTE_TIMECODE * pTimeCode
        );

void video_capture_ANCPacketHandler(struct video_capture *vc,
                                    MWCAP_SDI_ANC_PACKET *pPacket);

int video_capture_GetInputFrameID(struct video_capture *vc);

void video_capture_ResetInput(struct video_capture *vc);

BOOLEAN video_capture_IsInputMemoryBusy(struct video_capture *vc);

void video_capture_SetHPosition(struct video_capture *vc, int x);

LONGLONG video_capture_GetInputFieldTime(struct video_capture *vc);

BOOLEAN video_capture_GetInputFrameInfo(
        struct video_capture *vc,
        int * pnField,
        int * pnFieldIndex,
        int * pnFrameID
        );

LONGLONG video_capture_GetVFSFullFrameTime(struct video_capture *vc);

BOOLEAN video_capture_GetVFSFullFrameInfo(
        struct video_capture *vc,
        int * pnField,
        int * pnFieldIndex,
        int * pnFrameID);

BOOLEAN video_capture_GetVFSFullStripeInfo(
        struct video_capture *vc,
        int * pnField,
        int * pnFieldIndex,
        int * pnLineID,
        int * pnFrameID
        );

LONGLONG video_capture_GetVFSQuarterFrameTime(struct video_capture *vc);

BOOLEAN video_capture_GetVFSQuarterFrameInfo(struct video_capture *vc, int * pnFrameID);

BOOLEAN video_capture_GetVFSQuarterStripeInfo(
        struct video_capture *vc,
        int * pnLineID,
        int * pnFrameID
        );

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
        );

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
        );

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
        );

BOOLEAN video_capture_IsFrameReaderBusy(
        struct video_capture *vc,
        int iPipeline,
        BOOLEAN * pbPendingReadCommand,
        BOOLEAN * pbOSDBusy,
        BOOLEAN * pbMemoryBusy
        );

void video_capture_ReadOSDFrame(
        struct video_capture *vc,
        int iPipeline,
        BOOLEAN bBottomUp,
        DWORD dwFrameAddr,
        int cbStride,
        int cy,
        const RECT * prcOSDRects,
        int cOSDRects
        );

void video_capture_EnableWriteBack(struct video_capture *vc, BOOLEAN bEnable);

void video_capture_WriteFrame(
        struct video_capture *vc,
        DWORD dwFrameAddr,
        int cbStride,
        int cx,
        int cy
	);

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
        );

void video_capture_StopVideoPipeline(struct video_capture *vc, int iPipeline);

PIXEL_FORMAT video_capture_GetPixelFormat(DWORD dwFOURCC);

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
        MWCAP_VIDEO_SATURATION_RANGE srOutput);

void video_capture_SetupColorMatrix(
        struct video_capture *vc,
        int iPipeline,
        const color_matrix_params_t * pParams
        );


int video_capture_GetMaxFrames(struct video_capture *vc);

void video_capture_GetBufferInfo(struct video_capture *vc,
    MWCAP_VIDEO_BUFFER_INFO * pInfo
    );

BOOLEAN video_capture_GetFrameInfo(
        struct video_capture *vc,
        int iFrame,
        MWCAP_VIDEO_FRAME_INFO * pFrameInfo
    );

BOOLEAN video_capture_GetFrameContext(
        struct video_capture *vc,
        int iFrame,
        VPP_FRAME_CONTEXT * pFrameContext
    );

int video_capture_GetSDIANCPackets(
        struct video_capture *vc,
        int iFrame,
        int iCount,
        MWCAP_SDI_ANC_PACKET *pPackets
    );

#endif /* __VIDEO_CAPTURE_H__ */
