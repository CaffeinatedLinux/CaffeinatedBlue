////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#include "ospi/ospi.h"
#include "resource-pool.h"

typedef struct _resource_item {
    struct list_head        list_node;
    int			            index;
} resource_item;

typedef struct _resource_pool_priv {
    resource_item          *resources;
    int                     num_resources;

	struct list_head        list_free;

    os_spinlock_t           lock;
    os_event_t              wait;
} resource_pool_priv;

int resource_pool_create(struct resource_pool *pool, int num_resources)
{
    resource_pool_priv *priv;
    int i;
    int ret = 0;

    if (NULL == pool || NULL != pool->priv)
        return OS_RETURN_INVAL;

    priv = os_malloc(sizeof(resource_pool_priv));
    if (NULL == priv)
        return OS_RETURN_NOMEM;

    priv->resources = os_malloc(sizeof(resource_item) * num_resources);
    if (NULL == priv->resources) {
        ret = OS_RETURN_NOMEM;
        goto res_err;
    }

    priv->lock = os_spin_lock_alloc();
    if (priv->lock == NULL) {
        ret = OS_RETURN_NOMEM;
        goto spin_err;
    }
    priv->wait = os_event_alloc();
    if (priv->wait == NULL) {
        ret = OS_RETURN_NOMEM;
        goto event_err;
    }

    INIT_LIST_HEAD(&priv->list_free);

    for (i=0; i < num_resources; i++) {
        priv->resources[i].index = i;
        list_add_tail(&(priv->resources[i].list_node), &priv->list_free);
    }

    priv->num_resources = num_resources;

    pool->priv = priv;

    return OS_RETURN_SUCCESS;

event_err:
    os_spin_lock_free(priv->lock);
spin_err:
    os_free(priv->resources);
res_err:
    os_free(priv);
    return ret;
}

void resource_pool_destroy(struct resource_pool *pool)
{
    resource_pool_priv *priv;

    if (NULL == pool || NULL == pool->priv)
        return;

    priv = pool->priv;

    if (priv->lock != NULL) {
        os_spin_lock_free(priv->lock);
        priv->lock = NULL;
    }

    if (priv->wait != NULL) {
        os_event_free(priv->wait);
        priv->wait = NULL;
    }

    if (NULL != priv->resources) {
        os_free(priv->resources);
        priv->resources = NULL;
    }

    os_free(priv);

    pool->priv = NULL;
}

int resource_pool_alloc_resource(struct resource_pool *pool)
{
    resource_pool_priv *priv = pool->priv;
    resource_item *pitem = NULL;
    int ret;

    do {
        os_spin_lock(priv->lock);
        if (!list_empty(&priv->list_free)) {
            pitem = list_entry(priv->list_free.next, resource_item, list_node);
            list_del(&pitem->list_node);
        }
        os_spin_unlock(priv->lock);

        if (NULL != pitem)
            return pitem->index;

        // wait
        ret = os_event_wait(priv->wait, -1);
        if (ret <= 0)
            return OS_RETURN_ERROR;
    } while (NULL == pitem);

    return OS_RETURN_ERROR;
}

void resource_pool_free_resource(struct resource_pool *pool, int index)
{
    resource_pool_priv *priv = pool->priv;
    resource_item *pitem = NULL;

    if (index < 0 || index >= priv->num_resources)
        return;

    pitem = &(priv->resources[index]);

    os_spin_lock(priv->lock);
    list_add_tail(&pitem->list_node, &priv->list_free);
    os_spin_unlock(priv->lock);

    os_event_set(priv->wait);
}
