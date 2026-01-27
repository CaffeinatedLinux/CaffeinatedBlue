////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#ifndef __PCIE_MEMORY_WRITER_H__
#define __PCIE_MEMORY_WRITER_H__

#include "ospi/ospi.h"

struct pcie_memory_writer {
    volatile void __iomem *reg_base;
};

void xi_pcie_memory_writer_init(struct pcie_memory_writer *writer, volatile void __iomem *reg_base);

bool xi_pcie_memory_writer_get_status(struct pcie_memory_writer *writer, bool *interrupt);

void xi_pcie_memory_writer_clear_interrupt(struct pcie_memory_writer *writer);

void xi_pcie_memory_writer_write(struct pcie_memory_writer *writer, u32 addr_high, u32 addr_low, u32 data);

#endif /* __PCIE_MEMORY_WRITER_H__ */

