////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "qspiflash-micron.h"

enum QSPI_CMD {
    QSPI_CMD_JEDEC_ID					= 0x9F,

    QSPI_CMD_READ_DATA					= 0x03,
    QSPI_CMD_FAST_READ					= 0x0B,
    QSPI_CMD_FAST_READ_DUAL_OUTPUT		= 0x3B,
    QSPI_CMD_FAST_READ_DUAL_IO			= 0xBB,
    QSPI_CMD_FAST_READ_QUAD_OUTPUT		= 0x6B,
    QSPI_CMD_FAST_READ_QUAD_IO			= 0xEB,

    QSPI_CMD_WRITE_ENABLE				= 0x06,
    QSPI_CMD_WRITE_DISABLE				= 0x04,

    QSPI_CMD_READ_STATUS_1				= 0x05,
    QSPI_CMD_WRITE_STATUS_1				= 0x01,
    QSPI_CMD_READ_LOCK_STATUS			= 0xE8,
    QSPI_CMD_WRITE_LOCK_STATUS			= 0xE5,
    QSPI_CMD_READ_FLAG_STATUS			= 0x70,
    QSPI_CMD_CLEAR_FLAG_STATUS			= 0x50,
    QSPI_READ_NONVOLATILE_CONFIG		= 0xB5,
    QSPI_WRITE_NONVOLATILE_CONFIG		= 0xB1,
    QSPI_READ_VOLATILE_CONFIG			= 0x85,
    QSPI_WRITE_VOLATILE_CONFIG			= 0x81,
    QSPI_READ_ENH_VOLATILE_CONFIG		= 0x65,
    QSPI_WRITE_ENH_VOLATILE_CONFIG		= 0x61,

    QSPI_CMD_PAGE_PROGRAM				= 0x02,
    QSPI_CMD_FAST_PROGRAM_DUAL_INPUT	= 0xA2,
    QSPI_CMD_FAST_PROGRAM_QUAD_INPUT	= 0x32,

    QSPI_CMD_4K_BLOCK_ERASE				= 0x20,
    QSPI_CMD_64K_BLOCK_ERASE			= 0xD8,
    QSPI_CMD_CHIP_ERASE					= 0xC7,
    QSPI_CMD_PROGRAM_SUSPEND			= 0x75,
    QSPI_CMD_PROGRAM_RESUME				= 0x7A,

    QSPI_CMD_READ_OTP_ARRAY				= 0x4B,
    QSPI_CMD_PROGRAM_OTP_ARRAY			= 0x42
};

enum QSPI_STATUS {
    QSPI_STATUS_1_BUSY					= 0x01,
    QSPI_STATUS_1_WEL					= 0x02,
    QSPI_STATUS_1_BP0					= 0x04,
    QSPI_STATUS_1_BP1					= 0x08,
    QSPI_STATUS_1_BP2					= 0x10,
    QSPI_STATUS_1_TB					= 0x20,
    QSPI_STATUS_1_RESERVED				= 0x40,
    QSPI_STATUS_1_SRP0					= 0x80
};

enum QSPI_WRAP {
    QSPI_WRAP_16_BYTES					= 0,
    QSPI_WRAP_32_BYTES					= 1,
    QSPI_WRAP_64_BYTES					= 2,
    QSPI_WRAP_SEQUENTIAL				= 3
};

enum QSPI_SIZE {
    QSPI_SIZE_PAGE						= 256,
    QSPI_SIZE_4K_BLOCK					= 4096,
    QSPI_SIZE_64K_BLOCK					= 64 * 1024
};

static BOOLEAN _IsEmptyData(const BYTE * pbyData,DWORD cbData);
static VOID _SPIReadBlock(struct qspiflash_micron *pdev, BYTE * pbyData, DWORD cbData);
static VOID _SPIWriteBlock(struct qspiflash_micron *pdev, const BYTE * pbyData, DWORD cbData);
static BYTE _ReadStatus(struct qspiflash_micron *pdev, BYTE byReadStatusCommand);
static VOID _EnableWrite(struct qspiflash_micron *pdev, BOOLEAN bEnable);
static VOID _Erase(struct qspiflash_micron *pdev, BYTE byEraseCommand, DWORD dwAddr);
static VOID _PageProgarm(struct qspiflash_micron *pdev, DWORD dwAddr, const BYTE * pbyData, DWORD cbData);

