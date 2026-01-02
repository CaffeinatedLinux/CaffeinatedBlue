////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#include "shared-image-buffer.h"

int shared_image_buf_manager_init(struct shared_image_buf_manager *manager, struct xi_heap *pheap)
{
    manager->m_pheap = pheap;
    INIT_LIST_HEAD(&manager->m_buf_list);
    manager->m_lock = os_spin_lock_alloc();
    if (manager->m_lock == NULL)
        return OS_RETURN_NOMEM;

    return 0;
}

void shared_image_buf_manager_deinit(struct shared_image_buf_manager *manager)
{
    struct shared_image_buf *pbuf = NULL;

    os_spin_lock(manager->m_lock);
    while(!list_empty(&manager->m_buf_list)) {
        pbuf = list_entry(manager->m_buf_list.next, struct shared_image_buf, list_node);
        if (pbuf != NULL) {
            list_del(&pbuf->list_node);
            xi_image_buffer_destroy(&pbuf->image_buf);
            os_free(pbuf);
        }
    }
    os_spin_unlock(manager->m_lock);
    os_spin_lock_free(manager->m_lock);
}

struct shared_image_buf *shared_image_buf_create(struct shared_image_buf_manager *manager,
        int cx, int cy)
{
    struct shared_image_buf *pbuf = NULL;

    pbuf = os_zalloc(sizeof(*pbuf));
    if (pbuf == NULL)
        return NULL;

    if (!xi_image_buffer_create(&pbuf->image_buf, manager->m_pheap, cx, cy, cx * 4)) {
        os_free(pbuf);
        return NULL;
    }

    pbuf->ref_count = 1;
    os_spin_lock(manager->m_lock);
    list_add_tail(&pbuf->list_node, &manager->m_buf_list);
    os_spin_unlock(manager->m_lock);

    return pbuf;
}

bool shared_image_buf_open(struct shared_image_buf_manager *manager, struct shared_image_buf *pbuf,
        long *ref_count)
{
    struct shared_image_buf *_buf = NULL;
    struct list_head *pos;
    bool found = false;

    os_spin_lock(manager->m_lock);
    list_for_each(pos, &manager->m_buf_list) {
        _buf = list_entry(pos, struct shared_image_buf, list_node);
        if (_buf == pbuf) {
            found = true;
            break;
        }
    }

    if (found) {
        pbuf->ref_count++;
        if (ref_count != NULL)
            *ref_count = pbuf->ref_count;
    }
    os_spin_unlock(manager->m_lock);

    return found;
}

bool shared_image_buf_close(struct shared_image_buf_manager *manager, struct shared_image_buf *pbuf,
        long *ref_count)
{
    struct shared_image_buf *_buf = NULL;
    struct list_head *pos;
    bool found = false;

    os_spin_lock(manager->m_lock);
    list_for_each(pos, &manager->m_buf_list) {
        _buf = list_entry(pos, struct shared_image_buf, list_node);
        if (_buf == pbuf) {
            found = true;
            break;
        }
    }

    if (found) {
        pbuf->ref_count--;
        if (ref_count != NULL)
            *ref_count = pbuf->ref_count;

        if (pbuf->ref_count == 0) {
            xi_image_buffer_destroy(&pbuf->image_buf);
            list_del(&pbuf->list_node);
            os_free(pbuf);
        }
    }
    os_spin_unlock(manager->m_lock);

    return found;
}

bool shared_image_buf_is_valid(struct shared_image_buf_manager *manager,
        struct shared_image_buf *pbuf)
{
    struct shared_image_buf *_buf = NULL;
    struct list_head *pos;
    bool found = false;

    os_spin_lock(manager->m_lock);
    list_for_each(pos, &manager->m_buf_list) {
        _buf = list_entry(pos, struct shared_image_buf, list_node);
        if (_buf == pbuf) {
            found = true;
            break;
        }
    }
    os_spin_unlock(manager->m_lock);

    return found;
}
