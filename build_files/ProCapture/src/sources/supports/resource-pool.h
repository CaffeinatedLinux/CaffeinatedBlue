////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#ifndef __RESOURCE_POOL_H__
#define __RESOURCE_POOL_H__

struct resource_pool {
    void           *priv;
};

int resource_pool_create(struct resource_pool *pool, int num_resources);

void resource_pool_destroy(struct resource_pool *pool);

int resource_pool_alloc_resource(struct resource_pool *pool);

void resource_pool_free_resource(struct resource_pool *pool, int index);

#endif /* __RESOURCE_POOL_H__ */

