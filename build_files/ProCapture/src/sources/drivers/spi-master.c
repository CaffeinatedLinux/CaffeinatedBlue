////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#include "spi-master.h"

enum REG_ADDR {
    REG_ADDR_VER_CAPS		= 4 * 0,
    REG_ADDR_CONFIG			= 4 * 1,
    REG_ADDR_CHIP_SELECT	= 4 * 2,			// Chip select masks
    REG_ADDR_DATA			= 4 * 3,			// Data
    REG_ADDR_DATA_WORD_X2	= 4 * 4,
    REG_ADDR_DATA_BYTE_X4	= 4 * 5
};

void xi_spi_init(struct xi_spi_master *spi,
        volatile void __iomem *baseaddr,
        u32 clk_freq, u32 bitrate, u8 bits_width)
{
    u32 clock_half_div = (clk_freq + bitrate) / (2 * bitrate) - 1;

    spi->reg_base = baseaddr;

    pci_write_reg32(spi->reg_base+REG_ADDR_CONFIG, (bits_width & 0x07) | ((clock_half_div & 0xFFF) << 4));
}

void xi_spi_set_chip_select(struct xi_spi_master *spi, u32 masks)
{
    u32 chip_select = pci_read_reg32(spi->reg_base+REG_ADDR_CHIP_SELECT);

    chip_select &= ~masks;

    pci_write_reg32(spi->reg_base+REG_ADDR_CHIP_SELECT, chip_select);
}

u8 xi_spi_get_version(struct xi_spi_master *spi)
{
    return (pci_read_reg32(spi->reg_base+REG_ADDR_VER_CAPS) >> 28);
}

bool xi_spi_is_support_dualwordmode(struct xi_spi_master *spi)
{
    return xi_spi_get_version(spi) >= 2;
}

bool xi_spi_is_support_quadbytemode(struct xi_spi_master *spi)
{
    return xi_spi_get_version(spi) >= 2;
}

void xi_spi_clear_chip_select(struct xi_spi_master *spi, u32 masks)
{
    u32 chip_select = pci_read_reg32(spi->reg_base+REG_ADDR_CHIP_SELECT);

    chip_select |= masks;

    pci_write_reg32(spi->reg_base+REG_ADDR_CHIP_SELECT, chip_select);
}

u32 xi_spi_read(struct xi_spi_master *spi)
{
    return pci_read_reg32(spi->reg_base+REG_ADDR_DATA);
}

void xi_spi_write(struct xi_spi_master *spi, u32 data)
{
    pci_write_reg32(spi->reg_base+REG_ADDR_DATA, data);
}

u32 xi_spi_read_dualword(struct xi_spi_master *spi)
{
    return pci_read_reg32(spi->reg_base+REG_ADDR_DATA_WORD_X2);
}

void xi_spi_write_dualword(struct xi_spi_master *spi, u32 data)
{
    pci_write_reg32(spi->reg_base+REG_ADDR_DATA_WORD_X2, data);
}

u32 xi_spi_read_quadbyte(struct xi_spi_master *spi)
{
    return pci_read_reg32(spi->reg_base+REG_ADDR_DATA_BYTE_X4);
}

void xi_spi_write_quadbyte(struct xi_spi_master *spi, u32 data)
{
    pci_write_reg32(spi->reg_base+REG_ADDR_DATA_BYTE_X4, data);
}
