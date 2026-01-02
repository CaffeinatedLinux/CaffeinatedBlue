////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#ifndef __IRQ_CONTROL_H__
#define __IRQ_CONTROL_H__
#include "ospi/ospi.h"

struct xi_irq_control {
    volatile void __iomem *reg_base;
};

void xi_irq_init(struct xi_irq_control *ctrl, volatile void __iomem *reg_base);

void xi_irq_set_control(struct xi_irq_control *ctrl, unsigned char enable);

bool xi_irq_is_timeout(struct xi_irq_control *ctrl);

u32 xi_irq_get_raw_status(struct xi_irq_control *ctrl);
u32 xi_irq_get_enabled_status(struct xi_irq_control *ctrl);
u32 xi_irq_get_enable_bits_value(struct xi_irq_control *ctrl);
void xi_irq_set_enable_bits_value(struct xi_irq_control *ctrl, u32 bits_val);
void xi_irq_set_enable_bits(struct xi_irq_control *ctrl, u32 bits_val);
void xi_irq_clear_enable_bits(struct xi_irq_control *ctrl, u32 bits_val);
u32 xi_irq_get_trigger_bits_value(struct xi_irq_control *ctrl);
void xi_irq_set_trigger_bits_value(struct xi_irq_control *ctrl, u32 bits_val);
void xi_irq_set_trigger_bits(struct xi_irq_control *ctrl, u32 bits_val);
void xi_irq_clear_trigger_bits(struct xi_irq_control *ctrl, u32 bits_val);

#endif /* __IRQ_CONTROL_H__ */
