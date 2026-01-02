////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2017 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "pwm-fan-control.h"

#include "supports/math.h"

enum REG_ADDR {
    REG_ADDR_VER_CAPS		= 0 * 4,
    REG_ADDR_PWM_CONTROL	= 1 * 4, // {EN, RESERVED[], PERIOD_MINUS_1}
    REG_ADDR_PWM_HIGH		= 2 * 4, // {HIGH_TIME}
    REG_ADDR_SENSE_STATUS	= 3 * 4  // {LOCKED, SENSE_PERIOD}
};

void pwm_fan_ctrl_init(struct pwm_fan_ctrl *pdev, volatile void __iomem *reg_base,
                       DWORD dwClkFreq, DWORD dwPWMFreq)
{
    DWORD dwVerCaps;

    pdev->m_reg_base = reg_base;
    pdev->m_dwClkFreq = dwClkFreq;
    pdev->m_dwPWMPeriod = dwClkFreq / dwPWMFreq;

    dwVerCaps = pci_read_reg32(pdev->m_reg_base+REG_ADDR_VER_CAPS);
    pdev->m_bPresent = (dwVerCaps != 0 && dwVerCaps != 0xFFFFFFFF);
}

bool pwm_fan_ctrl_IsPresent(struct pwm_fan_ctrl *pdev)
{
    return pdev->m_bPresent;
}

void pwm_fan_ctrl_Enable(struct pwm_fan_ctrl *pdev, bool bEnable)
{
    pci_write_reg32(pdev->m_reg_base+REG_ADDR_PWM_CONTROL, bEnable ? (0x80000000 | pdev->m_dwPWMPeriod) : 0);
}

void pwm_fan_ctrl_SetDuty(struct pwm_fan_ctrl *pdev, BYTE byDuty)
{
    DWORD dwHighTime;

    byDuty = _limit(byDuty, 0, 100);
    dwHighTime = (byDuty * pdev->m_dwPWMPeriod) / 100;
    pci_write_reg32(pdev->m_reg_base+REG_ADDR_PWM_HIGH, dwHighTime);
}

DWORD pwm_fan_ctrl_GetRPM(struct pwm_fan_ctrl *pdev)
{
    DWORD dwPeriod = pci_read_reg32(pdev->m_reg_base+REG_ADDR_SENSE_STATUS);

    if (dwPeriod == 0 || (dwPeriod & 0x80000000) != 0)
        return 0;

    return (pdev->m_dwClkFreq / dwPeriod);
}
