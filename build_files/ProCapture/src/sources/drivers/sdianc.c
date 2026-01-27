////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2017 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "sdianc.h"

enum REG_ADDR {
    REG_VER_CAPS	= 0 * 4,
    REG_CONTROL		= 1 * 4,
    REG_STATUS		= 2 * 4,
    REG_ANC_1_TYPE	= 3 * 4,
    REG_ANC_2_TYPE  = 4 * 4,
    REG_ANC_3_TYPE  = 5 * 4,
    REG_ANC_4_TYPE  = 6 * 4,
    REG_READ_ADDR	= 7 * 4,
    REG_READ_DATA	= 7 * 4
};

static void _GetCaps(struct sdianc *pdev, int * pnMaxPackets, int * pnMaxPacketDWCount)
{
    DWORD dwData = pci_read_reg32(pdev->m_reg_base+REG_VER_CAPS);
    if (pnMaxPackets) *pnMaxPackets = (dwData & 0xFF);
    if (pnMaxPacketDWCount) *pnMaxPacketDWCount = (dwData >> 8) & 0xFF;
}

void sdianc_init(struct sdianc *pdev, volatile void __iomem *reg_base)
{
    pdev->m_reg_base = reg_base;
    _GetCaps(pdev, &pdev->m_nMaxPackets, &pdev->m_nMaxPacketDWCount);
}

int sdianc_GetMaxPackets(struct sdianc *pdev)
{
    return pdev->m_nMaxPackets;
}

int sdianc_GetPacketDWCount(struct sdianc *pdev)
{
    return pdev->m_nMaxPacketDWCount;
}

void sdianc_SetControl(struct sdianc *pdev, BOOLEAN bEnable)
{
    pci_write_reg32(pdev->m_reg_base+REG_CONTROL, (bEnable ? 0x01 : 0x00));
}

void sdianc_SetANCType(struct sdianc *pdev, int iANC, BOOLEAN bHANC, BOOLEAN bVANC, BYTE byDID, BYTE bySDID)
{
    pci_write_reg32(pdev->m_reg_base+REG_ANC_1_TYPE + iANC * 4, byDID | (bySDID << 8) | (bHANC ? (1 << 17) : 0) | (bVANC ? (1 << 16) : 0));
}

void sdianc_ClearInterrupts(struct sdianc *pdev)
{
    pci_write_reg32(pdev->m_reg_base+REG_STATUS, 0x0A);
}

BOOLEAN sdianc_GetStatus(struct sdianc *pdev, BOOLEAN * pbFull, BOOLEAN * pbNewPacket, BOOLEAN * pbOverflow, int * pnReadId, int * pnCount)
{
    DWORD dwData = pci_read_reg32(pdev->m_reg_base+REG_STATUS);
    if (pbFull) *pbFull = (dwData & 0x04) != 0;
    if (pbNewPacket) *pbNewPacket = (dwData & 0x02) != 0;
    if (pbOverflow) *pbOverflow = (dwData & 0x08) != 0;
    if (pnReadId) *pnReadId = (dwData >> 28);
    if (pnCount) *pnCount = (dwData >> 24) & 0x0F;
    return (dwData & 0x01) == 0;
}

void sdianc_PopPacket(struct sdianc *pdev)
{
    pci_write_reg32(pdev->m_reg_base+REG_CONTROL, (~0x02 << 16) | 0x02);
}

void sdianc_SetAddress(struct sdianc *pdev, int iRead)
{
    pci_write_reg32(pdev->m_reg_base+REG_READ_ADDR, iRead * 65);
}

DWORD sdianc_ReadPacketData(struct sdianc *pdev)
{
    return pci_read_reg32(pdev->m_reg_base+REG_READ_DATA);
}
