////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#ifndef __XI_HEAP_H__
#define __XI_HEAP_H__

#include "ospi/ospi.h"

struct xi_mem_desc {
	struct list_head        queue;
    bool                    is_free;
    u32                     addr;
    u32                     size;
};

struct xi_heap {
    void           *priv;
};

int xi_heap_create(struct xi_heap *heap, u32 addr, u32 size);

void xi_heap_destroy(struct xi_heap *heap);

struct xi_mem_desc *xi_heap_alloc(struct xi_heap *heap, u32 size);

void xi_heap_free(struct xi_heap *heap, struct xi_mem_desc *desc);

u32 xi_heap_get_size(struct xi_heap *heap);

u32 xi_heap_get_alloc_size(struct xi_heap *heap);

u32 xi_heap_get_free_size(struct xi_heap *heap);

#endif /* __XI_HEAP_H__ */

