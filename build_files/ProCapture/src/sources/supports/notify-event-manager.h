////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#ifndef __NOTIFY_EVENT_MANAGER_H__
#define __NOTIFY_EVENT_MANAGER_H__
#include "ospi/ospi.h"

#include "xi-notify-event.h"

typedef struct _xi_notify_event_manager {
    struct list_head       notify_list;

    os_spinlock_t          lock;
} xi_notify_event_manager;


int xi_notify_event_manager_init(xi_notify_event_manager *nm);

void xi_notify_event_manager_deinit(xi_notify_event_manager *nm);

xi_notify_event *xi_notify_event_manager_add(xi_notify_event_manager *nm,
                                             u64 enable_bits,
                                             os_event_t event);

void xi_notify_event_manager_remove(xi_notify_event_manager *nm, xi_notify_event *event);

void xi_notify_event_manager_set_enable_bits(xi_notify_event_manager *nm,
        xi_notify_event *event, u64 enable_bits);

u64 xi_notify_event_manager_get_status_bits(xi_notify_event_manager *nm, xi_notify_event *event);

void xi_notify_event_manager_notify(xi_notify_event_manager *nm, u64 event_bits);

#endif /* __NOTIFY_EVENT_MANAGER_H__ */

