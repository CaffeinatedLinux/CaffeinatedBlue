////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "pi7c9x2gxxx.h"
#include "2g308gp.h"
#include "2g608gp.h"

static int _read_eeprom_status(struct mw_pi7c9x2gxxx *pobj, WORD *pwStatus);
static int _read_eeprom_data(struct mw_pi7c9x2gxxx *pobj, WORD wAddress, WORD *pwData);
static int _read_eeprom_data_block(struct mw_pi7c9x2gxxx *pobj,
                                   BYTE * pbyBlock, int cbBlock);
static int _enable_eeprom_write(struct mw_pi7c9x2gxxx *pobj);
static int _disable_eeprom_write(struct mw_pi7c9x2gxxx *pobj);
static int _write_eeprom_data(struct mw_pi7c9x2gxxx *pobj, WORD wAddress, WORD wData);
static int _write_eeprom_data_block(struct mw_pi7c9x2gxxx *pobj, const BYTE * pbyBlock, int cbBlock);

void mw_pi7c9x2gxxx_init(struct mw_pi7c9x2gxxx *pobj,
                         os_pci_dev_t pci_dev,
                         os_pci_dev_t par_dev)
{
    DWORD dwValue;
    int ret;

    ret = os_pci_read_config_dword(pci_dev, 0, &dwValue);
    if (ret == OS_RETURN_SUCCESS && (dwValue == 0x260812D8 || dwValue == 0x230812D8)) {
        pobj->m_wDeviceID = (dwValue >> 16);
        pobj->pci_dev = pci_dev;
    }

    ret = os_pci_read_config_dword(par_dev, 0, &dwValue);
    if (ret == OS_RETURN_SUCCESS && (dwValue == 0x260812D8 || dwValue == 0x230812D8)) {
        pobj->par_dev = par_dev;
    }
}

int mw_pi7c9x2gxxx_acs_bugfix(struct mw_pi7c9x2gxxx *pobj)
{
    unsigned int value;
    int ret;

    if (pobj->pci_dev == NULL)
        return OS_RETURN_SUCCESS;

    ret = os_pci_read_config_dword(pobj->pci_dev, 0x224, &value);
    if (ret != OS_RETURN_SUCCESS)
        return ret;

    if ((value >> 16) != 0) {
        xi_debug(1, "ACS CAPs value: %08X, Apply ACS hotfix!\n", value);
        value = 0;
        return os_pci_write_config_dword(pobj->pci_dev, 0x224, value);
    }

    return OS_RETURN_SUCCESS;
}

BOOLEAN mw_pi7c9x2gxxx_is_eeprom_present(struct mw_pi7c9x2gxxx *pobj)
{
    WORD wData = 0;
    int ret;

    ret = _read_eeprom_status(pobj, &wData);
    if (ret != OS_RETURN_SUCCESS)
        return FALSE;

    if ((wData & 0xFFFF) != 0xFFFF)
        return TRUE;

    return FALSE;
}

int mw_pi7c9x2gxxx_init_eeprom_content(struct mw_pi7c9x2gxxx *pobj)
{
    const BYTE * pbyBlock;
    int cbBlock;
    int ret;
    int i;
    BYTE abyData[max(ARRAY_SIZE(g_aby2G308GP), ARRAY_SIZE(g_aby2G608GP))];

    if (!mw_pi7c9x2gxxx_is_eeprom_present(pobj)) {
        xi_debug(0, "EEPROM not present\n");
        return OS_RETURN_ERROR;
    }

    switch (pobj->m_wDeviceID) {
    case 0x2308:
        pbyBlock = g_aby2G308GP;
        cbBlock = ARRAY_SIZE(g_aby2G308GP);
        break;

    case 0x2608:
        pbyBlock = g_aby2G608GP;
        cbBlock = ARRAY_SIZE(g_aby2G608GP);
        break;

    default:
        return OS_RETURN_ERROR;
    }

    ret = _read_eeprom_data_block(pobj, abyData, cbBlock);
    if (ret != OS_RETURN_SUCCESS) {
        xi_debug(0, "Read EEPROM failed\n");
        return ret;
    }

    if (os_memcmp(abyData, pbyBlock, cbBlock) == 0) {
        xi_debug(0, "EEPROM up to date\n");
        return OS_RETURN_SUCCESS;
    }

    for (i = 0; i < 3; i++) {
        ret = _write_eeprom_data_block(pobj, pbyBlock, cbBlock);
        if (ret != OS_RETURN_SUCCESS) {
            xi_debug(0, "Write EEPROM failed\n");
            continue;
        }

        ret = _read_eeprom_data_block(pobj, abyData, cbBlock);
        if (ret != OS_RETURN_SUCCESS) {
            xi_debug(0, "Read EEPROM failed\n");
            continue;
        }

        if (os_memcmp(abyData, pbyBlock, cbBlock) != 0) {
            xi_debug(0, "Verify EEPROM failed!\n");
            ret = OS_RETURN_ERROR;
            continue;
        }

        ret = OS_RETURN_SUCCESS;
    }

    return ret;
}

