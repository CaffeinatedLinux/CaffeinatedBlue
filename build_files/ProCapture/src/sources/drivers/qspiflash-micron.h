////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#ifndef __QSPIFLASH_MICRON_H__
#define __QSPIFLASH_MICRON_H__

#include "spi-master.h"
#include "win-types.h"

struct qspiflash_micron {
    struct xi_spi_master *  m_pSPIMaster;
    DWORD                   m_dwChipSelectMask;
    BOOL                    m_bSupportQuadByte;
    os_mutex_t              m_mutex;
};

void qspiflash_micron_init(struct qspiflash_micron *pdev,
                           struct xi_spi_master *pSPIMaster,
                           DWORD dwChipSelectMask);

void qspiflash_micron_deinit(struct qspiflash_micron *pdev);

DWORD qspiflash_micron_ReadJedecID(struct qspiflash_micron *pdev);

VOID qspiflash_micron_EraseChip(struct qspiflash_micron *pdev);

VOID qspiflash_micron_EraseRange(struct qspiflash_micron *pdev, DWORD dwAddr, DWORD cbData);

VOID qspiflash_micron_Read(struct qspiflash_micron *pdev, DWORD dwAddr, BYTE * pbyData, DWORD cbData);

VOID qspiflash_micron_FastRead(struct qspiflash_micron *pdev,
                               DWORD dwAddr,
                               BYTE byDummyClocks,
                               BYTE * pbyData,
                               DWORD cbData);

VOID qspiflash_micron_Write(struct qspiflash_micron *pdev,
                            DWORD dwAddr,
                            const BYTE * pbyData,
                            DWORD cbData);

BOOLEAN qspiflash_micron_GetStorageInfo(struct qspiflash_micron *pdev,
                                        DWORD * pcbStorage,
                                        DWORD * pcbEraseBlock,
                                        DWORD * pcbProgramBlock
                                        );

#endif /* __QSPIFLASH_MICRON_H__ */
