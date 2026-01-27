////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "audio-capture.h"

enum REG_ADDR {
    REG_ADDR_VER_CAPS			= 4 * 0,
    REG_ADDR_INT_ENABLE			= 4 * 1,
    REG_ADDR_INT_STATUS			= 4 * 2,
    REG_ADDR_INT_RAW_STATUS		= 4 * 3,
    REG_ADDR_CONTROL			= 4 * 4,
    REG_ADDR_RESET				= 4 * 5,
    REG_ADDR_INTERVAL_INT		= 4 * 6,
    REG_ADDR_INTERVAL_FRAC		= 4 * 7,
    REG_ADDR_CONTROL_PORT		= 4 * 8,
    REG_ADDR_CONTROL_PORT_SET	= 4 * 9,
    REG_ADDR_CONTROL_PORT_CLEAR	= 4 * 10,
    REG_ADDR_TAG                = 4 * 11,
    REG_ADDR_CONTROL_EXT		= 4 * 12
};

void audio_capture_init(struct audio_capture *cap, volatile void __iomem *reg_base)
{
    cap->reg_base = reg_base;
}

void audio_capture_set_int_enables(struct audio_capture *cap, u32 enable_bits)
{
    pci_write_reg32(cap->reg_base+REG_ADDR_INT_ENABLE, enable_bits);
}

u32 audio_capture_get_int_status(struct audio_capture *cap)
{
    return pci_read_reg32(cap->reg_base+REG_ADDR_INT_STATUS);
}

u32 audio_capture_get_int_raw_status(struct audio_capture *cap)
{
    return pci_read_reg32(cap->reg_base+REG_ADDR_INT_RAW_STATUS);
}

void audio_capture_start(struct audio_capture *cap,
        u8 input_mode, bool free_run, bool left_align, bool msb_first,
        u8 msb_index, u8 channel_masks, u16 samples_per_frame,
        u32 tag
        )
{
    u32 ctrl_value = 0x01;
    
    if (free_run)
        ctrl_value |= 0x02;
    if (left_align)
        ctrl_value |= 0x04;
    if (msb_first)
        ctrl_value |= 0x08;
    ctrl_value |= ((msb_index & 0x1F) << 4);
    ctrl_value |= ((channel_masks & 0x0F) << 12);
    ctrl_value |= (samples_per_frame * 2 - 1) << 16;
    ctrl_value |= (input_mode << 28);

    pci_write_reg32(cap->reg_base+REG_ADDR_TAG, tag);
    pci_write_reg32(cap->reg_base+REG_ADDR_CONTROL, ctrl_value);
    pci_write_reg32(cap->reg_base+REG_ADDR_CONTROL_EXT, channel_masks >> 4);
}

void audio_capture_stop(struct audio_capture *cap)
{
    pci_write_reg32(cap->reg_base+REG_ADDR_CONTROL, 0);
}

void audio_capture_reset(struct audio_capture *cap)
{
    pci_write_reg32(cap->reg_base+REG_ADDR_RESET, 0x01);
}

void audio_capture_set_free_run_sample_rate(struct audio_capture *cap,
        u32 sys_freq, u32 sample_rate)
{
    u32 div, mod, frac;

    // L & R
    sample_rate *= 2;

    div = sys_freq / sample_rate;
    mod = sys_freq % sample_rate;
    frac = (u32)os_div64((long long)mod * 0x100000000LL, sample_rate);

    pci_write_reg32(cap->reg_base+REG_ADDR_INTERVAL_INT, div);
    pci_write_reg32(cap->reg_base+REG_ADDR_INTERVAL_FRAC, frac);
}

void audio_capture_set_control_port_value(struct audio_capture *cap, u32 value)
{
    pci_write_reg32(cap->reg_base+REG_ADDR_CONTROL_PORT, value);
}

void audio_capture_set_control_port_bits(struct audio_capture *cap, u32 bits)
{
    pci_write_reg32(cap->reg_base+REG_ADDR_CONTROL_PORT_SET, bits);
}

void audio_capture_clear_control_port_bits(struct audio_capture *cap, u32 bits)
{
    pci_write_reg32(cap->reg_base+REG_ADDR_CONTROL_PORT_CLEAR, bits);
}

