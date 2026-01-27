////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "gpio-master.h"

enum REG_ADDR {
    REG_ADDR_VER_CAPS			= 4 * 0,
    REG_ADDR_OUT_ENALBE			= 4 * 1,	
    REG_ADDR_DATA				= 4 * 2,
    REG_ADDR_SET_DATA			= 4 * 3,
    REG_ADDR_RESET_DATA			= 4 * 4,
    REG_ADDR_INTR_MASK			= 4 * 5,		// 0 - Disable interrupt, 1 - Enable interrupt
    REG_ADDR_SET_INTR_MASK		= 4 * 6,
    REG_ADDR_RESET_INTR_MASK	= 4 * 7,		
    REG_ADDR_INTR_TRIG_MODE		= 4 * 8,		// 0 - Trigger on change, 1 - Trigger on event
    REG_ADDR_INTR_TRIG_TYPE		= 4 * 9,		// 0 - Level trigger, 1 - Edge trigger
    REG_ADDR_INTR_TRIG_VALUE	= 4 * 10,		// 0 - Low or falling edge, 1 - High or rising edge
    REG_ADDR_INTR_STATUS		= 4 * 11,		// 0 - No interrupt, 1 - interrupt; R, W1C
    REG_ADDR_INTR_ENABLED_STATUS= 4 * 12,
    REG_ADDR_DATA_MASK			= 4 * 13		// 1 - Write protected, 0 - Write enabled
};

void xi_gpio_init(struct xi_gpio *gpio, volatile void __iomem *reg_base)
{
    gpio->reg_base = reg_base;
}

u32 xi_gpio_get_out_enable(struct xi_gpio *gpio)
{
    return pci_read_reg32(gpio->reg_base+REG_ADDR_OUT_ENALBE);
}

void xi_gpio_set_out_enable(struct xi_gpio *gpio, u32 bits_val)
{
    pci_write_reg32(gpio->reg_base+REG_ADDR_OUT_ENALBE, bits_val);
}

u32 xi_gpio_get_data_value(struct xi_gpio *gpio)
{
    return pci_read_reg32(gpio->reg_base+REG_ADDR_DATA);
}

void xi_gpio_set_data_value(struct xi_gpio *gpio, u32 value)
{
    xi_gpio_set_data_value_with_mask(gpio, value, 0xFFFFFFFF);
}

void xi_gpio_set_data_value_with_mask(struct xi_gpio *gpio, u32 value, u32 enabled_bits)
{
    pci_write_reg32(gpio->reg_base+REG_ADDR_DATA_MASK, ~enabled_bits);
    pci_write_reg32(gpio->reg_base+REG_ADDR_DATA, value);
}

void xi_gpio_set_data_bits(struct xi_gpio *gpio, u32 bits_val)
{
    pci_write_reg32(gpio->reg_base+REG_ADDR_SET_DATA, bits_val);
}

void xi_gpio_clear_data_bits(struct xi_gpio *gpio, u32 bits_val)
{
    pci_write_reg32(gpio->reg_base+REG_ADDR_RESET_DATA, bits_val);
}

void xi_gpio_set_intr_trigger_mode(struct xi_gpio *gpio, u32 mode)
{
    pci_write_reg32(gpio->reg_base+REG_ADDR_INTR_TRIG_MODE, mode);
}

void xi_gpio_set_intr_trigger_type(struct xi_gpio *gpio, u32 type)
{
    pci_write_reg32(gpio->reg_base+REG_ADDR_INTR_TRIG_TYPE, type);
}

void xi_gpio_set_intr_trigger_value(struct xi_gpio *gpio, u32 value)
{
    pci_write_reg32(gpio->reg_base+REG_ADDR_INTR_TRIG_VALUE, value);
}

u32 xi_gpio_get_intr_enable_bits_value(struct xi_gpio *gpio)
{
    return pci_read_reg32(gpio->reg_base+REG_ADDR_INTR_MASK);
}

void xi_gpio_set_intr_enable_bits_value(struct xi_gpio *gpio, u32 value)
{
    pci_write_reg32(gpio->reg_base+REG_ADDR_INTR_MASK, value);
}

void xi_gpio_set_intr_enable_bits(struct xi_gpio *gpio, u32 bits_val)
{
    pci_write_reg32(gpio->reg_base+REG_ADDR_SET_INTR_MASK, bits_val);
}

void xi_gpio_clear_intr_enable_bits(struct xi_gpio *gpio, u32 bits_val)
{
    pci_write_reg32(gpio->reg_base+REG_ADDR_RESET_INTR_MASK, bits_val);
}

u32 xi_gpio_get_interrupt_status(struct xi_gpio *gpio)
{
    return pci_read_reg32(gpio->reg_base+REG_ADDR_INTR_STATUS);
}

void xi_gpio_clear_interrupt(struct xi_gpio *gpio, u32 bits_val)
{
    pci_write_reg32(gpio->reg_base+REG_ADDR_INTR_STATUS, bits_val);
}

u32 xi_gpio_get_enabled_interrupt_status(struct xi_gpio *gpio)
{
    return pci_read_reg32(gpio->reg_base+REG_ADDR_INTR_ENABLED_STATUS);
}

