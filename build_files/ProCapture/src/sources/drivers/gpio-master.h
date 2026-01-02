////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#ifndef __GPIO_MASTER_H__
#define __GPIO_MASTER_H__

#include "ospi/ospi.h"

enum GPIO_INTR_TRIG_MODE {
    GPIO_INTR_TRIG_MODE_CHANGE      = 0,
    GPIO_INTR_TRIG_MODE_EVENT       = 1
};

enum GPIO_INTR_TRIG_TYPE {
    GPIO_INTR_TRIG_TYPE_LEVEL       = 0,
    GPIO_INTR_TRIG_TYPE_EDGE        = 1
}; 

enum GPIO_INTR_TRIG_VALUE {
    GPIO_INTR_TRIG_VALUE_LOW        = 0,
    GPIO_INTR_TRIG_VALUE_HIGH       = 1,
    GPIO_INTR_TRIG_VALUE_FALLING    = 0,
    GPIO_INTR_TRIG_VALUE_RISING     = 1
};

struct xi_gpio {
    volatile void __iomem *reg_base;
};

void xi_gpio_init(struct xi_gpio *gpio, volatile void __iomem *reg_base);

u32 xi_gpio_get_out_enable(struct xi_gpio *gpio);

void xi_gpio_set_out_enable(struct xi_gpio *gpio, u32 bits_val);

u32 xi_gpio_get_data_value(struct xi_gpio *gpio);

void xi_gpio_set_data_value(struct xi_gpio *gpio, u32 value);

void xi_gpio_set_data_value_with_mask(struct xi_gpio *gpio, u32 value, u32 enabled_bits);

void xi_gpio_set_data_bits(struct xi_gpio *gpio, u32 bits_val);

void xi_gpio_clear_data_bits(struct xi_gpio *gpio, u32 bits_val);

void xi_gpio_set_intr_trigger_mode(struct xi_gpio *gpio, u32 mode);

void xi_gpio_set_intr_trigger_type(struct xi_gpio *gpio, u32 type);

void xi_gpio_set_intr_trigger_value(struct xi_gpio *gpio, u32 value);

u32 xi_gpio_get_intr_enable_bits_value(struct xi_gpio *gpio);

void xi_gpio_set_intr_enable_bits_value(struct xi_gpio *gpio, u32 value);

void xi_gpio_set_intr_enable_bits(struct xi_gpio *gpio, u32 bits_val);

void xi_gpio_clear_intr_enable_bits(struct xi_gpio *gpio, u32 bits_val);

u32 xi_gpio_get_interrupt_status(struct xi_gpio *gpio);

void xi_gpio_clear_interrupt(struct xi_gpio *gpio, u32 bits_val);

#endif