void qspiflash_micron_init(struct qspiflash_micron *pdev,
                           struct xi_spi_master *pSPIMaster,
                           DWORD dwChipSelectMask)
{
    pdev->m_dwChipSelectMask = dwChipSelectMask;
    pdev->m_pSPIMaster = pSPIMaster;

    pdev->m_bSupportQuadByte = xi_spi_is_support_quadbytemode(pSPIMaster);
    pdev->m_mutex= os_mutex_alloc();
    BUG_ON(pdev->m_mutex == NULL);
}

void qspiflash_micron_deinit(struct qspiflash_micron *pdev)
{
    if (pdev->m_mutex != NULL)
        os_mutex_free(pdev->m_mutex);

    pdev->m_mutex = NULL;
    os_memset(pdev, 0, sizeof(*pdev));
}

DWORD qspiflash_micron_ReadJedecID(struct qspiflash_micron *pdev)
{
    BYTE byManufacturerID, byMemoryTypeID, byCapacityID;

    os_mutex_lock(pdev->m_mutex);
    xi_spi_set_chip_select(pdev->m_pSPIMaster, pdev->m_dwChipSelectMask);
    xi_spi_write(pdev->m_pSPIMaster, QSPI_CMD_JEDEC_ID);
    byManufacturerID = (BYTE)xi_spi_read(pdev->m_pSPIMaster);
    byMemoryTypeID = (BYTE)xi_spi_read(pdev->m_pSPIMaster);
    byCapacityID = (BYTE)xi_spi_read(pdev->m_pSPIMaster);
    xi_spi_clear_chip_select(pdev->m_pSPIMaster, pdev->m_dwChipSelectMask);
    os_mutex_unlock(pdev->m_mutex);

    return ((DWORD)byManufacturerID << 16) | ((DWORD)byMemoryTypeID << 8) | byCapacityID;
}

VOID qspiflash_micron_EraseChip(struct qspiflash_micron *pdev)
{
    os_mutex_lock(pdev->m_mutex);
    _EnableWrite(pdev, TRUE);

    xi_spi_set_chip_select(pdev->m_pSPIMaster, pdev->m_dwChipSelectMask);
    xi_spi_write(pdev->m_pSPIMaster, QSPI_CMD_CHIP_ERASE);
    xi_spi_clear_chip_select(pdev->m_pSPIMaster, pdev->m_dwChipSelectMask);

    while (_ReadStatus(pdev, QSPI_CMD_READ_STATUS_1) & QSPI_STATUS_1_BUSY)
        os_msleep(1);
    os_mutex_unlock(pdev->m_mutex);
}

VOID qspiflash_micron_EraseRange(struct qspiflash_micron *pdev, DWORD dwAddr, DWORD cbData)
{
    DWORD dwEnd = dwAddr + cbData;
    dwAddr = dwAddr & ~(QSPI_SIZE_4K_BLOCK - 1);
    dwEnd = (dwEnd + QSPI_SIZE_4K_BLOCK - 1) & ~(QSPI_SIZE_4K_BLOCK - 1);

    os_mutex_lock(pdev->m_mutex);
    while (dwAddr != dwEnd) {
        BYTE byEraseCommand;
        DWORD cbErase = dwEnd - dwAddr;

        if (cbErase >= QSPI_SIZE_64K_BLOCK && (dwAddr % QSPI_SIZE_64K_BLOCK) == 0) {
            cbErase = QSPI_SIZE_64K_BLOCK;
            byEraseCommand = QSPI_CMD_64K_BLOCK_ERASE;
        }
        else {
            cbErase = QSPI_SIZE_4K_BLOCK;
            byEraseCommand = QSPI_CMD_4K_BLOCK_ERASE;
        }

        _Erase(pdev, byEraseCommand, dwAddr);
        dwAddr += cbErase;
    }
    os_mutex_unlock(pdev->m_mutex);
}

VOID qspiflash_micron_Read(struct qspiflash_micron *pdev, DWORD dwAddr, BYTE * pbyData, DWORD cbData)
{
    os_mutex_lock(pdev->m_mutex);
    xi_spi_set_chip_select(pdev->m_pSPIMaster, pdev->m_dwChipSelectMask);
    if (pdev->m_bSupportQuadByte) {
        xi_spi_write_quadbyte(pdev->m_pSPIMaster,
                                 ((dwAddr & 0xFF) << 24) | ((dwAddr & 0xFF00) << 8) | ((dwAddr & 0xFF0000) >> 8) | QSPI_CMD_READ_DATA);
    }
    else {
        xi_spi_write(pdev->m_pSPIMaster, QSPI_CMD_READ_DATA);
        xi_spi_write(pdev->m_pSPIMaster, dwAddr >> 16);
        xi_spi_write(pdev->m_pSPIMaster, dwAddr >> 8);
        xi_spi_write(pdev->m_pSPIMaster, dwAddr);
    }
    _SPIReadBlock(pdev, pbyData, cbData);
    xi_spi_clear_chip_select(pdev->m_pSPIMaster, pdev->m_dwChipSelectMask);
    os_mutex_unlock(pdev->m_mutex);
}

