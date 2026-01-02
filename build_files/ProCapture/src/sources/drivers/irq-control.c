////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "irq-control.h"

enum REG_ADDR {
    REG_ADDR_VER_CAPS			= 4 * 0,
    REG_ADDR_INT_STATUS			= 4 * 1,
    REG_ADDR_INT_RAW_STATUS		= 4 * 2,
    REG_ADDR_INT_CONTROL		= 4 * 3,
    REG_ADDR_INT_ENABLE			= 4 * 4,
    REG_ADDR_INT_EN_SET			= 4 * 5,
    REG_ADDR_INT_EN_CLEAR		= 4 * 6,
    REG_ADDR_INT_TRIGGER		= 4 * 7,
    REG_ADDR_INT_TRIG_SET		= 4 * 8,
    REG_ADDR_INT_TRIG_CLEAR		= 4 * 9
};

void xi_irq_init(struct xi_irq_control *ctrl, volatile void __iomem *reg_base)
{
    ctrl->reg_base = reg_base;
}

void xi_irq_set_control(struct xi_irq_control *ctrl, unsigned char enable)
{
    pci_write_reg32(ctrl->reg_base+REG_ADDR_INT_CONTROL, enable ? 0x01:0x00);
}

bool xi_irq_is_timeout(struct xi_irq_control *ctrl)
{
    return (pci_read_reg32(ctrl->reg_base+REG_ADDR_INT_CONTROL) & 0x02) != 0;
}

u32 xi_irq_get_raw_status(struct xi_irq_control *ctrl)
{
    return pci_read_reg32(ctrl->reg_base+REG_ADDR_INT_RAW_STATUS);
}

u32 xi_irq_get_enabled_status(struct xi_irq_control *ctrl)
{
    return pci_read_reg32(ctrl->reg_base+REG_ADDR_INT_STATUS);
}

u32 xi_irq_get_enable_bits_value(struct xi_irq_control *ctrl)
{
    return pci_read_reg32(ctrl->reg_base+REG_ADDR_INT_ENABLE);
}

void xi_irq_set_enable_bits_value(struct xi_irq_control *ctrl, u32 bits_val)
{
    pci_write_reg32(ctrl->reg_base+REG_ADDR_INT_ENABLE, bits_val);
}

void xi_irq_set_enable_bits(struct xi_irq_control *ctrl, u32 bits_val)
{
    pci_write_reg32(ctrl->reg_base+REG_ADDR_INT_EN_SET, bits_val);
}

void xi_irq_clear_enable_bits(struct xi_irq_control *ctrl, u32 bits_val)
{
    pci_write_reg32(ctrl->reg_base+REG_ADDR_INT_EN_CLEAR, bits_val);
}

u32 xi_irq_get_trigger_bits_value(struct xi_irq_control *ctrl)
{
    return pci_read_reg32(ctrl->reg_base+REG_ADDR_INT_TRIGGER);
}

void xi_irq_set_trigger_bits_value(struct xi_irq_control *ctrl, u32 bits_val)
{
    pci_write_reg32(ctrl->reg_base+REG_ADDR_INT_TRIGGER, bits_val);
}

void xi_irq_set_trigger_bits(struct xi_irq_control *ctrl, u32 bits_val)
{
    pci_write_reg32(ctrl->reg_base+REG_ADDR_INT_TRIG_SET, bits_val);
}

void xi_irq_clear_trigger_bits(struct xi_irq_control *ctrl, u32 bits_val)
{
    pci_write_reg32(ctrl->reg_base+REG_ADDR_INT_TRIG_CLEAR, bits_val);
}

