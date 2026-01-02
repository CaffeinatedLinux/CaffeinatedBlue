////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include "xi-driver-priv.h"
#include "ospi/ospi.h"

void parse_internal_params(struct xi_driver *driver, const char *internal_params)
{
    char *pos_start = NULL;
    char *pos_next = NULL;
    char *pos_end = NULL;
    char *key_ptr = NULL;
    char *value_ptr = NULL;

    char *params_copy = NULL;
    int params_len = 0;

    if (internal_params == NULL)
        return;

    params_len = strlen(internal_params);
    if (params_len < 2)
        return;

    params_copy = (char *)os_malloc(params_len + 1);
    if (params_copy == NULL)
        return;

    os_strlcpy(params_copy, internal_params, params_len + 1);
    pos_start = params_copy;
    pos_end = pos_start + params_len;

    while (pos_start != NULL && pos_start < pos_end) {
        pos_next = strchr(pos_start, ',');
        if (pos_next != NULL && pos_next < pos_end) {
            *pos_next = '\0';
            pos_next++;
        }

        key_ptr = pos_start;
        pos_start = pos_next;

        value_ptr = strchr(key_ptr, '=');
        if (value_ptr != NULL && value_ptr < (key_ptr + strlen(key_ptr))) {
            value_ptr++;
        } else {
            continue;
        }

        if (strncmp(key_ptr, "hdcp", sizeof("hdcp")-1) == 0) {
            int value = -1;
            sscanf(value_ptr, "%d", &value);
            if (value == 0 || value == 1)
                xi_driver_set_hdcp_support(driver, value);
            continue;
        }

        if (strncmp(key_ptr, "force2si", sizeof("force2si")-1) == 0) {
            int value = -1;
            sscanf(value_ptr, "%d", &value);
            if (value == 0 || value == 1)
                xi_driver_SetQuadSDIForceConvMode2SI(driver, !!value);
            continue;
        }
    }
}
