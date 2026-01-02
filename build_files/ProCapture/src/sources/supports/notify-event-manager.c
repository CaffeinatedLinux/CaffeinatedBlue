////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "notify-event-manager.h"

int xi_notify_event_manager_init(xi_notify_event_manager *nm)
{
    INIT_LIST_HEAD(&nm->notify_list);
    nm->lock = os_spin_lock_alloc();
    if (nm->lock == NULL)
        return OS_RETURN_NOMEM;

    return OS_RETURN_SUCCESS;
}

void xi_notify_event_manager_deinit(xi_notify_event_manager *nm)
{
    xi_notify_event *event;

    os_spin_lock_bh(nm->lock);
    while (!list_empty(&nm->notify_list)) {
        event = list_entry(nm->notify_list.next, xi_notify_event, list_node);
        if (event != NULL) {
            list_del(&event->list_node);
            if (event->event != NULL && event->need_free)
                os_event_free(event->event);
            os_free(event);
        }
    }
    os_spin_unlock_bh(nm->lock);

    os_spin_lock_free(nm->lock);
}

xi_notify_event *xi_notify_event_manager_add(xi_notify_event_manager *nm,
                                             u64 enable_bits,
                                             os_event_t event)
{
    xi_notify_event *notify = NULL;

    notify = os_zalloc(sizeof(*notify));
    if (notify == NULL)
        return NULL;

    notify->enable_bits = enable_bits;
    notify->status_bits = 0;

    if (event != NULL) {
        notify->event = event;
        notify->need_free = false;
    } else {
        notify->event = os_event_alloc();
        if (notify->event == NULL) {
            os_free(notify);
            return NULL;
        }
        notify->need_free = true;
    }

    os_spin_lock_bh(nm->lock);
    list_add_tail(&notify->list_node, &nm->notify_list);
    os_spin_unlock_bh(nm->lock);

    return notify;
}

void xi_notify_event_manager_remove(xi_notify_event_manager *nm, xi_notify_event *event)
{
    os_spin_lock_bh(nm->lock);
    list_del(&event->list_node);
    os_spin_unlock_bh(nm->lock);

    if (event->event != NULL && event->need_free)
        os_event_free(event->event);
    os_free(event);
}

void xi_notify_event_manager_set_enable_bits(xi_notify_event_manager *nm,
        xi_notify_event *event, u64 enable_bits)
{
    os_spin_lock_bh(nm->lock);
    event->enable_bits = enable_bits;
    os_spin_unlock_bh(nm->lock);
}

u64 xi_notify_event_manager_get_status_bits(xi_notify_event_manager *nm, xi_notify_event *event)
{
    u64 status;

    os_spin_lock_bh(nm->lock);
    status = event->status_bits;
    event->status_bits = 0;
    os_event_clear(event->event);
    os_spin_unlock_bh(nm->lock);

    return status;
}

void xi_notify_event_manager_notify(xi_notify_event_manager *nm, u64 event_bits)
{
    xi_notify_event *event;
    struct list_head *pos;
    u64 notify_bits;

    os_spin_lock_bh(nm->lock);
    if (!list_empty(&nm->notify_list)) {
        list_for_each(pos, &nm->notify_list) {
            event = list_entry(pos, xi_notify_event, list_node);
            if (event != NULL) {
                notify_bits = (event->enable_bits & event_bits);
                if (notify_bits) {
                    event->status_bits |= notify_bits;
                    os_event_set(event->event);
                }
            }
        }
    }
    os_spin_unlock_bh(nm->lock);
}

