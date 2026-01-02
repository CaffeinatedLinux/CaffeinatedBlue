////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "pcie-ring-dma-controller.h"

enum REG_ADDR {
    REG_ADDR_VER_CAPS			= 0 * 4,
    REG_ADDR_CONTROL			= 1 * 4,
    REG_ADDR_STATUS				= 2 * 4,
    REG_ADDR_SIZE_INFO			= 3 * 4,
    REG_ADDR_ADDR_LOW			= 4 * 4,
    REG_ADDR_ADDR_HIGH			= 5 * 4
};

void xi_pcie_ring_dma_controller_init(struct pcie_ring_dma_controller *pobj, volatile void __iomem *baseaddr)
{
    pobj->reg_base = baseaddr;
}

void xi_pcie_ring_dma_controller_set_control(struct pcie_ring_dma_controller *pobj, bool enable)
{
    pci_write_reg32(pobj->reg_base+REG_ADDR_CONTROL, enable ? 0x01 : 0x00);
}

bool xi_pcie_ring_dma_controller_get_status(struct pcie_ring_dma_controller *pobj,
        bool *interrupt,
        u8 *completed_block_id)
{
    u32 status = pci_read_reg32(pobj->reg_base+REG_ADDR_STATUS);

    if (NULL != interrupt)
        *interrupt = (status & 0x02) ? true : false;
    if (NULL != completed_block_id)
        *completed_block_id = (u8)(status >> 8);

    return (status & 0x01) ? true : false; // Busy
}

void xi_pcie_ring_dma_controller_enable(struct pcie_ring_dma_controller *pobj)
{
    xi_pcie_ring_dma_controller_set_control(pobj, true);
}

void xi_pcie_ring_dma_controller_disable(struct pcie_ring_dma_controller *pobj)
{
    xi_pcie_ring_dma_controller_set_control(pobj, false);
    /*
    while (xi_pcie_ring_dma_controller_get_status(pobj, NULL, NULL))
        os_msleep(1);
        */
}

void xi_pcie_ring_dma_controller_clear_interrupt(struct pcie_ring_dma_controller *pobj)
{
    pci_write_reg32(pobj->reg_base+REG_ADDR_STATUS, 0x02);
}

void xi_pcie_ring_dma_controller_setup_ring_buffer(struct pcie_ring_dma_controller *pobj,
        u32 addr_high,
        u32 addr_low,
        u32 block_size,
        u32 block_count)
{
    if (block_size == 0 || block_count == 0 || ((block_size & 0x03) != 0))
        return;

    pci_write_reg32(pobj->reg_base+REG_ADDR_ADDR_LOW, addr_low);
    pci_write_reg32(pobj->reg_base+REG_ADDR_ADDR_HIGH, addr_high);
    pci_write_reg32(pobj->reg_base+REG_ADDR_SIZE_INFO, (block_count - 1) | (block_size << 8));
}

