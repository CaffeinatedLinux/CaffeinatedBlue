////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#ifndef __PCIE_RING_DMA_CONTROLLER_H__
#define __PCIE_RING_DMA_CONTROLLER_H__

#include "ospi/ospi.h"

struct pcie_ring_dma_controller {
    volatile void __iomem *reg_base;
};

void xi_pcie_ring_dma_controller_init(struct pcie_ring_dma_controller *pobj, volatile void __iomem *baseaddr);

void xi_pcie_ring_dma_controller_set_control(struct pcie_ring_dma_controller *pobj, bool enable);

bool xi_pcie_ring_dma_controller_get_status(struct pcie_ring_dma_controller *pobj,
        bool *interrupt,
        u8 *completed_block_id);

void xi_pcie_ring_dma_controller_enable(struct pcie_ring_dma_controller *pobj);

void xi_pcie_ring_dma_controller_disable(struct pcie_ring_dma_controller *pobj);

void xi_pcie_ring_dma_controller_clear_interrupt(struct pcie_ring_dma_controller *pobj);

void xi_pcie_ring_dma_controller_setup_ring_buffer(struct pcie_ring_dma_controller *pobj,
        u32 addr_high,
        u32 addr_low,
        u32 block_size,
        u32 block_count);

#endif /* __PCIE_RING_DMA_CONTROLLER_H__ */

