////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2017 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#ifndef __SDIANC_H__
#define __SDIANC_H__

#include "ospi/ospi.h"
#include "win-types.h"

struct sdianc {
    volatile void __iomem   *m_reg_base;

    int                     m_nMaxPackets;
    int                     m_nMaxPacketDWCount;
};

void sdianc_init(struct sdianc *pdev, volatile void __iomem *reg_base);

int sdianc_GetMaxPackets(struct sdianc *pdev);

int sdianc_GetPacketDWCount(struct sdianc *pdev);

void sdianc_SetControl(struct sdianc *pdev, BOOLEAN bEnable);

void sdianc_SetANCType(struct sdianc *pdev, int iANC, BOOLEAN bHANC, BOOLEAN bVANC, BYTE byDID, BYTE bySDID);

void sdianc_ClearInterrupts(struct sdianc *pdev);

BOOLEAN sdianc_GetStatus(struct sdianc *pdev, BOOLEAN * pbFull, BOOLEAN * pbNewPacket, BOOLEAN * pbOverflow, int * pnReadId, int * pnCount);

void sdianc_PopPacket(struct sdianc *pdev);

void sdianc_SetAddress(struct sdianc *pdev, int iRead);

DWORD sdianc_ReadPacketData(struct sdianc *pdev);

#endif /* __SDIANC_H__ */