VOID qspiflash_micron_FastRead(struct qspiflash_micron *pdev,
                               DWORD dwAddr,
                               BYTE byDummyClocks,
                               BYTE * pbyData,
                               DWORD cbData)
{
    os_mutex_lock(pdev->m_mutex);
    xi_spi_set_chip_select(pdev->m_pSPIMaster, pdev->m_dwChipSelectMask);
    if (pdev->m_bSupportQuadByte) {
        xi_spi_write_quadbyte(pdev->m_pSPIMaster,
                                 ((dwAddr & 0xFF) << 24) | ((dwAddr & 0xFF00) << 8) | ((dwAddr & 0xFF0000) >> 8) | QSPI_CMD_FAST_READ);
    }
    else {
        xi_spi_write(pdev->m_pSPIMaster, QSPI_CMD_FAST_READ);
        xi_spi_write(pdev->m_pSPIMaster, dwAddr >> 16);
        xi_spi_write(pdev->m_pSPIMaster, dwAddr >> 8);
        xi_spi_write(pdev->m_pSPIMaster, dwAddr);
    }
    while (byDummyClocks-- != 0)
        xi_spi_write(pdev->m_pSPIMaster, 0);
    _SPIReadBlock(pdev, pbyData, cbData);
    xi_spi_clear_chip_select(pdev->m_pSPIMaster, pdev->m_dwChipSelectMask);
    os_mutex_unlock(pdev->m_mutex);
}

VOID qspiflash_micron_Write(struct qspiflash_micron *pdev,
                            DWORD dwAddr,
                            const BYTE * pbyData,
                            DWORD cbData)
{
    os_mutex_lock(pdev->m_mutex);
    while (cbData != 0) {
        DWORD cbPage = QSPI_SIZE_PAGE - (dwAddr & (QSPI_SIZE_PAGE - 1));
        cbPage = cbPage > cbData ? cbData : cbPage;

        _PageProgarm(pdev, dwAddr, pbyData, cbPage);

        dwAddr += cbPage;
        pbyData += cbPage;
        cbData -= cbPage;
    }
    os_mutex_unlock(pdev->m_mutex);
}

BOOLEAN qspiflash_micron_GetStorageInfo(struct qspiflash_micron *pdev,
                                        DWORD * pcbStorage,
                                        DWORD * pcbEraseBlock,
                                        DWORD * pcbProgramBlock
                                        )
{
    if (pcbStorage) {
        switch (qspiflash_micron_ReadJedecID(pdev) & 0xFF) {
        case 0x15:
            *pcbStorage = 16 * 1024 * 1024 / 8;
            break;
        case 0x16:
            *pcbStorage = 32 * 1024 * 1024 / 8;
            break;
        case 0x17:
            *pcbStorage = 64 * 1024 * 1024 / 8;
            break;
        case 0x18:
            *pcbStorage = 128 * 1024 * 1024 / 8;
            break;
        case 0x19:
            *pcbStorage = 256 * 1024 * 1024 / 8;
            break;
        default:
            *pcbStorage = 32 * 1024 * 1024 / 8;
            return FALSE;
        }
    }

    if (pcbEraseBlock) *pcbEraseBlock = QSPI_SIZE_4K_BLOCK;
    if (pcbProgramBlock) *pcbProgramBlock = QSPI_SIZE_PAGE;
    return TRUE;
}

/* local functions */
static BOOLEAN _IsEmptyData(const BYTE * pbyData, DWORD cbData)
{
    BOOLEAN bIsEmpty = TRUE;
    while (cbData--) {
        if (*pbyData++ != 0xFF) {
            bIsEmpty = FALSE;
            break;
        }
    }

    return bIsEmpty;
}

static VOID _SPIReadBlock(struct qspiflash_micron *pdev, BYTE * pbyData, DWORD cbData)
{
    if (pdev->m_bSupportQuadByte) {
        DWORD * pdwData = (DWORD *)pbyData;
        DWORD cdwData = cbData / 4;
        cbData = cbData & 3;

        while (cdwData--)
            *pdwData++ = xi_spi_read_quadbyte(pdev->m_pSPIMaster);

        pbyData = (BYTE *)pdwData;
    }

    while (cbData--)
        *pbyData++ = (BYTE)xi_spi_read(pdev->m_pSPIMaster);
}