static int _read_eeprom_status(struct mw_pi7c9x2gxxx *pobj, WORD *pwStatus)
{
    DWORD dwValue = (0x01 | (5 << 1) | (1 << 5) | (1 << 6));
    int ret;

    if (pobj->par_dev == NULL)
        return OS_RETURN_ERROR;

    ret = os_pci_write_config_dword(pobj->par_dev, 0xA0, dwValue);
    if (ret != OS_RETURN_SUCCESS)
        return ret;

    os_msleep(1);

    ret = os_pci_read_config_dword(pobj->par_dev, 0xA4, &dwValue);
    if (ret != OS_RETURN_SUCCESS)
        return ret;

    if (pwStatus != NULL)
        *pwStatus = (dwValue >> 16);

    return OS_RETURN_SUCCESS;
}

static int _read_eeprom_data(struct mw_pi7c9x2gxxx *pobj, WORD wAddress, WORD *pwData)
{
    int ret;
    DWORD dwValue;

    if (pobj->par_dev == NULL)
        return OS_RETURN_ERROR;

    dwValue = wAddress;
    ret = os_pci_write_config_dword(pobj->par_dev, 0xA4, dwValue);
    if (ret != OS_RETURN_SUCCESS)
        return ret;

    dwValue = (0x01 | (3 << 1) | (1 << 5) | (1 << 6));
    ret = os_pci_write_config_dword(pobj->par_dev, 0xA0, dwValue);
    if (ret != OS_RETURN_SUCCESS)
        return ret;

    os_msleep(1);

    ret = os_pci_read_config_dword(pobj->par_dev, 0xA4, &dwValue);
    if (ret != OS_RETURN_SUCCESS)
        return ret;

    if (pwData != NULL)
        *pwData = (dwValue >> 16);

    return OS_RETURN_SUCCESS;
}

static int _read_eeprom_data_block(struct mw_pi7c9x2gxxx *pobj,
                                   BYTE * pbyBlock, int cbBlock)
{
    WORD * pwBlock = (WORD *)pbyBlock;
    int cWords = cbBlock / sizeof(WORD);
    WORD wAddress = 0;

    while (cWords != 0) {
        int ret = _read_eeprom_data(pobj, wAddress, pwBlock);
        if (ret != OS_RETURN_SUCCESS)
            return ret;

        pwBlock++;
        cWords--;
        wAddress += 2;
    }

    return OS_RETURN_SUCCESS;
}

static int _enable_eeprom_write(struct mw_pi7c9x2gxxx *pobj)
{
    int ret;
    DWORD dwValue;

    if (pobj->par_dev == NULL)
        return OS_RETURN_ERROR;

    dwValue = (0x01 | (6 << 1) | (1 << 5) | (1 << 6));
    ret = os_pci_write_config_dword(pobj->par_dev, 0xA0, dwValue);
    if (ret != OS_RETURN_SUCCESS)
        return ret;

    os_msleep(1);

    return OS_RETURN_SUCCESS;
}

static int _disable_eeprom_write(struct mw_pi7c9x2gxxx *pobj)
{
    int ret;
    DWORD dwValue;

    if (pobj->par_dev == NULL)
        return OS_RETURN_ERROR;

    dwValue = (0x01 | (4 << 1) | (1 << 5) | (1 << 6));
    ret = os_pci_write_config_dword(pobj->par_dev, 0xA0, dwValue);
    if (ret != OS_RETURN_SUCCESS)
        return ret;

    os_msleep(1);

    return OS_RETURN_SUCCESS;
}

static int _write_eeprom_data(struct mw_pi7c9x2gxxx *pobj, WORD wAddress, WORD wData)
{
    int ret;
    DWORD dwValue;

    if (pobj->par_dev == NULL)
        return OS_RETURN_ERROR;

    dwValue = wAddress | (wData << 16);
    ret = os_pci_write_config_dword(pobj->par_dev, 0xA4, dwValue);
    if (ret != OS_RETURN_SUCCESS)
        return ret;

    dwValue = (0x01 | (2 << 1) | (1 << 5) | (1 << 6));
    ret = os_pci_write_config_dword(pobj->par_dev, 0xA0, dwValue);
    if (ret != OS_RETURN_SUCCESS)
        return ret;

    while (1) {
        os_msleep(1);

        ret = os_pci_read_config_dword(pobj->par_dev, 0xA0, &dwValue);
        if (ret != OS_RETURN_SUCCESS)
            return ret;

        if ((dwValue & 0x100) == 0)
            break;
    }

    return OS_RETURN_SUCCESS;
}

static int _write_eeprom_data_block(struct mw_pi7c9x2gxxx *pobj, const BYTE * pbyBlock, int cbBlock)
{
    int ret;
    const WORD * pwBlock = (const WORD *)pbyBlock;
    int cWords = cbBlock / sizeof(WORD);
    WORD wAddress = 0;

    if (pobj->par_dev == NULL)
        return OS_RETURN_ERROR;

    while (cWords != 0) {
        ret = _enable_eeprom_write(pobj);
        if (ret != OS_RETURN_SUCCESS)
            return ret;

        ret = _write_eeprom_data(pobj, wAddress, *pwBlock);
        if (ret != OS_RETURN_SUCCESS)
            return ret;

        pwBlock++;
        cWords--;
        wAddress += 2;
    }

    return _disable_eeprom_write(pobj);
}
