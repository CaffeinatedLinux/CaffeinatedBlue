////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "device-info.h"

enum REG_ADDR {
    REG_ADDR_HARDWARE_VER			= 4 * 0,
    REG_ADDR_FIRMWARE_VER			= 4 * 1,
    REG_ADDR_PRODUCT_ID				= 4 * 2,
    REG_ADDR_CHANNEL_ID				= 4 * 3,
    REG_ADDR_DEVICE_STATUS			= 4 * 4,
    REG_ADDR_PCIE_DEVICE_ADDR		= 4 * 5,
    REG_ADDR_PCIE_LINK_STATUS		= 4 * 6,
    REG_ADDR_PCIE_PAYLOAD_SIZE		= 4 * 7,
    REG_ADDR_DEVICE_TEMPERATURE		= 4 * 8,
    REG_ADDR_RECONFIG_DELAY			= 4 * 9,
    REG_ADDR_MEMORY_SIZE			= 4 * 10,
    REG_ADDR_VFS_FRAME_COUNT		= 4 * 11,
    REG_ADDR_VFS_FULL_FRAME_SIZE	= 4 * 12,
    REG_ADDR_MAX_IMAGE_DIMENSION	= 4 * 14,
    REG_ADDR_REF_CLK_FREQ			= 4 * 15,
    REG_ADDR_LED_CONTROL            = 4 * 0
};

void xi_device_init(struct xi_device *device, volatile void __iomem *reg_base)
{
    device->reg_base = reg_base;
}

u32 xi_device_get_hardware_ver(struct xi_device *device)
{
    return pci_read_reg32(device->reg_base+REG_ADDR_HARDWARE_VER);
}

u32 xi_device_get_firmware_ver(struct xi_device *device)
{
    return pci_read_reg32(device->reg_base+REG_ADDR_FIRMWARE_VER);
}

u8 xi_device_get_firmware_id(struct xi_device *device)
{
    return (pci_read_reg32(device->reg_base+REG_ADDR_PRODUCT_ID) >> 16) & 0xFF;
}

u16 xi_device_get_product_id(struct xi_device *device)
{
    return pci_read_reg32(device->reg_base+REG_ADDR_PRODUCT_ID) & 0xFFFF;
}

u32 xi_device_get_channel_id(struct xi_device *device)
{
    return pci_read_reg32(device->reg_base+REG_ADDR_CHANNEL_ID);
}

u32 xi_device_get_pcie_device_addr(struct xi_device *device)
{
    return pci_read_reg32(device->reg_base+REG_ADDR_PCIE_DEVICE_ADDR);
}

u32 xi_device_get_pcie_link_status(struct xi_device *device)
{
    return pci_read_reg32(device->reg_base+REG_ADDR_PCIE_LINK_STATUS);
}

u32 xi_device_get_pcie_payload_size(struct xi_device *device)
{
    return pci_read_reg32(device->reg_base+REG_ADDR_PCIE_PAYLOAD_SIZE);
}

void xi_device_get_device_status(struct xi_device *device,
        unsigned char *auth_passed,
        unsigned char *mem_init_done,
        unsigned char *mem_error)
{
    u32 status;

    status = pci_read_reg32(device->reg_base+REG_ADDR_DEVICE_STATUS);

    if (NULL != auth_passed)
        *auth_passed = (status & 0x01) ? 1 : 0;
    if (NULL != mem_init_done)
        *mem_init_done = (status & 0x02) ? 1 : 0;
    if (NULL != mem_error)
        *mem_error = (status & 0x04) ? 1 : 0;
}

int xi_device_get_temperature(struct xi_device *device)
{
    int adc_code = (int)pci_read_reg32(device->reg_base+REG_ADDR_DEVICE_TEMPERATURE);
    if (adc_code & 0x80000000)
        return (((adc_code & 0x7FFFFFFF) * 693 * 10) >> 10) - 2650;

    return ((adc_code * 504 * 10) >> 12) - 2730;
}

u32 xi_device_get_memory_size(struct xi_device *device)
{
    return pci_read_reg32(device->reg_base+REG_ADDR_MEMORY_SIZE);
}

u32 xi_device_get_vfs_frame_count(struct xi_device *device)
{
    return pci_read_reg32(device->reg_base+REG_ADDR_VFS_FRAME_COUNT);
}

u32 xi_device_get_vfs_full_buffer_address(struct xi_device *device)
{
    return 0;
}

u32 xi_device_get_vfs_full_frame_size(struct xi_device *device)
{
    return pci_read_reg32(device->reg_base+REG_ADDR_VFS_FULL_FRAME_SIZE);
}

u32 xi_device_get_vfs_quarter_buffer_address(struct xi_device *device)
{
    u32 frame_count = pci_read_reg32(device->reg_base+REG_ADDR_VFS_FRAME_COUNT);
    u32 full_frame_size = pci_read_reg32(device->reg_base+REG_ADDR_VFS_FULL_FRAME_SIZE);
    return frame_count * full_frame_size;
}

u32 xi_device_get_vfs_quarter_frame_size(struct xi_device *device)
{
    // Check if quarter frame buffer disabled
    if (pci_read_reg32(device->reg_base+REG_ADDR_DEVICE_STATUS) & 0x80000000)
        return 0;

    return xi_device_get_vfs_full_frame_size(device) / 4;
}

u32 xi_device_get_user_buffer_address(struct xi_device *device, int num_chs, u32 *size)
{
    u32 frame_count = pci_read_reg32(device->reg_base+REG_ADDR_VFS_FRAME_COUNT);
    u32 full_frame_size = pci_read_reg32(device->reg_base+REG_ADDR_VFS_FULL_FRAME_SIZE) * num_chs;
    u32 quarter_frame_size =
            (pci_read_reg32(device->reg_base+REG_ADDR_DEVICE_STATUS) & 0x80000000) ?
                0 : (full_frame_size / 4);
    u32 address = (full_frame_size + quarter_frame_size) * frame_count;
    if (NULL != size)
        *size = pci_read_reg32(device->reg_base+REG_ADDR_MEMORY_SIZE) - address;

    return address;
}

u32 xi_device_get_ref_clk_freq(struct xi_device *device)
{
    return pci_read_reg32(device->reg_base+REG_ADDR_REF_CLK_FREQ);
}

void xi_device_reconfig(struct xi_device *device, u32 delay_ms)
{
    if (delay_ms > 1000)
        delay_ms = 1000;
    pci_write_reg32(device->reg_base+REG_ADDR_RECONFIG_DELAY, delay_ms);
}

void xi_device_set_led_mode(struct xi_device *device, u32 led_mode)
{
    pci_write_reg32(device->reg_base+REG_ADDR_LED_CONTROL, led_mode);
}

