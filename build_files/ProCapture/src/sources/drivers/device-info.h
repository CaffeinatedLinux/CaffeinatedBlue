////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#ifndef __DEVICE_INFO_H__
#define __DEVICE_INFO_H__

#include "ospi/ospi.h"

struct xi_device {
    volatile void __iomem *reg_base;
};

void xi_device_init(struct xi_device *device, volatile void __iomem *reg_base);

u32 xi_device_get_hardware_ver(struct xi_device *device);

u32 xi_device_get_firmware_ver(struct xi_device *device);

u8 xi_device_get_firmware_id(struct xi_device *device);

u16 xi_device_get_product_id(struct xi_device *device);

u32 xi_device_get_channel_id(struct xi_device *device);

u32 xi_device_get_pcie_device_addr(struct xi_device *device);

u32 xi_device_get_pcie_link_status(struct xi_device *device);

u32 xi_device_get_pcie_payload_size(struct xi_device *device);

void xi_device_get_device_status(struct xi_device *device,
        unsigned char *auth_passed,
        unsigned char *mem_init_done,
        unsigned char *mem_error);

int xi_device_get_temperature(struct xi_device *device);

u32 xi_device_get_memory_size(struct xi_device *device);

u32 xi_device_get_vfs_frame_count(struct xi_device *device);

u32 xi_device_get_vfs_full_buffer_address(struct xi_device *device);

u32 xi_device_get_vfs_full_frame_size(struct xi_device *device);

u32 xi_device_get_vfs_quarter_buffer_address(struct xi_device *device);

u32 xi_device_get_vfs_quarter_frame_size(struct xi_device *device);

u32 xi_device_get_user_buffer_address(struct xi_device *device, int num_chs, u32 *size);

u32 xi_device_get_ref_clk_freq(struct xi_device *device);

void xi_device_reconfig(struct xi_device *device, u32 delay_ms);

void xi_device_set_led_mode(struct xi_device *device, u32 led_mode);

#endif /* __DEVICE_INFO_H__ */
