////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#ifndef __AUDIO_CAPTURE_H__
#define __AUDIO_CAPTURE_H__

#include "ospi/ospi.h"

struct audio_capture {
    volatile void __iomem   *reg_base;
};

enum AUD_INT_MASK {
    AUD_INT_MASK_INPUT_OVERFLOW		= 1 << 0,
    AUD_INT_MASK_INPUT_LOST_SYNC	= 1 << 1
};

void audio_capture_init(struct audio_capture *cap, volatile void __iomem *reg_base);

void audio_capture_set_int_enables(struct audio_capture *cap, u32 enable_bits);

u32 audio_capture_get_int_status(struct audio_capture *cap);

u32 audio_capture_get_int_raw_status(struct audio_capture *cap);

// Outputs
//   MSB first: x,...,x,n-1,n-2,...,1,0
//   LSB first: n-1,n-2,...,1,0,x,...,x

void audio_capture_start(struct audio_capture *cap,
        u8 input_mode, bool free_run, bool left_align, bool msb_first,
        u8 msb_index, u8 channel_masks, u16 samples_per_frame,
        u32 tag
        );

void audio_capture_stop(struct audio_capture *cap);

void audio_capture_reset(struct audio_capture *cap);

void audio_capture_set_free_run_sample_rate(struct audio_capture *cap,
        u32 sys_freq, u32 sample_rate);

void audio_capture_set_control_port_value(struct audio_capture *cap, u32 value);

void audio_capture_set_control_port_bits(struct audio_capture *cap, u32 bits);

void audio_capture_clear_control_port_bits(struct audio_capture *cap, u32 bits);

#endif /* __AUDIO_CAPTURE_H__ */
