////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#ifndef __XI_TIMESTAMP_H__
#define __XI_TIMESTAMP_H__

#include "ospi/ospi.h"

#define TIMESTAMP_INFINITE              (-1LL)

enum INT_MASK {
    INT_MASK_ALARM_FIRED			= 1 << 0
};

struct xi_timestamp {
    volatile void __iomem *reg_base;
};

void xi_timestamp_init(struct xi_timestamp *ts, volatile void __iomem *reg_base);

void xi_timestamp_set_int_enables(struct xi_timestamp *ts, u32 enable_bits);

u32 xi_timestamp_get_int_status(struct xi_timestamp *ts);

u32 xi_timestamp_get_int_raw_status(struct xi_timestamp *ts);

void xi_timestamp_set_time(struct xi_timestamp *ts, long long lltime);

void xi_timestamp_set_threshold(struct xi_timestamp *ts, u32 threshold);

void xi_timestamp_update_time(struct xi_timestamp *ts, long long lltime);

long long xi_timestamp_get_time(struct xi_timestamp *ts);

void xi_timestamp_set_alarm_time(struct xi_timestamp *ts, long long lltime);

void xi_timestamp_enable_alarm_int(struct xi_timestamp *ts, bool enable);

#endif /* __XI_TIMESTAMP_H__ */

