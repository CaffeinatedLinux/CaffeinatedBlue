////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#ifndef __SPI_MASTER_H__
#define __SPI_MASTER_H__

#include "ospi/ospi.h"

enum SPI_BITS_WIDTH {
    SPI_BITS_WIDTH_8			= 1,
    SPI_BITS_WIDTH_16			= 2,
    SPI_BITS_WIDTH_32			= 4
};

struct xi_spi_master {
    volatile void __iomem *reg_base;
};

void xi_spi_init(struct xi_spi_master *spi,
        volatile void __iomem *baseaddr,
        u32 clk_freq, u32 bitrate, u8 bits_width);

void xi_spi_set_chip_select(struct xi_spi_master *spi, u32 masks);

u8 xi_spi_get_version(struct xi_spi_master *spi);

bool xi_spi_is_support_dualwordmode(struct xi_spi_master *spi);

bool xi_spi_is_support_quadbytemode(struct xi_spi_master *spi);

void xi_spi_clear_chip_select(struct xi_spi_master *spi, u32 masks);

u32 xi_spi_read(struct xi_spi_master *spi);

void xi_spi_write(struct xi_spi_master *spi, u32 data);

u32 xi_spi_read_dualword(struct xi_spi_master *spi);

void xi_spi_write_dualword(struct xi_spi_master *spi, u32 data);

u32 xi_spi_read_quadbyte(struct xi_spi_master *spi);

void xi_spi_write_quadbyte(struct xi_spi_master *spi, u32 data);

#endif /* __SPI_MASTER_H__ */
