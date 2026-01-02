////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#ifndef __PCIE_DMA_CONTROLLER_H__
#define __PCIE_DMA_CONTROLLER_H__

#include "ospi/ospi.h"
#include "public/win-types.h"

struct pcie_dma_controller {
    volatile void __iomem *reg_base;
};

void xi_pcie_dma_controller_init(struct pcie_dma_controller *pobj, volatile void __iomem *baseaddr);

void xi_pcie_dma_controller_set_control(struct pcie_dma_controller *pobj,
                                        BOOLEAN bEnable,
                                        BOOLEAN bStreamMode);

unsigned long xi_pcie_dma_controller_get_max_transfer_size(struct pcie_dma_controller *pobj);

bool xi_pcie_dma_controller_get_status(struct pcie_dma_controller *pobj,
                                       BOOLEAN * pbInterrupt,
                                       BOOLEAN * pbCPLDTagValid,
                                       BOOLEAN * pbCPLDStreamError,
                                       BOOLEAN * pbIRQTagValid,
                                       BOOLEAN * pbIRQStreamError,
                                       BOOLEAN * pbRequestFull,
                                       BOOLEAN * pbRequestEmpty);

void xi_pcie_dma_controller_enable(struct pcie_dma_controller *pobj, BOOLEAN bStreamMode);

void xi_pcie_dma_controller_disable(struct pcie_dma_controller *pobj);

DWORD xi_pcie_dma_controller_GetCPLDTag(struct pcie_dma_controller *pobj);

DWORD xi_pcie_dma_controller_GetIRQTag(struct pcie_dma_controller *pobj);

void xi_pcie_dma_controller_clear_interrupt(struct pcie_dma_controller *pobj);

void xi_pcie_dma_controller_clear_cpldtag_valid(struct pcie_dma_controller *pobj);

VOID xi_pcie_dma_controller_ClearIRQTagValid(struct pcie_dma_controller *pobj);

void xi_pcie_dma_controller_xfer_chain(struct pcie_dma_controller *pobj,
        u32 chain_addr_high, u32 chain_addr_low,
        u32 desc_count);

#endif /* __PCIE_DMA_CONTROLLER_H__ */
