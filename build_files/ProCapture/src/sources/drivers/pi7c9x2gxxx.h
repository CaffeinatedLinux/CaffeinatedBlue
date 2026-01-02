////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#ifndef __PI7C9X2GXXX_H__
#define __PI7C9X2GXXX_H__

#include "ospi/ospi.h"
#include "win-types.h"

struct mw_pi7c9x2gxxx {
    os_pci_dev_t            pci_dev;
    os_pci_dev_t            par_dev;

    WORD                    m_wDeviceID;
};

void mw_pi7c9x2gxxx_init(struct mw_pi7c9x2gxxx *pobj,
                         os_pci_dev_t pci_dev,
                         os_pci_dev_t par_dev);

int mw_pi7c9x2gxxx_acs_bugfix(struct mw_pi7c9x2gxxx *pobj);

BOOLEAN mw_pi7c9x2gxxx_is_eeprom_present(struct mw_pi7c9x2gxxx *pobj);

int mw_pi7c9x2gxxx_init_eeprom_content(struct mw_pi7c9x2gxxx *pobj);

#endif /* __PI7C9X2GXXX_H__ */
