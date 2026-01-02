////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "timestamp.h"

// NOTE: Read or write high part first
enum REG_ADDR {
    REG_ADDR_VER_CAPS				= 4 * 0,
    REG_ADDR_INT_ENABLE				= 4 * 1,
    REG_ADDR_INT_STATUS				= 4 * 2,
    REG_ADDR_INT_RAW_STATUS			= 4 * 3,
    REG_ADDR_TIME_LOW				= 4 * 4,
    REG_ADDR_TIME_HIGH				= 4 * 5,
    REG_ADDR_TIME_UPDATE_LOW		= 4 * 6,
    REG_ADDR_TIME_UPDATE_HIGH		= 4 * 7,
    REG_ADDR_THRESHOLD				= 4 * 8,
    REG_ADDR_ALARM_TIME_LOW			= 4 * 9,
    REG_ADDR_ALARM_TIME_HIGH		= 4 * 10
};

void xi_timestamp_init(struct xi_timestamp *ts, volatile void __iomem *reg_base)
{
    ts->reg_base = reg_base;
}

void xi_timestamp_set_int_enables(struct xi_timestamp *ts, u32 enable_bits)
{
    pci_write_reg32(ts->reg_base+REG_ADDR_INT_ENABLE, enable_bits);
}

u32 xi_timestamp_get_int_status(struct xi_timestamp *ts)
{
    return pci_read_reg32(ts->reg_base+REG_ADDR_INT_STATUS);
}

u32 xi_timestamp_get_int_raw_status(struct xi_timestamp *ts)
{
    return pci_read_reg32(ts->reg_base+REG_ADDR_INT_RAW_STATUS);
}

void xi_timestamp_set_time(struct xi_timestamp *ts, long long lltime)
{
    pci_write_reg32(ts->reg_base+REG_ADDR_TIME_HIGH, (u32)(lltime >> 32));
    pci_write_reg32(ts->reg_base+REG_ADDR_TIME_LOW, (u32)(lltime & 0xFFFFFFFF));
}

void xi_timestamp_set_threshold(struct xi_timestamp *ts, u32 threshold)
{
    pci_write_reg32(ts->reg_base+REG_ADDR_THRESHOLD, threshold);
}

void xi_timestamp_update_time(struct xi_timestamp *ts, long long lltime)
{
    pci_write_reg32(ts->reg_base+REG_ADDR_TIME_UPDATE_HIGH, (u32)(lltime >> 32));
    pci_write_reg32(ts->reg_base+REG_ADDR_TIME_UPDATE_LOW, (u32)(lltime & 0xFFFFFFFF));
}

long long xi_timestamp_get_time(struct xi_timestamp *ts)
{
    u32 high = pci_read_reg32(ts->reg_base+REG_ADDR_TIME_HIGH);
    u32 low = pci_read_reg32(ts->reg_base+REG_ADDR_TIME_LOW);

    return ((long long)high << 32) | low;
}

void xi_timestamp_set_alarm_time(struct xi_timestamp *ts, long long lltime)
{
    pci_write_reg32(ts->reg_base+REG_ADDR_ALARM_TIME_HIGH, (u32)(lltime >> 32));
    pci_write_reg32(ts->reg_base+REG_ADDR_ALARM_TIME_LOW, (u32)(lltime & 0xFFFFFFFF));
}

void xi_timestamp_enable_alarm_int(struct xi_timestamp *ts, bool enable)
{
    pci_write_reg32(ts->reg_base+REG_ADDR_INT_ENABLE, enable ? 0x01 : 0x00);
}