static VOID _SPIWriteBlock(struct qspiflash_micron *pdev, const BYTE * pbyData, DWORD cbData)
{
    if (pdev->m_bSupportQuadByte) {
        DWORD * pdwData = (DWORD *)pbyData;
        DWORD cdwData = cbData / 4;
        cbData = cbData & 3;

        while (cdwData--)
            xi_spi_write_quadbyte(pdev->m_pSPIMaster, *pdwData++);

        pbyData = (BYTE *)pdwData;
    }

    while (cbData--)
        xi_spi_write(pdev->m_pSPIMaster, *pbyData++);
}

static BYTE _ReadStatus(struct qspiflash_micron *pdev, BYTE byReadStatusCommand)
{
    BYTE byStatus;

    xi_spi_set_chip_select(pdev->m_pSPIMaster, pdev->m_dwChipSelectMask);
    xi_spi_write(pdev->m_pSPIMaster, byReadStatusCommand);
    byStatus = (BYTE)xi_spi_read(pdev->m_pSPIMaster);
    xi_spi_clear_chip_select(pdev->m_pSPIMaster, pdev->m_dwChipSelectMask);

    return byStatus;
}

static VOID _EnableWrite(struct qspiflash_micron *pdev, BOOLEAN bEnable)
{
    xi_spi_set_chip_select(pdev->m_pSPIMaster, pdev->m_dwChipSelectMask);
    xi_spi_write(pdev->m_pSPIMaster, bEnable ? QSPI_CMD_WRITE_ENABLE : QSPI_CMD_WRITE_DISABLE);
    xi_spi_clear_chip_select(pdev->m_pSPIMaster, pdev->m_dwChipSelectMask);
}

static VOID _Erase(struct qspiflash_micron *pdev, BYTE byEraseCommand, DWORD dwAddr)
{
    _EnableWrite(pdev, TRUE);

    xi_spi_set_chip_select(pdev->m_pSPIMaster, pdev->m_dwChipSelectMask);
    if (pdev->m_bSupportQuadByte) {
        xi_spi_write_quadbyte(pdev->m_pSPIMaster,
                                 ((dwAddr & 0xFF) << 24) | ((dwAddr & 0xFF00) << 8) | ((dwAddr & 0xFF0000) >> 8) | byEraseCommand);
    }
    else {
        xi_spi_write(pdev->m_pSPIMaster, byEraseCommand);
        xi_spi_write(pdev->m_pSPIMaster, dwAddr >> 16);
        xi_spi_write(pdev->m_pSPIMaster, dwAddr >> 8);
        xi_spi_write(pdev->m_pSPIMaster, dwAddr);
    }
    xi_spi_clear_chip_select(pdev->m_pSPIMaster, pdev->m_dwChipSelectMask);

    while (_ReadStatus(pdev, QSPI_CMD_READ_STATUS_1) & QSPI_STATUS_1_BUSY)
        os_msleep(1);
}

static VOID _PageProgarm(struct qspiflash_micron *pdev, DWORD dwAddr, const BYTE * pbyData, DWORD cbData)
{
    if (_IsEmptyData(pbyData, cbData))
        return;

    _EnableWrite(pdev, TRUE);

    xi_spi_set_chip_select(pdev->m_pSPIMaster, pdev->m_dwChipSelectMask);
    if (pdev->m_bSupportQuadByte) {
        xi_spi_write_quadbyte(pdev->m_pSPIMaster,
                                 ((dwAddr & 0xFF) << 24) | ((dwAddr & 0xFF00) << 8) | ((dwAddr & 0xFF0000) >> 8) | QSPI_CMD_PAGE_PROGRAM);
    }
    else {
        xi_spi_write(pdev->m_pSPIMaster, QSPI_CMD_PAGE_PROGRAM);
        xi_spi_write(pdev->m_pSPIMaster, dwAddr >> 16);
        xi_spi_write(pdev->m_pSPIMaster, dwAddr >> 8);
        xi_spi_write(pdev->m_pSPIMaster, dwAddr);
    }
    _SPIWriteBlock(pdev, pbyData, cbData);
    xi_spi_clear_chip_select(pdev->m_pSPIMaster, pdev->m_dwChipSelectMask);

    while (_ReadStatus(pdev, QSPI_CMD_READ_STATUS_1) & QSPI_STATUS_1_BUSY)
        os_msleep(1);
}
