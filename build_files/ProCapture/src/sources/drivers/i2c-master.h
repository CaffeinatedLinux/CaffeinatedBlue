////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#ifndef __I2C_MASTER_H__
#define __I2C_MASTER_H__

#include "ospi/ospi.h"

struct xi_i2c_master {
    os_event_t              cmd_done;
    os_mutex_t              mutex;

    volatile void __iomem   *reg_base;

    uint32_t                clk_freq;
    uint32_t                bitrate;
    bool                    is_version2;

    u32                     response;
};

int xi_i2c_master_init(struct xi_i2c_master *master,
        volatile void __iomem *reg_base, uint32_t clk_freq, uint32_t bitrate);

void xi_i2c_master_deinit(struct xi_i2c_master *master);

void xi_i2c_master_irq_handler_top(struct xi_i2c_master *master);

void xi_i2c_master_irq_handler_bottom(struct xi_i2c_master *master);

int xi_i2c_master_read(struct xi_i2c_master *master,
        int ch, uint8_t devaddr, uint8_t regaddr, uint8_t *val);

int xi_i2c_master_write(struct xi_i2c_master *master,
        int ch, uint8_t devaddr, uint8_t regaddr, uint8_t val);

int xi_i2c_master_read_regs(struct xi_i2c_master *master,
        int ch, uint8_t devaddr, uint8_t regaddr, uint8_t *data, short count);

int xi_i2c_master_write_regs(struct xi_i2c_master *master,
        int ch, uint8_t devaddr, uint8_t regaddr, uint8_t *data, short count);

#endif /* __I2C_MASTER_H__ */
