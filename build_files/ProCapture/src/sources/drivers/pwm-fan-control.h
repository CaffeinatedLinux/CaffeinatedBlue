////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2017 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#ifndef __PWM_FAN_CONTROL_H__
#define __PWM_FAN_CONTROL_H__

#include "ospi/ospi.h"
#include "win-types.h"

struct pwm_fan_ctrl {
    volatile void __iomem   *m_reg_base;

    bool                    m_bPresent;
    DWORD                   m_dwClkFreq;
    DWORD                   m_dwPWMPeriod;
};

void pwm_fan_ctrl_init(struct pwm_fan_ctrl *pdev, volatile void __iomem *reg_base,
                       DWORD dwClkFreq, DWORD dwPWMFreq);

bool pwm_fan_ctrl_IsPresent(struct pwm_fan_ctrl *pdev);

void pwm_fan_ctrl_Enable(struct pwm_fan_ctrl *pdev, bool bEnable);

void pwm_fan_ctrl_SetDuty(struct pwm_fan_ctrl *pdev, BYTE byDuty);

DWORD pwm_fan_ctrl_GetRPM(struct pwm_fan_ctrl *pdev);

#endif /* __PWM_FAN_CONTROL_H__ */
