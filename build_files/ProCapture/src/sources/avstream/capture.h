////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#ifndef __XI_CAPTURE_H__
#define __XI_CAPTURE_H__

#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <linux/version.h>

#include <xi-version.h>
#include "dma/mw-dma-mem-priv.h"
#include "ospi/linux-file.h"
#include "xi-driver.h"
#include "ospi/ospi.h"
#include "v4l2.h"
#include "alsa.h"
#include "mw-uio-drv.h"
#include "picopng/picopng.h"

#define VIDEO_CAP_DRIVER_NAME "Pro Capture"

struct xi_cap_context {
    void __iomem            *reg_mem; /* pointer to mapped registers memory */
    struct xi_driver        driver;
    struct xi_v4l2_dev      v4l2;
    struct snd_card         *card[MAX_CHANNELS_PER_DEVICE];

    struct tasklet_struct   irq_tasklet;

    bool                    msi_enabled;

    struct cap_device_info  info;
};

int loadPngImage(const char *filename, png_pix_info_t *dinfo);

#endif
