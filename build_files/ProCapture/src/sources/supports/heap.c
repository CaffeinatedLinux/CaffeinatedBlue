////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "heap.h"

typedef struct _xi_heap_priv {
	struct list_head        memlist;

    os_spinlock_t           lock;
    u32                     total_size;
    u32                     free_size;
} xi_heap_priv;

int xi_heap_create(struct xi_heap *heap, u32 addr, u32 size)
{
    struct xi_mem_desc *desc;
    xi_heap_priv *priv;
    int ret;

    priv = os_zalloc(sizeof(xi_heap_priv));
    if (NULL == priv)
        return OS_RETURN_NOMEM;

    desc = os_zalloc(sizeof(struct xi_mem_desc));
    if (NULL == desc) {
        ret = OS_RETURN_NOMEM;
        goto err;
    }

    priv->lock = os_spin_lock_alloc();
    if (priv->lock == NULL)
        goto err;

    desc->is_free = true;
    desc->addr = (addr + 3) & ~3;
    desc->size = size & ~3;

    priv->total_size = desc->size;
    priv->free_size = priv->total_size;

    INIT_LIST_HEAD(&priv->memlist);

    list_add_tail(&desc->queue, &priv->memlist);

    heap->priv = priv;

    return 0;

err:
    if (priv->lock != NULL)
        os_spin_lock_free(priv->lock);
    if (desc != NULL)
        os_free(desc);
    INIT_LIST_HEAD(&priv->memlist);
    if (priv != NULL)
        os_free(priv);
    heap->priv = NULL;
    return ret;
}

void xi_heap_destroy(struct xi_heap *heap)
{
    xi_heap_priv *priv;
    struct xi_mem_desc *desc;

    if (NULL == heap || NULL == heap->priv)
        return;

    priv = heap->priv;

    os_spin_lock(priv->lock);
    while (!list_empty(&priv->memlist)) {
        desc = list_entry(priv->memlist.next, struct xi_mem_desc, queue);
        list_del(&desc->queue);
        os_free(desc);
    }
    priv->total_size = 0;
    priv->free_size = 0;
    os_spin_unlock(priv->lock);

    os_spin_lock_free(priv->lock);

    os_free(priv);
    heap->priv = NULL;
}

#if 0
static void _mem_list_dump(xi_heap_priv *priv)
{
    struct list_head *pos;
    struct xi_mem_desc *desc = NULL;

    xi_debug(0, "*** HEAP MEM LIST ***");
    if (!list_empty(&priv->memlist)) {
        list_for_each(pos, &priv->memlist) {
            desc = list_entry(pos, struct xi_mem_desc, queue);
            xi_debug(0, "\t MEM at %08X, size: %d, free: %d\n",
                    desc->addr, desc->size, desc->is_free);
        }
    }
}
#endif

struct xi_mem_desc *xi_heap_alloc(struct xi_heap *heap, u32 size)
{
    bool found = false;
    struct xi_mem_desc *desc = NULL;
    struct list_head *pos;
    u32 size_free;
    xi_heap_priv *priv = heap->priv;

    size = (size + 3) & ~3;

    os_spin_lock(priv->lock);
    if (size > priv->total_size)
        goto out;

    if (!list_empty(&priv->memlist)) {
        list_for_each(pos, &priv->memlist) {
            desc = list_entry(pos, struct xi_mem_desc, queue);
            if (desc->is_free && desc->size >= size) {
                found = true;
                break;
            }
        }
    }

    if (!found)
        goto out;

    size_free = desc->size - size;
    if (size_free != 0) {
        struct xi_mem_desc *desc_free;
        os_spin_unlock(priv->lock); // malloc maybe sleep
        desc_free = os_malloc(sizeof(struct xi_mem_desc));
        os_spin_lock(priv->lock);
        if (NULL == desc_free)
            goto out;

        desc->size = size;

        desc_free->is_free = true;
        desc_free->addr = desc->addr + size;
        desc_free->size = size_free;

        list_add(&desc_free->queue, &desc->queue);
    }

    desc->is_free = false;

    priv->free_size -= size;

out:
    //_mem_list_dump(priv);
    os_spin_unlock(priv->lock);
    return desc;
}

void xi_heap_free(struct xi_heap *heap, struct xi_mem_desc *desc)
{
    xi_heap_priv *priv = heap->priv;

    struct xi_mem_desc *prev = NULL;
    struct xi_mem_desc *next = NULL;

    os_spin_lock(priv->lock);

    desc->is_free = true;
    priv->free_size += desc->size;

    /* test if have prev */
    if (desc->queue.prev != &priv->memlist)
        prev = list_entry(desc->queue.prev, struct xi_mem_desc, queue);
    if (NULL != prev && prev->is_free) {
        prev->size += desc->size;
        list_del(&desc->queue);
        os_free(desc);
        desc = prev;
    }

    /* test if have next */
    if (desc->queue.next != &priv->memlist)
        next = list_entry(desc->queue.next, struct xi_mem_desc, queue);
    if (NULL != next && next->is_free) {
        desc->size += next->size;
        list_del(&next->queue);
        os_free(next);
    }
    //_mem_list_dump(priv);
    os_spin_unlock(priv->lock);
}

u32 xi_heap_get_size(struct xi_heap *heap)
{
    u32 size;
    xi_heap_priv *priv = heap->priv;

    os_spin_lock(priv->lock);
    size = priv->total_size;
    os_spin_unlock(priv->lock);

    return size;
}

u32 xi_heap_get_alloc_size(struct xi_heap *heap)
{
    u32 size;
    xi_heap_priv *priv = heap->priv;

    os_spin_lock(priv->lock);
    size = priv->total_size - priv->free_size;
    os_spin_unlock(priv->lock);

    return size;
}

u32 xi_heap_get_free_size(struct xi_heap *heap)
{
    u32 size;
    xi_heap_priv *priv = heap->priv;

    os_spin_lock(priv->lock);
    size = priv->free_size;
    os_spin_unlock(priv->lock);

    return size;
}

