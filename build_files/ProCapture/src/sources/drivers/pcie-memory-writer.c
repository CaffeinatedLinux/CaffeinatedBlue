////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "pcie-memory-writer.h"

enum REG_ADDR {
    REG_ADDR_VER_CAPS			= 0 * 4,
    REG_ADDR_STATUS				= 1 * 4,
    REG_ADDR_CONTROL			= 2 * 4,
    REG_ADDR_ADDRESS_LOW		= 3 * 4,
    REG_ADDR_ADDRESS_HIGH		= 4 * 4,
    REG_ADDR_DATA				= 5 * 4
};


void xi_pcie_memory_writer_init(struct pcie_memory_writer *writer, volatile void __iomem *reg_base)
{
    writer->reg_base = reg_base;
}

bool xi_pcie_memory_writer_get_status(struct pcie_memory_writer *writer, bool *interrupt)
{
    u32 status = pci_read_reg32(writer->reg_base+REG_ADDR_STATUS);

    if (NULL != interrupt)
        *interrupt = (status & 0x02) ? true : false;

    return (status & 0x01) ? true : false; // Busy
}

void xi_pcie_memory_writer_clear_interrupt(struct pcie_memory_writer *writer)
{
    pci_write_reg32(writer->reg_base+REG_ADDR_STATUS, 0x02);
}

void xi_pcie_memory_writer_write(struct pcie_memory_writer *writer, u32 addr_high, u32 addr_low, u32 data)
{
    pci_write_reg32(writer->reg_base+REG_ADDR_DATA, data);
    pci_write_reg32(writer->reg_base+REG_ADDR_ADDRESS_LOW, addr_low);
    pci_write_reg32(writer->reg_base+REG_ADDR_ADDRESS_HIGH, addr_high);
    pci_write_reg32(writer->reg_base+REG_ADDR_CONTROL, 0x01);
}

