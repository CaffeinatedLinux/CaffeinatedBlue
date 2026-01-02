////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "heap.h"

#include "image-buffer.h"

bool xi_image_buffer_create(struct xi_image_buffer *imgbuf, struct xi_heap *heap,
        u32 cx, u32 cy, u32 stride)
{
    u32 img_size;

    if (stride < cx * 4)
        return FALSE;

    imgbuf->m_pHeap = heap;

    img_size = stride * cy;
    imgbuf->m_pMemDesc = xi_heap_alloc(heap, img_size);

    imgbuf->m_cx = cx;
    imgbuf->m_cy = cy;
    imgbuf->m_cbStride = stride;

    return (imgbuf->m_pMemDesc != NULL);
}

void xi_image_buffer_destroy(struct xi_image_buffer *imgbuf)
{
    if (imgbuf->m_pMemDesc) {
        xi_heap_free(imgbuf->m_pHeap, imgbuf->m_pMemDesc);
        imgbuf->m_pMemDesc = NULL;
    }
}

u32 xi_image_buffer_get_address(struct xi_image_buffer *imgbuf)
{
    return (imgbuf->m_pMemDesc != NULL) ? imgbuf->m_pMemDesc->addr : 0;
}

