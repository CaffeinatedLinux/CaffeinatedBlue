////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "pcie-dma-controller.h"

enum REG_ADDR {
    REG_ADDR_VER_CAPS			= 0 * 4,
    REG_ADDR_CONTROL			= 1 * 4,
    REG_ADDR_STATUS				= 2 * 4,
    REG_ADDR_MAX_BURST_LEN		= 3 * 4,
    REG_ADDR_DESC_ADDR_LOW		= 3 * 4,
    REG_ADDR_DESC_ADDR_HIGH		= 4 * 4,
    REG_ADDR_DESC_COUNT			= 5 * 4,
    REG_ADDR_DESC_CPLD_TAG		= 6 * 4,
    REG_ADDR_DESC_IRQ_TAG 		= 7 * 4,
    REG_ADDR_FREE_COUNT 		= 8 * 4
};

void xi_pcie_dma_controller_init(struct pcie_dma_controller *pobj, volatile void __iomem *baseaddr)
{
    pobj->reg_base = baseaddr;
}

void xi_pcie_dma_controller_set_control(struct pcie_dma_controller *pobj,
                                        BOOLEAN bEnable,
                                        BOOLEAN bStreamMode)
{
    pci_write_reg32(pobj->reg_base+REG_ADDR_CONTROL, (bEnable ? 0x01 : 0x00) | (bStreamMode ? 0x04 : 0x00));
}

unsigned long xi_pcie_dma_controller_get_max_transfer_size(struct pcie_dma_controller *pobj)
{
    u32 max_transfer_size = pci_read_reg32(pobj->reg_base+REG_ADDR_MAX_BURST_LEN);

    return (max_transfer_size == 0) ? (8 * 1024 * 1024) : max_transfer_size;
}

bool xi_pcie_dma_controller_get_status(struct pcie_dma_controller *pobj,
                                       BOOLEAN * pbInterrupt,
                                       BOOLEAN * pbCPLDTagValid,
                                       BOOLEAN * pbCPLDStreamError,
                                       BOOLEAN * pbIRQTagValid,
                                       BOOLEAN * pbIRQStreamError,
                                       BOOLEAN * pbRequestFull,
                                       BOOLEAN * pbRequestEmpty
                                       )
{
    DWORD dwStatus = pci_read_reg32(pobj->reg_base+REG_ADDR_STATUS);

    if (pbInterrupt) *pbInterrupt = (dwStatus & 0x02) ? TRUE : FALSE;
    if (pbCPLDTagValid) *pbCPLDTagValid = (dwStatus & 0x04) ? TRUE : FALSE;
    if (pbCPLDStreamError) *pbCPLDStreamError = (dwStatus & 0x08) ? TRUE : FALSE;
    if (pbIRQTagValid) *pbIRQTagValid = (dwStatus & 0x10) ? TRUE : FALSE;
    if (pbIRQStreamError) *pbIRQStreamError = (dwStatus & 0x20) ? TRUE : FALSE;
    if (pbRequestFull) *pbRequestFull = (dwStatus & 0x40) ? TRUE : FALSE;
    if (pbRequestEmpty) *pbRequestEmpty = (dwStatus & 0x80) ? TRUE : FALSE;

    return (dwStatus & 0x01) ? TRUE : FALSE; // Busy
}

void xi_pcie_dma_controller_enable(struct pcie_dma_controller *pobj, BOOLEAN bStreamMode)
{
    xi_pcie_dma_controller_set_control(pobj, true, bStreamMode);
}

void xi_pcie_dma_controller_disable(struct pcie_dma_controller *pobj)
{
    xi_pcie_dma_controller_set_control(pobj, false, false);
    while (xi_pcie_dma_controller_get_status(pobj, NULL, NULL, NULL, NULL, NULL, NULL, NULL))
        os_udelay(10);
}

DWORD xi_pcie_dma_controller_GetCPLDTag(struct pcie_dma_controller *pobj)
{
    return pci_read_reg32(pobj->reg_base+REG_ADDR_DESC_CPLD_TAG);
}

DWORD xi_pcie_dma_controller_GetIRQTag(struct pcie_dma_controller *pobj)
{
    return pci_read_reg32(pobj->reg_base+REG_ADDR_DESC_IRQ_TAG);
}

void xi_pcie_dma_controller_clear_interrupt(struct pcie_dma_controller *pobj)
{
    pci_write_reg32(pobj->reg_base+REG_ADDR_STATUS, 0x02);
}

void xi_pcie_dma_controller_clear_cpldtag_valid(struct pcie_dma_controller *pobj)
{
    pci_write_reg32(pobj->reg_base+REG_ADDR_STATUS, 0x04);
}

VOID xi_pcie_dma_controller_ClearIRQTagValid(struct pcie_dma_controller *pobj)
{
    pci_write_reg32(pobj->reg_base+REG_ADDR_STATUS, 0x10);
}

void xi_pcie_dma_controller_xfer_chain(struct pcie_dma_controller *pobj,
        u32 chain_addr_high, u32 chain_addr_low,
        u32 desc_count)
{
    pci_write_reg32(pobj->reg_base+REG_ADDR_DESC_ADDR_LOW, chain_addr_low);
    pci_write_reg32(pobj->reg_base+REG_ADDR_DESC_ADDR_HIGH, chain_addr_high);
    pci_write_reg32(pobj->reg_base+REG_ADDR_DESC_COUNT, desc_count);
}

